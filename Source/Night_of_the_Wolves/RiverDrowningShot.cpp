#include "RiverDrowningShot.h"
#include "FirstLegCrossingLayout.h"
#include "Night_of_the_WolvesCharacter.h"
#include "WolfTrailGameMode.h"
#include "WolfHeldItem.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "TwoBoneIK.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr double WaterZ = -209.; // top of the visible running-water sheets
constexpr auto WorldSpace = EBoneSpaces::WorldSpace;
}

ARiverDrowningShot::ARiverDrowningShot()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	GetCameraComponent()->FieldOfView=64.f;
	GetCameraComponent()->bConstrainAspectRatio=false;
	// A cool bounce catches the wet face against the shadowed far bank.
	auto* MoonBounce=CreateDefaultSubobject<UPointLightComponent>(TEXT("River moon bounce"));
	MoonBounce->SetupAttachment(GetRootComponent());
	MoonBounce->SetRelativeLocation(FVector(110,100,45));
	MoonBounce->SetIntensity(3400);
	MoonBounce->SetAttenuationRadius(520);
	MoonBounce->SetLightColor(FLinearColor(.55f,.69f,.9f));
	MoonBounce->SetCastShadows(false);
	Cry=CreateDefaultSubobject<UAudioComponent>(TEXT("River yell"));
	Cry->SetupAttachment(GetRootComponent());
	Cry->bAutoActivate=false;
	Cry->bAllowSpatialization=false;
	static ConstructorHelpers::FObjectFinder<USoundBase> Yell(TEXT("/Game/NightOfTheWolves/FirstLeg/Hermit/SC_HermitFallYell"));
	Cry->SetSound(Yell.Object);
	Cry->SetVolumeMultiplier(.86f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere"));
	for (int32 I=0; I<12; ++I)
	{
		auto* Crest=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Surface wake %d"),I));
		Crest->SetupAttachment(GetRootComponent());
		Crest->SetAbsolute(true,true,true);
		Crest->SetStaticMesh(Sphere.Object);
		Crest->SetCastShadow(false);
		Crest->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Crest->SetCanEverAffectNavigation(false);
		Crest->SetVisibility(false);
		Foam.Add(Crest);
	}
}

void ARiverDrowningShot::Start(AWolfTrailGameMode& Game, ACharacter& Player)
{
	Mode=&Game;
	Victim=&Player;
	StartLocation=Player.GetActorLocation();
	// Stage within the visible current even if the player overshot the edge of the bank.
	Entry.Y=FMath::Clamp(StartLocation.Y,-4850.,4050.);
	const double Middle=10350.+FirstLegLayout::StreamBend(Entry.Y);
	Entry.X=FMath::Clamp(StartLocation.X,Middle-170.,Middle+170.);
	Entry.Z=WaterZ;
	TowardCamera=FVector(-.77,.64,0).GetSafeNormal();
	Player.SetActorEnableCollision(false);
	if (auto* Gear=Player.FindComponentByClass<UWolfHeldItem>())
		Gear->SetHiddenInGame(true,true);
	auto* Body=Player.GetMesh();
	if (Body && Body->GetSkinnedAsset())
	{
		Swimmer=NewObject<UPoseableMeshComponent>(&Player);
		Swimmer->SetupAttachment(Body);
		Swimmer->SetSkinnedAssetAndUpdate(Body->GetSkinnedAsset());
		Swimmer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Swimmer->SetCanEverAffectNavigation(false);
		Swimmer->RegisterComponent();
		for (int32 I=0; I<Body->GetNumMaterials(); ++I) Swimmer->SetMaterial(I,Body->GetMaterial(I));
		Swimmer->CopyPoseFromSkeletalComponent(Body);
		if (auto* Max=Cast<ANight_of_the_WolvesCharacter>(&Player)) Max->BeginDigAppearance(Swimmer);
		Body->SetVisibility(false);
		Body->bPauseAnims=true;
	}
	if (auto* Tint=Foam[0]->CreateDynamicMaterialInstance(0))
	{
		Tint->SetVectorParameterValue(TEXT("Color"),FLinearColor(.26f,.38f,.43f));
		for (UStaticMeshComponent* Crest:Foam) Crest->SetMaterial(0,Tint);
	}
	SetActorLocation(Entry+TowardCamera*285.+FVector(0,0,100));
	SetActorRotation((Entry+FVector(0,0,32)-GetActorLocation()).Rotation());
	if (auto* PC=Cast<APlayerController>(Player.GetController()))
		PC->SetViewTargetWithBlend(this,.24f,VTBlend_Cubic,0,true);
	UE_LOG(LogTemp,Display,TEXT("RIVER plunge at CP%d into %s"),Game.Checkpoint,*Entry.ToCompactString());
}

