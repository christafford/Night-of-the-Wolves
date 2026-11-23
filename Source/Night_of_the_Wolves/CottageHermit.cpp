#include "CottageHermit.h"
#include "DialogueVoiceComponent.h"
#include "DialogueFaceAnimInstance.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#if !UE_BUILD_SHIPPING
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#endif
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ACottageHermit::ACottageHermit()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Feet")));
	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Resident"));
	Face = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Face"));
	Workwear = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Bib overalls and work shirt"));
	Body->SetupAttachment(GetRootComponent());
	Body->SetRelativeRotation(FRotator(0,90,0));
	Face->SetupAttachment(Body);
	Workwear->SetupAttachment(Body);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMesh(TEXT("/Game/NightOfTheWolves/Max/Wardrobe/SK_MaxExposedBody"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FaceMesh(TEXT("/Game/NightOfTheWolves/FirstLeg/Hermit/Hermit_Head"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Clothes(TEXT("/Game/NightOfTheWolves/FirstLeg/Hermit/SK_HermitWorkwear"));
	Body->SetSkeletalMesh(BodyMesh.Object);
	Face->SetSkeletalMesh(FaceMesh.Object);
	Face->SetAnimInstanceClass(UDialogueFaceAnimInstance::StaticClass());
	Workwear->SetSkeletalMesh(Clothes.Object);
	for (auto* Part : {Body.Get(), Face.Get(), Workwear.Get()})
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
	}
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Hold(TEXT("/Game/NightOfTheWolves/FirstLeg/Hermit/A_HermitAim"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Run(TEXT("/Game/NightOfTheWolves/FirstLeg/Hermit/A_HermitJog"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> Slump(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Left_01"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Report(TEXT("/Game/NightOfTheWolves/FirstLeg/Shot"));
	Aim = Hold.Object;
	Jog = Run.Object;
	Collapse = Slump.Object;
	Shot = Report.Object;
	static ConstructorHelpers::FObjectFinder<USoundBase> Warning(TEXT("/Game/NightOfTheWolves/FirstLeg/Voices/HermitWarning"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Shout(TEXT("/Game/NightOfTheWolves/FirstLeg/Voices/HermitShout"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Pit(TEXT("/Game/NightOfTheWolves/FirstLeg/Voices/HermitPit"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Yell(TEXT("/Game/NightOfTheWolves/FirstLeg/Hermit/SC_HermitFallYell"));
	WarningVoice = Warning.Object;
	ShoutVoice = Shout.Object;
	PitVoice = Pit.Object;
	FallYell = Yell.Object;
	Voice = CreateDefaultSubobject<UDialogueVoiceComponent>(TEXT("HermitVoice"));
	Voice->SetupAttachment(GetRootComponent());
	Voice->bOverrideAttenuation = true;
	Voice->AttenuationOverrides.AttenuationShapeExtents = FVector(380,0,0);
	Voice->AttenuationOverrides.FalloffDistance = 1550.f;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> HatMesh(TEXT("/Game/NightOfTheWolves/FirstLeg/Hermit/SM_HermitFeltHat"));
	auto* Hat = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Felt hat"));
	Hat->SetupAttachment(Face, TEXT("head"));
	Hat->SetStaticMesh(HatMesh.Object);
	Hat->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BeardMesh(TEXT("/Game/NightOfTheWolves/FirstLeg/Hermit/SK_HermitWhiskers"));
	Whiskers = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Animated whiskers"));
	Whiskers->SetupAttachment(Face);
	Whiskers->SetSkeletalMesh(BeardMesh.Object);
	Whiskers->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Shotgun = CreateDefaultSubobject<USceneComponent>(TEXT("Held shotgun"));
	Shotgun->SetupAttachment(GetRootComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Tube(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/NightOfTheWolves/Presentation/M_TreeBark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Iron(TEXT("/Game/NightOfTheWolves/Max/Wardrobe/MI_Hardware"));
	auto Part = [this](FName Name, UStaticMesh* Mesh, UMaterialInterface* Mat, FVector Location, FVector Size, FRotator Rotation)
	{
		auto* Piece = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Piece->SetupAttachment(Shotgun);
		Piece->SetStaticMesh(Mesh);
		Piece->SetMaterial(0,Mat);
		Piece->SetRelativeTransform(FTransform(Rotation,Location,Size));
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	};
	Part(TEXT("Wooden stock"),Cube.Object,Wood.Object,{-16,0,0},{.32,.047,.087},{5,0,0});
	Part(TEXT("Dark iron"),Cube.Object,Iron.Object,{6,0,3.8},{.19,.055,.061},{0,0,0});
	Part(TEXT("Fore-end"),Cube.Object,Wood.Object,{33,0,4.2},{.34,.062,.052},{0,0,0});
	for (int Side : {-1,1})
		Part(Side < 0 ? TEXT("Left tube") : TEXT("Right tube"), Tube.Object,Iron.Object,
			{40,Side*1.1,7.3},{.026,.026,.70},{90,0,0});
	Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Shot flash"));
	Flash->SetupAttachment(Shotgun);
	Flash->SetRelativeLocation({76,0,7.3});
	Flash->SetLightColor(FLinearColor(1.f,.54f,.23f));
	Flash->SetIntensityUnits(ELightUnits::Candelas);
	Flash->SetIntensity(32.f);
	Flash->SetAttenuationRadius(460);
	Flash->SetCastShadows(false);
	Flash->SetVisibility(false);
}

void ACottageHermit::BeginPlay()
{
	Super::BeginPlay();
	for (USoundBase* Cue : {WarningVoice.Get(), ShoutVoice.Get(), PitVoice.Get(), FallYell.Get(), Shot.Get()})
		UGameplayStatics::PrimeSound(Cue);
	Workwear->SetLeaderPoseComponent(Body);
	Whiskers->SetLeaderPoseComponent(Face);
	Face->AddTickPrerequisiteComponent(Body);
	Voice->BindFace(Face);
	Camera = GetWorld()->SpawnActor<ACameraActor>();
	Camera->GetCameraComponent()->SetFieldOfView(57.f);
}

void ACottageHermit::RestoreEncounter(bool bTrapped, FVector Door, FVector Hollow, float HalfWidth)
{
	StopSpeech();
	bWarned = bTrapped;
	EndReveal(false);
	Doorway = FVector(Door.X+150, Door.Y, Hollow.Z);
	Timbers = Hollow;
	HollowHalfWidth = HalfWidth;
	bRunning = false;
	Body->SetRelativeLocation(FVector::ZeroVector);
	Body->PlayAnimation(Aim, true);
	Stage(bTrapped ? 1.f : 0.f, 0.f, bTrapped ? 1.f : 0.f);
	if (bTrapped) RestInPit();
	SetActorHiddenInGame(!bTrapped);
	Flash->SetVisibility(false);
	FlashSeconds = 0.f;
	SetActorTickEnabled(bTrapped);
}

void ACottageHermit::WarnAtWindow()
{
	if (bWarned) return;
	bWarned = true;
	Voice->SetWorldLocation(FVector(2470,220,177));
	Voice->SetVolumeMultiplier(.76f);
	Voice->SetMood(.42f, 0.f);
	Voice->PlayLine(WarningVoice, EDialogueLine::HermitWarning);
}

void ACottageHermit::ShoutAtMax()
{
	Voice->SetWorldLocation(Face->GetSocketLocation(TEXT("head")));
	Voice->SetVolumeMultiplier(1.f);
	Voice->SetMood(1.f, 0.f);
	Voice->PlayLine(ShoutVoice, EDialogueLine::HermitShout);
}

void ACottageHermit::CryFromPit()
{
	Voice->SetWorldLocation(Face->GetSocketLocation(TEXT("head")));
	Voice->SetVolumeMultiplier(.79f);
	Voice->SetMood(.65f, 0.f);
	Voice->PlayLine(PitVoice, EDialogueLine::HermitPit);
}

void ACottageHermit::LoseFooting()
{
	if (Collapse) Body->PlayAnimation(Collapse, false);
	Voice->SetWorldLocation(Face->GetSocketLocation(TEXT("head")));
	Voice->SetVolumeMultiplier(1.f);
	Voice->SetMood(0.f, 0.f);
	Voice->PlayLine(FallYell, EDialogueLine::HermitFallYell);
}

void ACottageHermit::RestInPit()
{
	StopSpeech();
	if (Collapse)
	{
		Body->PlayAnimation(Collapse, false);
		Body->SetPosition(Collapse->GetPlayLength(), false);
		Body->SetPlayRate(0.f);
	}
}

void ACottageHermit::StopSpeech()
{
	Voice->ResetPerformance();
}

void ACottageHermit::AnswerKnock(APawn& Max)
{
	Viewer = &Max;
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	const float Side = Max.GetActorLocation().Y < Timbers.Y ? -1.f : 1.f;
	SidestepY = Timbers.Y + Side*(HollowHalfWidth + 65.f);
	bHoldingMark = false;
	// On Max's side of the action, the hermit's turn brings his expression
	// and gun into three-quarter view, with the jamb and boards in the frame.
	const FVector Cam = Timbers + FVector(-110, Side * 327, 137);
	Camera->SetActorLocationAndRotation(Cam,(FMath::Lerp(Timbers,Doorway,.55f) + FVector(0,0,96) - Cam).Rotation());
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	Stage(0,0,0);
	if (APlayerController* PC = Cast<APlayerController>(Max.GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		if (ACharacter* C = Cast<ACharacter>(&Max))
		{
			auto* Movement = C->GetCharacterMovement();
			bJumpWasAllowed = Movement->IsJumpAllowed();
			bTurnedWithMovement = Movement->bOrientRotationToMovement;
			Movement->bOrientRotationToMovement = false;
			Movement->SetJumpAllowed(false);
			C->StopJumping();
			Movement->StopMovementImmediately();
		}
		PC->SetViewTargetWithBlend(Camera,.36f,VTBlend_Cubic);
	}
}

void ACottageHermit::Stage(float Run, float Turn, float Drop)
{
	const bool bJogging = Run > 0.f && Run < 1.f;
	if (bJogging != bRunning)
	{
		bRunning = bJogging;
		Body->PlayAnimation(bRunning ? Jog.Get() : Aim.Get(),true);
		if (bRunning) Body->SetPlayRate(.88f);
	}
	Fall = Drop;
	AimFraming = Turn;
	float FieldOfView = FMath::Lerp(57.f, 43.f, AimFraming);
#if !UE_BUILD_SHIPPING
	if (PortraitHold > .74f && Turn > .98f) FieldOfView = 21.f;
#endif
	if (Camera) Camera->GetCameraComponent()->SetFieldOfView(FieldOfView);
	const FVector Feet = FMath::Lerp(Doorway,Timbers,Run) - FVector(0,0,219.f*Drop*Drop);
	float Yaw = 0;
	if (Viewer.IsValid())
	{
		const FVector Max = Viewer->GetActorLocation();
		Yaw = Turn * FRotator::NormalizeAxis((Max - Timbers).Rotation().Yaw - 180.f);
		// First clear the hollow sideways, then back down its solid rim. This leaves
		// Max facing the approaching resident and both silhouettes clear of each other.
		const float Side = FMath::Sign(SidestepY - Timbers.Y);
		if (!bHoldingMark)
		{
			if ((SidestepY-Max.Y)*Side > 5.f)
				Viewer->AddMovementInput(FVector(0,Side,0),.80f,true);
			else if (Max.X > Timbers.X-164)
				Viewer->AddMovementInput(FVector(-1,0,0),.78f,true);
			else
			{
				bHoldingMark = true;
				if (auto* C = Cast<ACharacter>(Viewer.Get())) C->GetCharacterMovement()->StopMovementImmediately();
			}
		}
		const FRotator Watch(0,(Feet-Max).Rotation().Yaw,0);
		Viewer->SetActorRotation(FMath::RInterpTo(Viewer->GetActorRotation(),Watch,GetWorld()->GetDeltaSeconds(),8.f));
	}
	SetActorLocationAndRotation(Feet,FRotator(-28.f*Drop,Yaw,17.f*Drop));
}

void ACottageHermit::Tick(float Dt)
{
	Super::Tick(Dt);
	if (Fall > .60f)
	{
		// This collapse was authored on level ground and travels sideways. Let him fold
		// into the narrow excavation without carrying his upper body under solid earth.
		const FVector ChestOffset = Body->GetSocketLocation(TEXT("spine_05")) - Body->GetComponentLocation();
		const float Slide = FMath::SmoothStep(.60f, .98f, Fall);
		Body->SetWorldLocation(GetActorLocation() - FVector(ChestOffset.X, ChestOffset.Y, 0.f) * Slide);
	}
	// Both animated hands define the prop's carry direction. The same grip works
	// in the across-body running pose and the shouldered aiming pose.
	const FVector Right = Body->GetSocketLocation(TEXT("index_01_r"));
	const FVector Left = Body->GetSocketLocation(TEXT("index_01_l"));
	const FVector Forward = (Left-Right).GetSafeNormal();
	const FRotator Grip = FRotationMatrix::MakeFromXZ(Forward,FVector::UpVector).Rotator();
	// The stock slips from his supporting hand as he topples into the hollow.
	const float Slip = FMath::SmoothStep(.40f, .98f, Fall);
	const FTransform GripTransform(Grip, Right);
	const FRotator Tumble(83.f * Slip, 29.f * Slip, 65.f * Slip);
	Shotgun->SetWorldLocationAndRotation(GripTransform.TransformPosition(FVector(0, 0, -18.f * Slip)),
		GripTransform.GetRotation() * Tumble.Quaternion());
	if (Viewer.IsValid())
	{
		const float OverRim = FMath::SmoothStep(.60f, .99f, Fall);
		if (OverRim > 0.f)
		{
			const float Side = FMath::Sign(SidestepY - Timbers.Y);
			const FVector YardAngle(-110.f, Side * 327.f, 137.f);
			const FVector AboveHollow(-55.f, Side * HollowHalfWidth * .68f, 153.f);
			Camera->SetActorLocation(Timbers + FMath::Lerp(YardAngle, AboveHollow, OverRim));
		}
		const FVector StandingFocus = GetActorLocation() + FVector(0,0,97.f + 27.f * AimFraming + 60.f * Fall * Fall);
		const FVector Focus = FMath::Lerp(StandingFocus, Face->GetSocketLocation(TEXT("head")), OverRim);
		const FRotator Look = (Focus-Camera->GetActorLocation()).Rotation();
		Camera->SetActorRotation(FMath::RInterpTo(Camera->GetActorRotation(),Look,Dt,3.f + 6.f * OverRim));
#if !UE_BUILD_SHIPPING
		// The recovery-test screenshot uses the same animated figure and cottage practicals.
		static const bool bReviewPortrait = FParse::Param(FCommandLine::Get(), TEXT("HermitPortrait"));
		if (bReviewPortrait)
			PortraitHold = AimFraming > .98f ? PortraitHold + Dt : 0.f;
		if (bReviewPortrait && PortraitHold > .74f)
		{
			FVector Jaw = Face->GetSocketLocation(TEXT("head"));
			if (Jaw.Z < GetActorLocation().Z + 125.f)
				Jaw = GetActorLocation() + FVector(0, 0, 161.f);
			const float Side = FMath::Sign(SidestepY - Timbers.Y);
			const FVector PortraitCamera = Jaw + FVector(-51.f, Side * 176.f, 4.f);
			const FRotator PortraitLook = (Jaw + FVector(0, 0, 2.5f) - PortraitCamera).Rotation();
			Camera->SetActorLocationAndRotation(PortraitCamera, PortraitLook);
			Camera->GetCameraComponent()->SetFieldOfView(21.f);
			if (PortraitHold >= 1.18f && PortraitHold - Dt < 1.18f)
			{
				const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("FirstLegReview"));
				IFileManager::Get().MakeDirectory(*Folder, true);
				FScreenshotRequest::RequestScreenshot(Folder / TEXT("Hermit-Portrait.png"), false, false);
			}
		}
#endif
	}
	else if (Fall >= 1.f)
	{
		SetActorTickEnabled(false);
		Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}
	if (FlashSeconds > 0)
	{
		FlashSeconds -= Dt;
		if (FlashSeconds <= 0) Flash->SetVisibility(false);
	}
}

void ACottageHermit::Fire()
{
	FlashSeconds = .12f;
	Flash->SetVisibility(true);
	if (Shot) UGameplayStatics::PlaySoundAtLocation(this,Shot,Flash->GetComponentLocation(),.9f);
}

void ACottageHermit::EndReveal(bool bBlend)
{
	if (Viewer.IsValid())
		if (auto* PC = Cast<APlayerController>(Viewer->GetController()))
		{
			PC->SetViewTargetWithBlend(Viewer.Get(), bBlend ? .35f : 0.f, VTBlend_Cubic);
			PC->SetIgnoreMoveInput(false);
			PC->SetIgnoreLookInput(false);
		}
	if (auto* C = Cast<ACharacter>(Viewer.Get()))
	{
		C->GetCharacterMovement()->SetJumpAllowed(bJumpWasAllowed);
		C->GetCharacterMovement()->bOrientRotationToMovement = bTurnedWithMovement;
	}
	Viewer.Reset();
}
