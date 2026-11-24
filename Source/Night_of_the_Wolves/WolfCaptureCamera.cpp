#include "WolfCaptureCamera.h"
#include "WolfMeshComponent.h"
#include "WolfPackComponent.h"
#include "WolfTrailGameMode.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "CollisionShape.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#if !UE_BUILD_SHIPPING
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#endif

namespace
{
constexpr float GatherTime = .30f;
}

AWolfCaptureCamera::AWolfCaptureCamera()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	GetCameraComponent()->SetFieldOfView(70.f);
	GetCameraComponent()->bConstrainAspectRatio = false;
	MoonFill = CreateDefaultSubobject<UPointLightComponent>(TEXT("Reflected moonlight"));
	MoonFill->SetupAttachment(GetRootComponent());
	MoonFill->SetUsingAbsoluteLocation(true);
	MoonFill->SetIntensityUnits(ELightUnits::Candelas);
	MoonFill->SetLightColor(FLinearColor(.68f,.80f,1.f));
	MoonFill->SetIntensity(0.f);
	MoonFill->SetAttenuationRadius(440.f);
	MoonFill->SetSourceRadius(38.f);
	MoonFill->SetCastShadows(false);
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Front(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Back(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Left(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Left_01"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Right(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Right_01"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Growl(TEXT("/Game/NightOfTheWolves/FirstLeg/Growl"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Thud(TEXT("/Game/NightOfTheWolves/FirstLeg/DoorImpact"));
	FrontFall = Front.Object;
	BackFall = Back.Object;
	LeftFall = Left.Object;
	RightFall = Right.Object;
	Snarl = Growl.Object;
	Impact = Thud.Object;
}

FVector AWolfCaptureCamera::TargetChest() const
{
	USkeletalMeshComponent* Body = Max->GetMesh();
	if (Body && Body->DoesSocketExist(TEXT("spine_04")))
		return Body->GetSocketLocation(TEXT("spine_04"));
	return Max->GetActorLocation() + FVector(0, 0, 38);
}

void AWolfCaptureCamera::Start(AWolfTrailGameMode& Game, ACharacter& Player, UWolfMeshComponent& Leader)
{
	Mode = &Game;
	Max = &Player;
	Wolf = &Leader;
	Pack = Game.GetHuntingPack();
	Wolf->SetVisibility(true);
	LaunchFeet = Wolf->GetComponentLocation();
	const FVector Chest = TargetChest();
	AttackDirection = (Chest - LaunchFeet).GetSafeNormal2D();
	if (AttackDirection.IsNearlyZero()) AttackDirection = Wolf->GetForwardVector();
	const float Gap = FVector::Dist2D(Chest,LaunchFeet);
	FramingCentre = FMath::Lerp(Chest, LaunchFeet + FVector(0,0,83), .40);

	if (APlayerController* PC = Cast<APlayerController>(Player.GetController()))
	{
		int32 Width, Height;
		PC->GetViewportSize(Width,Height);
		if (Width > 0 && Height > 0) ViewAspect = float(Width) / Height;
		FrameAttack(*PC);
		const FVector NearSide = (GetActorLocation() - FramingCentre).GetSafeNormal2D();
		MoonFill->SetWorldLocation(FramingCentre + NearSide * 140. + FVector(0,0,140));
		PC->SetViewTargetWithBlend(this, .16f, VTBlend_Cubic, 0, true);
	}
	if (Snarl) UGameplayStatics::PlaySoundAtLocation(this, Snarl, LaunchFeet, .80f, 1.08f);
#if !UE_BUILD_SHIPPING
	bPhotograph = FParse::Param(FCommandLine::Get(),TEXT("WolfCapturePhotos"));
#endif
	UE_LOG(LogTemp, Display, TEXT("WOLF POUNCE started at CP%d from %s, %.0fcm to Max"), Mode->Checkpoint, Pack->GetTactics().IdentityName(Wolf->GetIdentity()), Gap);
}

void AWolfCaptureCamera::FrameAttack(APlayerController& Controller)
{
	FVector OldCamera;
	FRotator OldLook;
	Controller.GetPlayerViewPoint(OldCamera, OldLook);
	const FVector PreviousSide = (OldCamera - FramingCentre).GetSafeNormal2D();
	const FVector Chest = TargetChest();
	const FVector Shoulder = LaunchFeet + FVector(0,0,60);
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(WolfCaptureView), false, Max);
	double BestScore = -DBL_MAX;
	FVector BestLocation = OldCamera;

	// Try both sides of the action line. Tight approaches such as the cabin can
	// exclude one entirely. Sweep out from Max to keep the lens inside the room.
	for (double Angle : {80., -80., 110., -110., 50., -50., 145., -145., 25., -25., 180.})
	{
		const FVector Axis = AttackDirection.RotateAngleAxis(Angle, FVector::UpVector);
		const FVector Desired = FramingCentre + 415. * Axis + FVector(0,0,83);
		FHitResult Hit;
		const bool bWall = GetWorld()->SweepSingleByChannel(Hit, Chest, Desired, FQuat::Identity,
			ECC_Camera, FCollisionShape::MakeSphere(12.f), Query);
		const FVector Lens = bWall ? Hit.Location + Hit.Normal * 3. : Desired;
		const bool bWolfObscured = GetWorld()->LineTraceTestByChannel(Lens, Shoulder, ECC_Camera, Query);
		const double SideView = 1. - FMath::Abs(FVector::DotProduct(Axis, AttackDirection));
		const double Score = FMath::Min(415., FVector::Dist(Lens, FramingCentre))
			+ 76. * SideView + 30. * FVector::DotProduct(Axis, PreviousSide) - (bWolfObscured ? 230. : 0.);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestLocation = Lens;
		}
	}
	SetActorLocationAndRotation(BestLocation, (FramingCentre - BestLocation).Rotation());
}

void AWolfCaptureCamera::HitPlayer()
{
	bHit = true;
	const FVector Source = -AttackDirection;
	const float Front = FVector::DotProduct(Source, Max->GetActorForwardVector());
	const float Side = FVector::DotProduct(Source, Max->GetActorRightVector());
	UAnimSequence* Reaction = FMath::Abs(Front) >= FMath::Abs(Side)
		? (Front >= 0 ? FrontFall.Get() : BackFall.Get())
		: (Side >= 0 ? RightFall.Get() : LeftFall.Get());
	if (Reaction) Max->GetMesh()->PlayAnimation(Reaction, false);
	if (Impact) UGameplayStatics::PlaySoundAtLocation(this, Impact, TargetChest(), .53f, .72f);
	UE_LOG(LogTemp, Display, TEXT("WOLF POUNCE contact with Max at CP%d, reaction %s"), Mode->Checkpoint, *GetNameSafe(Reaction));
}

void AWolfCaptureCamera::KeepSilhouettesInFrame()
{
	// The nominal side view may back against a trunk or cabin wall. Reserve a
	// small screen margin around both actual poses, widening only as needed.
	const FTransform View = GetActorTransform();
	double HalfWidth = FMath::Tan(FMath::DegreesToRadians(.5 * GetCameraComponent()->FieldOfView));
	auto Include = [&](FVector Point)
	{
		const FVector Local = View.InverseTransformPositionNoScale(Point);
		if (Local.X <= 10.) return;
		const double Edge = FMath::Max(FMath::Abs(Local.Y), ViewAspect * FMath::Abs(Local.Z));
		HalfWidth = FMath::Max(HalfWidth, (Edge + 12.) / (.91 * Local.X));
	};
	const USkeletalMeshComponent* Body = Max->GetMesh();
	Include(Body->GetSocketLocation(TEXT("head")) + FVector(0,0,19));
	Include(Body->GetSocketLocation(TEXT("foot_l")) - FVector(0,0,8));
	Include(Body->GetSocketLocation(TEXT("foot_r")) - FVector(0,0,8));
	Include(Wolf->GetSocketLocation(TEXT("Skull")) + Wolf->GetForwardVector() * 24. + FVector(0,0,13));
	Include(Wolf->GetSocketLocation(TEXT("EarL")) + FVector(0,0,14));
	Include(Wolf->GetSocketLocation(TEXT("BrushTip")) - Wolf->GetForwardVector()*12.);
	static const FName Feet[] = {TEXT("ForeLPaw"),TEXT("ForeRPaw"),TEXT("HindLPaw"),TEXT("HindRPaw")};
	for (FName Foot : Feet) Include(Wolf->GetSocketLocation(Foot));
	GetCameraComponent()->SetFieldOfView(FMath::Min(108.,2. * FMath::RadiansToDegrees(FMath::Atan(HalfWidth))));
}

void AWolfCaptureCamera::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Mode || !Max || !Wolf || bFinished) return;
	Elapsed += Dt;
	if (Pack) Pack->CoverPounce(Dt, Max->GetActorLocation() - FVector(0,0,Max->GetSimpleCollisionHalfHeight()));
	MoonFill->SetIntensity(1.2f * FMath::Min(1.f, Elapsed / .25f));
	const float LeapTime = Elapsed - GatherTime;
	const float Flight = FMath::Max(.15f, Mode->WolfFlightSeconds);
	const float Hold = FMath::Max(.15f, Mode->WolfImpactSeconds);
	const float T = FMath::Clamp(LeapTime / Flight, 0.f, 1.f);
	const float AfterHit = FMath::Clamp((LeapTime - Flight) / Hold, 0.f, 1.f);
	const float Gather = FMath::Clamp(1.f + LeapTime / GatherTime, 0.f, 1.f);
	const FVector Crouch = LaunchFeet - AttackDirection * (9. * Gather);
	if (!bHit && LeapTime >= Flight) HitPlayer();

	// Keep both forepaws braced on the near shoulder as Max recoils. The reach in
	// rig space is rotated with the shoulder-up pitch before solving foot origin.
	const FVector Chest = TargetChest();
	const float Pitch = 22.f * FMath::Sin(T * UE_HALF_PI) - 12.f * AfterHit;
	FRotator Heading = AttackDirection.Rotation();
	Heading.Pitch = Pitch;
	Wolf->SetPouncePose(LeapTime < 0 ? 0.f : T);
	const FVector LeftPaw = Wolf->GetSocketTransform(TEXT("ForeLPaw"),RTS_Component).GetLocation();
	const FVector RightPaw = Wolf->GetSocketTransform(TEXT("ForeRPaw"),RTS_Component).GetLocation();
	const FVector PawCentre = (LeftPaw + RightPaw) * .5;
	const FVector ContactOrigin = Chest - AttackDirection * 13.
		- Heading.RotateVector(PawCentre * Wolf->GetComponentScale());
	FVector Position = FMath::Lerp(Crouch, ContactOrigin, T);
	Position.Z += 4.f * T * (1.f - T) * 75.f;
	// Keep the wolf above the local floor once Max drops.
	Position.Z = FMath::Max(Position.Z, LaunchFeet.Z - 3.);
	Wolf->SetWorldLocationAndRotation(Position, Heading);
	const FVector Focus = FMath::Lerp(FramingCentre + FVector(0,0,12), Chest + FVector(0,0,11), .48f * AfterHit);
	SetActorRotation((Focus - GetActorLocation()).Rotation());
	KeepSilhouettesInFrame();
#if !UE_BUILD_SHIPPING
	if (bPhotograph)
	{
		if (ReviewFrame == 0 && Gather >= .8f) Review(TEXT("Takeoff"));
		else if (ReviewFrame == 1 && T >= .50f) Review(TEXT("Airborne"));
		else if (ReviewFrame == 2 && AfterHit >= .10f) Review(TEXT("Contact"));
	}
#endif
	if (LeapTime >= Flight + Hold)
	{
		bFinished = true;
		Mode->ShowDeathCard();
#if !UE_BUILD_SHIPPING
		if (bPhotograph) Review(TEXT("Death"));
#endif
		SetActorTickEnabled(false);
	}
}

#if !UE_BUILD_SHIPPING
void AWolfCaptureCamera::Review(const TCHAR* Stage)
{
	++ReviewFrame;
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("EncounterReview"));
	IFileManager::Get().MakeDirectory(*Folder,true);
	FScreenshotRequest::RequestScreenshot(Folder / FString::Printf(TEXT("CP%d-Wolf-%s.png"), Mode->Checkpoint, Stage),false,false);
}
#endif