void ARiverDrowningShot::AnimateSwimmer(float Beat, float Sink)
{
	// Pose the actual gameplay body. Its jacket follows the same arm and shoulder bones.
	if (!Swimmer) return;
	Swimmer->CopyPoseFromSkeletalComponent(Victim->GetMesh());
	const FVector Side=Victim->GetActorRightVector();
	const FVector Forward=Victim->GetActorForwardVector();
	const FVector Up=FVector::UpVector;
	auto Bend=[&](FName Bone,const FVector& Axis,float Angle)
	{
		const FQuat Base=Swimmer->GetBoneTransformByName(Bone,WorldSpace).GetRotation();
		Swimmer->SetBoneRotationByName(Bone,(FQuat(Axis,FMath::DegreesToRadians(Angle))*Base).Rotator(),WorldSpace);
	};
	Bend(TEXT("spine_02"),Side,-17.f+11.f*FMath::Sin(Beat));
	Bend(TEXT("neck_01"),Side,-13.f+9.f*FMath::Sin(Beat+.7f));
	for (int32 I=0; I<2; ++I)
	{
		const bool Left=I==0;
		const float Sign=Left?-1.f:1.f;
		const float A=Beat+(Left ? 0.f:2.65f);
		const float Reach=FMath::Sin(A);
		const FName Shoulder=Left ? TEXT("upperarm_l"):TEXT("upperarm_r");
		const FName Elbow=Left ? TEXT("lowerarm_l"):TEXT("lowerarm_r");
		const FName Hand=Left ? TEXT("hand_l"):TEXT("hand_r");
		auto Arm=Swimmer->GetBoneTransformByName(Shoulder,WorldSpace);
		auto Forearm=Swimmer->GetBoneTransformByName(Elbow,WorldSpace);
		auto Palm=Swimmer->GetBoneTransformByName(Hand,WorldSpace);
		const FVector Origin=Arm.GetLocation();
		const FVector Target=Origin+Up*(5.f+53.f*Reach+Sink*11.f)
			+Side*Sign*(37.f+16.f*FMath::Cos(A)) +Forward*(12.f-21.f*FMath::Cos(A));
		const FVector Pole=Origin+Side*Sign*55.f-Forward*28.+Up*5.;
		AnimationCore::SolveTwoBoneIK(Arm,Forearm,Palm,Pole,Target,false,1.,1.);
		Swimmer->SetBoneRotationByName(Shoulder,Arm.Rotator(),WorldSpace);
		Swimmer->SetBoneRotationByName(Elbow,Forearm.Rotator(),WorldSpace);
		Bend(Left?TEXT("thigh_l"):TEXT("thigh_r"),Side,22.f*Reach);
	}
	Swimmer->RefreshBoneTransforms();
	if (auto* Max=Cast<ANight_of_the_WolvesCharacter>(Victim))
		Max->FollowDigHead(Swimmer->GetSocketTransform(TEXT("head")));
}

void ARiverDrowningShot::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!IsValid(Victim) || !IsValid(Mode) || !Mode->IsRiverDrowning()) return;

#if !UE_BUILD_SHIPPING
	const float Previous=Elapsed;
#endif
	Elapsed+=Dt;
	const float T=FMath::Clamp(Elapsed/StruggleSeconds,0.f,1.f);
	const float Sink=FMath::SmoothStep(.67f,1.f,T);
	const float Beat=T*15.f;
	const FVector Stream=Entry+FVector(FMath::Sin(T*2.2)*22.,-T*257.,0);
	const double SurfaceFeet=WaterZ-107.-23.*FMath::Sin(Beat+.12)-113.*Sink;
	const FVector Center(Stream.X,Stream.Y,SurfaceFeet+Victim->GetSimpleCollisionHalfHeight());
	Victim->SetActorLocation(FMath::Lerp(StartLocation,Center,FMath::SmoothStep(0.f,.13f,T)));
	Victim->SetActorRotation(FRotator(6.f*FMath::Cos(Beat),TowardCamera.Rotation().Yaw+19.f*FMath::Sin(Beat*.47f),11.f*FMath::Sin(Beat)));
	AnimateSwimmer(Beat,Sink);
	if (Swimmer)
	{
		// The entry can capture a running leap, a deep falling crouch or the
		// cable release. Stage the actual face against the water in every pose.
		const double HeadTarget=WaterZ+40.+16.*FMath::Sin(Beat)-84.*Sink;
		const double HeadZ=Swimmer->GetSocketLocation(TEXT("head")).Z;
		Victim->AddActorWorldOffset(FVector(0,0,(HeadTarget-HeadZ)*FMath::SmoothStep(0.f,.13f,T)));
	}
	if (!bStartedCry && Elapsed>.16f)
	{
		bStartedCry=true;
		Cry->Play();
		UE_LOG(LogTemp,Display,TEXT("RIVER Wilhelm at %.2fs"),Elapsed);
	}

	for (int32 I=0; I<Foam.Num(); ++I)
	{
		const float Phase=FMath::Frac(T*3.8f+I*.381966f);
		const float Angle=I*2.4f;
		const FVector Out(FMath::Cos(Angle),FMath::Sin(Angle),0);
		const FVector Pos=Stream+Out*(22.f+Phase*56.f)+FVector(0,Phase*29.,0);
		const float Strength=FMath::Sin(PI*Phase)*(1.f-.8f*Sink);
		const bool bDroplet=(I%3)==0;
		UStaticMeshComponent* Crest=Foam[I];
		Crest->SetVisibility(Strength>.08f);
		Crest->SetWorldLocationAndRotation(FVector(Pos.X,Pos.Y,WaterZ+1.7+(bDroplet?Strength*15.:0.)),FRotator(0,Out.Rotation().Yaw+90,0));
		Crest->SetWorldScale3D(bDroplet ? FVector(.032,.035,.068)*Strength : FVector(.17+.13*Phase,.024,.012)*Strength);
	}
	const FVector Lens=Stream+TowardCamera*(281.f+31.f*T)+FVector(0,0,95.f+13.f*T);
	SetActorLocation(Lens);
	SetActorRotation((Stream+FVector(0,0,38.f-22.f*Sink)-Lens).Rotation());
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(),TEXT("RiverPhotos")))
	{
		const TCHAR* Stage=Previous<.78f && Elapsed>=.78f ? TEXT("River-Reach") :
			Previous<1.72f && Elapsed>=1.72f ? TEXT("River-Struggle") :
			Previous<StruggleSeconds-.28f && Elapsed>=StruggleSeconds-.28f ? TEXT("River-Sink"):nullptr;
		if (Stage && Swimmer)
		{
			UE_LOG(LogTemp,Display,TEXT("RIVER frame %s: head %s pawn %s"),Stage,*Swimmer->GetSocketLocation(TEXT("head")).ToCompactString(),*Victim->GetActorLocation().ToCompactString());
			const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("Saved/StormReview"));
			IFileManager::Get().MakeDirectory(*Folder,true);
			FScreenshotRequest::RequestScreenshot(Folder/(FString(Stage)+TEXT(".png")),false,false);
		}
	}
#endif
	if (T>=1.f)
	{
		UE_LOG(LogTemp,Display,TEXT("RIVER struggle complete %.2fs, immersion %.0fcm"),Elapsed,WaterZ-SurfaceFeet);
		SetActorTickEnabled(false);
		Mode->ShowDeathCard();
	}
}
