#include "StormfallSet.h"
#include "StormfallLayout.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "WolfTrailGameMode.h"
#include "WolfPackComponent.h"
#include "WolfMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundWave.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
template<class T> T* Load(const TCHAR* Path)
{
	ConstructorHelpers::FObjectFinder<T> Found(Path);
	return Found.Object;
}
const FVector Stump(-875,-253,-33);
const FVector Hinge = Stump+FVector(0,0,95);
const FQuat FallenRotation(FVector::RightVector,FMath::DegreesToRadians(87.5));
}

AStormfallSet::AStormfallSet()
{
	PrimaryActorTick.bCanEverTick=true;
	RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Stormfall"));
	StrikeCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("Strike camera"));
	StrikeCamera->SetupAttachment(RootComponent);
	StrikeCamera->FieldOfView=82.f;
	StrikeCamera->SetRelativeLocation({-917,1334,535});
	auto Mesh=[this](FName Name,UStaticMesh* Shape,FVector Position,USceneComponent* Parent=nullptr)
	{
		auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent ? Parent : RootComponent.Get());
		Part->SetStaticMesh(Shape);
		Part->SetRelativeLocation(Position);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		return Part;
	};
	auto Block=[this](FName Name,FVector Center,FVector HalfSize,FRotator Rotation=FRotator::ZeroRotator)
	{
		auto* Shape=CreateDefaultSubobject<UBoxComponent>(Name);
		Shape->SetupAttachment(RootComponent);
		Shape->SetRelativeLocationAndRotation(Center,Rotation);
		Shape->InitBoxExtent(HalfSize);
		Shape->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Shape->SetCanEverAffectNavigation(false);
		return Shape;
	};
	auto Light=[this](FName Name,FVector Point,FLinearColor Color,float Radius)
	{
		auto* Lamp=CreateDefaultSubobject<UPointLightComponent>(Name);
		Lamp->SetupAttachment(RootComponent);
		Lamp->SetRelativeLocation(Point);
		Lamp->SetLightColor(Color);
		Lamp->SetAttenuationRadius(Radius);
		Lamp->SetCastShadows(true);
		Lamp->SetIntensity(0.f);
		return Lamp;
	};
	auto Sound=[this](const TCHAR* Name,const FVector& Point,float Radius,float Falloff,float Level)
	{
		auto* Voice=CreateDefaultSubobject<UAudioComponent>(Name);
		Voice->SetupAttachment(RootComponent);
		Voice->SetRelativeLocation(Point);
		Voice->bAutoActivate=false;
		Voice->bOverrideAttenuation=true;
		Voice->AttenuationOverrides.AttenuationShapeExtents=FVector(Radius);
		Voice->AttenuationOverrides.FalloffDistance=Falloff;
		Voice->SetVolumeMultiplier(Level);
		Voice->SetSound(Load<USoundWave>(*FString::Printf(TEXT("/Game/NightOfTheWolves/Stormfall/%s"),Name)));
		return Voice;
	};
	Mesh(TEXT("RivenBasaltBluff"),Load<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Stormfall/SM_RivenBluff")),FVector::ZeroVector);
	BuildFractureFaces();
	// The sloping dirt fans meet the logging tread without an edge for the capsule to catch.
	auto Floor=[&](FName Name,FVector A,FVector B,float HalfWidth)
	{
		const FVector Span=B-A;
		const FRotator Grade=Span.Rotation();
		Block(Name,(A+B)*.5-Grade.RotateVector(FVector(0,0,17)),{Span.Size()*.5+1.5,HalfWidth,17},Grade);
	};
	Floor(TEXT("FanToForest"),{-650,0,-45},{0,0,0},440);
	Floor(TEXT("VestibuleTread"),{0,0,0},{847,0,14},263);
	Floor(TEXT("SideCleftTread"),{500,180,8},{537,929,23},185);
	Block(TEXT("BluffWestFace"),{9,1287,206},{91,1051,230});
	Block(TEXT("BluffEastFace"),{12,-1150,206},{97,922,230});
	Block(TEXT("LeftBeddingPlane"),{387,-265,158},{461,36,177});
	Block(TEXT("RightBeddingPlane"),{146,263,155},{199,30,175});
	Block(TEXT("RightBlindCorner"),{753,225,155},{95,34,169});
	Block(TEXT("RockfallAcrossMainBore"),{795,-3,139},{70,243,155});
	Block(TEXT("UndersideOfOverhang"),{401,0,371},{490,288,43});
	Block(TEXT("SideCleftLeftWall"),{331,575,145},{25,343,157});
	Block(TEXT("SideCleftRightWall"),{691,576,146},{34,347,163});
	Block(TEXT("SideCleftRoof"),{505,565,304},{181,358,40});
	Block(TEXT("BeyondFirstChamber"),{540,944,141},{191,32,175});

	RootBlock=Block(TEXT("MattedRootCollision"),{499,232,137},{166,17,134});
	for (int32 I=0; I<5; ++I)
	{
		UStaticMesh* Fibres=Load<UStaticMesh>(*FString::Printf(TEXT("/Game/NightOfTheWolves/Stormfall/SM_ExposedRoots%d"),I%3));
		auto* Strand=Mesh(*FString::Printf(TEXT("CeilingRoots%d"),I),Fibres,{365.+I*66.,229.+(I%3)*5.,250.+(I%2)*5.});
		RootStrands.Add(Strand);
	}

	Mesh(TEXT("SplitStump"),Load<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Stormfall/SM_SplitFirStump")),Stump);
	Block(TEXT("StumpCore"),Stump+FVector(0,0,57),{42,42,58});
	FallingTree=CreateDefaultSubobject<USceneComponent>(TEXT("BreakingHinge"));
	FallingTree->SetupAttachment(RootComponent);
	FallingTree->SetRelativeLocation(Hinge);
	auto* Fir=Load<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Cascades/SM_WesternFir/StaticMeshes/SM_WesternFir"));
	Mesh(TEXT("OldFirBole"),Load<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Stormfall/SM_OldFirBole")),FVector::ZeroVector,FallingTree);
	HeroFir=Mesh(TEXT("LightningStruckFir"),Fir,FVector::ZeroVector,FallingTree);
	if (Fir)
	{
		const FBox Bounds=Fir->GetBoundingBox();
		const double Height=FMath::Max(1.,Bounds.GetSize().Z);
		const double Scale=701./Height;
		HeroFir->SetRelativeScale3D({Scale*.90,Scale*.90,Scale});
		HeroFir->SetRelativeLocation({0,0,718.-Bounds.Min.Z*Scale});
	}
	BurnMarks=Mesh(TEXT("ResinousSplitLimb"),Load<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Stormfall/SM_ResinLimb")),FVector::ZeroVector,FallingTree);
	LogBlock=Block(TEXT("FallenTrunkAndCrown"),{-175,-253,140},{742,105,181});
	Bolt=Mesh(TEXT("LightningChannel"),Load<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Stormfall/SM_LightningChannel")),Hinge+FVector(0,0,1405));
	Bolt->SetCastShadow(false);
	Flash=Light(TEXT("DischargeFlash"),Hinge+FVector(0,0,935),{.69f,.80f,1.f},6900.f);
	Flash->SourceRadius=142.f;
	FireLight=Light(TEXT("LimbFirelight"),{-462,-98,81},{1.f,.29f,.061f},1150.f);
	FireLight->SourceRadius=52.f;
	EmberLight=Light(TEXT("RootFirelight"),{489,189,142},{1.f,.23f,.044f},535.f);
	EmberLight->SourceRadius=24.f;
	auto* Flame=Load<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Stormfall/SM_PitchFlame"));
	auto* Cinder=Load<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere"));
	auto* Ember=Load<UMaterialInterface>(TEXT("/Game/NightOfTheWolves/Presentation/M_FireGlow"));
	for (int32 I=0; I<24; ++I)
	{
		// Put the first flames on the reachable split branch, the rest along the trunk.
		const FVector Base = I<8 ? Hinge+FallenRotation.RotateVector(FVector(9.,37.+I*21.,362.+I*10.)):
			Hinge+FallenRotation.RotateVector(FVector(-17.+9.*FMath::Sin(I*1.8),28.*FMath::Sin(I*1.1),72.+(I-8)*58.));
		FireAnchors.Add(Base);
		auto* Tongue=Mesh(*FString::Printf(TEXT("PitchFlame%d"),I),Flame,Base);
		Tongue->SetCastShadow(false);
		Flames.Add(Tongue);
		if (I>=16) continue;
		auto* Spark=Mesh(*FString::Printf(TEXT("DriftingEmber%d"),I),Cinder,Base);
		Spark->SetMaterial(0,Ember);
		Spark->SetCastShadow(false);
		Sparks.Add(Spark);
	}
	for (int32 I=0; I<14; ++I)
	{
		auto* Tongue=Mesh(*FString::Printf(TEXT("BurningFibres%d"),I),Flame,FVector::ZeroVector);
		Tongue->SetCastShadow(false);
		RootEmbers.Add(Tongue);
	}

	Thunder=Sound(TEXT("ThunderRoll"),Hinge+FVector(0,0,800),760,6400,.94f);
	Break=Sound(TEXT("FirBreak"),Hinge,430,2900,.89f);
	Crash=Sound(TEXT("CrownImpact"),{-203,-248,25},480,3650,.98f);
	Crackle=Sound(TEXT("PitchSnap"),{-460,-95,63},220,1220,.60f);
	RootsBurn=Sound(TEXT("RootsCatching"),{497,231,125},105,690,.78f);
	CaveDrips=Sound(TEXT("CaveSeep"),{583,525,121},76,830,.16f);
}

void AStormfallSet::BuildFractureFaces()
{
	// Broken blocks of different heights interrupt continuous bedding planes.
	// They share the bank's scanned rock surfaces and remain outside the tread.
	FRandomStream Random(60316);
	TArray<UInstancedStaticMeshComponent*> Beds;
	const TCHAR* Names[]={TEXT("SM_CreekStoneA"),TEXT("SM_CreekStoneB"),TEXT("SM_CreekStoneC")};
	for (const TCHAR* Name : Names)
	{
		auto* Pieces=CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Pieces->SetupAttachment(RootComponent);
		Pieces->SetStaticMesh(Load<UStaticMesh>(*FString::Printf(TEXT("/Game/NightOfTheWolves/Cascades/%s/StaticMeshes/%s"),Name,Name)));
		Pieces->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pieces->SetGenerateOverlapEvents(false);
		Pieces->SetCanEverAffectNavigation(false);
		Beds.Add(Pieces);
	}
	auto Slab=[&](const FVector& Center, const FVector& Dimensions, float Yaw)
	{
		auto* Bed=Beds[Random.RandRange(0,Beds.Num()-1)];
		if (!Bed->GetStaticMesh()) return;
		const FBox Bounds=Bed->GetStaticMesh()->GetBoundingBox();
		const FVector Scale=Dimensions/Bounds.GetSize().ComponentMax(FVector(1));
		const FQuat Rotation=FRotator(Random.FRandRange(-17.f,17.f),Yaw+Random.FRandRange(-14.f,14.f),Random.FRandRange(-14.f,14.f)).Quaternion();
		Bed->AddInstance(FTransform(Rotation,Center-Rotation.RotateVector(Scale*Bounds.GetCenter()),Scale));
	};
	for (double Side : {-1.,1.})
	{
		for (int32 Along=0; Along<10; ++Along)
		{
			const float X=-37.f+Along*87.f+Random.FRandRange(-14.f,14.f);
			if (Side>0 && X>300 && X<673) continue;
			for (int32 Lift=0; Lift<3; ++Lift)
			{
				const double Height=45.+Lift*87.+Random.FRandRange(-18.f,18.f);
				Slab({X,Side*(248.-21.*Lift),Height},{Random.FRandRange(137,208),Random.FRandRange(64,111),Random.FRandRange(107,169)},6.f);
			}
		}
		for (int32 Along=0; Along<9; ++Along)
		{
			const float Y=208.f+Along*82.f+Random.FRandRange(-16.f,16.f);
			const float Center=498.f+(Y-208.f)*.088f;
			for (int32 Lift=0; Lift<3; ++Lift)
			{
				const double Height=46.+Lift*85.+Random.FRandRange(-20.f,20.f);
				Slab({Center+Side*(181.-Lift*14.),Y,Height},{Random.FRandRange(127,207),Random.FRandRange(60,104),Random.FRandRange(112,157)},88.f);
			}
		}
	}
}

void AStormfallSet::BeginPlay()
{
	Super::BeginPlay();
	Restore(false,false);
	CaveDrips->Play();
}

void AStormfallSet::FrameRunningPack(bool bCut)
{
	auto* PC=GetWorld()->GetFirstPlayerController();
	auto* Mode=GetWorld()->GetAuthGameMode<AWolfTrailGameMode>();
	auto* Pack=Mode ? Mode->GetHuntingPack() : nullptr;
	if (!PC || PC->GetViewTarget()!=this || !Pack) return;
	if (bCut)
	{
		FramedWolves.Reset();
		auto* Leader=Pack->GetLeadWolf();
		if (!Leader) return;
		const FVector Feet=Leader->GetComponentLocation();
		for (int32 I=0; I<Pack->Count; ++I)
		{
			auto* Wolf=Pack->GetMember(I);
			if (Wolf && FVector::DistSquared2D(Feet,Wolf->GetComponentLocation())<FMath::Square(790.))
				FramedWolves.Add(I);
		}
	}
	if (FramedWolves.IsEmpty()) return;
	FBox Group(ForceInit);
	for (int32 I : FramedWolves)
		if (auto* Wolf=Pack->GetMember(I)) Group+=Wolf->GetComponentLocation();
	const FVector Center=Group.GetCenter();
	if (bCut)
	{
		FirstPackCenter=Center;
		const float Span=FMath::Clamp(float(Group.GetExtent().Size2D()),95.f,510.f);
		ReactionOffset=FVector(-168.-Span*.40,350.+Span*1.03,124.+Span*.28);
		StrikeCamera->FieldOfView=74.f;
		if (PC->PlayerCameraManager) PC->PlayerCameraManager->SetGameCameraCutThisFrame();
	}
	const FVector Eye=FirstPackCenter+(Center-FirstPackCenter)*.83+ReactionOffset;
	const FVector Aim=Center+FVector(-47,0,49);
	StrikeCamera->SetWorldLocationAndRotation(Eye,(Aim-Eye).Rotation());
}

void AStormfallSet::EndStrikeShot(float Blend)
{
	auto* PC=GetWorld()->GetFirstPlayerController();
	if (PC && PC->GetViewTarget()==this)
	{
		AActor* Return=PreviousView.IsValid() ? PreviousView.Get() : PC->GetPawn();
		if (Return) PC->SetViewTargetWithBlend(Return,Blend,VTBlend_Cubic,0.f,true);
	}
	PreviousView.Reset();
}

void AStormfallSet::Restore(bool bFallen,bool bRootsCleared)
{
	EndStrikeShot(0.f);
	FramedWolves.Reset();
	for (UAudioComponent* Cue : {Thunder.Get(),Break.Get(),Crash.Get(),RootsBurn.Get(),Crackle.Get()}) Cue->Stop();
	PreviousStorm=bFallen ? StormfallLayout::Settled : -1.f;
	bFire=bFallen;
	FallingTree->SetRelativeRotation(bFallen ? FallenRotation : FQuat::Identity);
	Bolt->SetVisibility(false);
	Flash->SetIntensity(0.f);
	LogBlock->SetCollisionEnabled(bFallen ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Burn=bRootsCleared ? 1.f : 0.f;
	SetPuzzlePose(Burn);
	if (bFire) Crackle->Play();
	Tick(0.f);
}

void AStormfallSet::SetStormPose(float T)
{
	using namespace StormfallLayout;
	if (T<0 || T<PreviousStorm) { Restore(false,Burn>=1.f); if (T<0) return; }
	if (T<ReactionCut)
	{
		const FVector Eye(-917,1334,535);
		const float Tilt=FMath::SmoothStep(.44f,GroundImpact+.12f,T);
		const FVector Subject=FMath::Lerp(FVector(-807,-230,1040),FVector(-291,-219,143),Tilt);
		StrikeCamera->FieldOfView=82.f;
		StrikeCamera->SetRelativeLocationAndRotation(Eye,(Subject-Eye).Rotation());
	}
	if (PreviousStorm<0.f)
	{
		if (auto* PC=GetWorld()->GetFirstPlayerController(); PC && PC->GetPawn() && PC->GetViewTarget()==PC->GetPawn())
		{
			PreviousView=PC->GetViewTarget();
			PC->SetViewTarget(this);
			if (PC->PlayerCameraManager) PC->PlayerCameraManager->SetGameCameraCutThisFrame();
		}
	}
	if (T>=ReactionCut && T<Settled) FrameRunningPack(PreviousStorm<ReactionCut);
	if (T>=Settled && PreviousStorm<Settled) EndStrikeShot(.40f);
	const float Main=1.f-FMath::SmoothStep(.045f,.125f,T);
	const float Return=FMath::SmoothStep(.175f,.205f,T)*(1.f-FMath::SmoothStep(.225f,.39f,T))*.67f;
	const float Discharge=Main+Return;
	Bolt->SetVisibility(Discharge>.045f);
	Flash->SetIntensity(Discharge*630000.f);
	if (PreviousStorm<.08f && T>=.08f) Thunder->Play();
	if (PreviousStorm<FallBegins && T>=FallBegins) Break->Play();
	const float Swing=FMath::Clamp((T-FallBegins)/(GroundImpact-FallBegins),0.f,1.f);
	const float SinceHit=FMath::Max(0.f,T-GroundImpact);
	const float Recoil=SinceHit<.56f ? 4.9f*FMath::Sin(SinceHit/.56f*2*PI)*FMath::Exp(-SinceHit*7.7f) : 0.f;
	FallingTree->SetRelativeRotation(FQuat(FVector::RightVector,FMath::DegreesToRadians(87.5f*FMath::Pow(Swing,1.85f)-Recoil)));
	if (PreviousStorm<GroundImpact && T>=GroundImpact)
	{
		Crash->Play();
		Crackle->Play();
		LogBlock->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		bFire=true;
	}
	PreviousStorm=T;
}

void AStormfallSet::SetPuzzlePose(float Fraction)
{
	const float Progress=FMath::Clamp(Fraction,0.f,1.f);
	if (Burn<=0 && Progress>0 && Progress<1) RootsBurn->Play();
	Burn=Progress;
	RootBlock->SetCollisionEnabled(Burn>=1.f ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	for (int32 I=0; I<RootStrands.Num(); ++I)
	{
		const float Delay=.055f*FMath::Abs(I-2);
		const float Char=FMath::SmoothStep(Delay,1.f,Burn);
		RootStrands[I]->SetVisibility(Burn<1.f);
		RootStrands[I]->SetRelativeScale3D({1.-.19*Char,1.,1.-.988*Char});
		RootStrands[I]->SetRelativeRotation(FRotator(0,0,(I%2 ? 1.f : -1.f)*14.f*Char));
	}
}

void AStormfallSet::Tick(float Dt)
{
	Super::Tick(Dt);
	const float T=GetWorld()->GetTimeSeconds();
	for (int32 I=0; I<Flames.Num(); ++I)
	{
		auto* Tongue=Flames[I].Get();
		Tongue->SetVisibility(bFire);
		if (!bFire) continue;
		const float Pulse=1.f+.11f*FMath::Sin(T*7.7f+I*1.5f)+.09f*FMath::Sin(T*15.1f+I);
		const float Height=(I<8 ? .56f+.16f*(I%3) : .68f+.28f*(I%4))*Pulse;
		Tongue->SetRelativeScale3D({Height,Height,Height});
		Tongue->SetRelativeRotation(FRotator(5.f*FMath::Sin(T*3.3f+I),I*47.f,4.f*Pulse));
	}
	for (int32 I=0; I<Sparks.Num(); ++I)
	{
		auto* Spark=Sparks[I].Get();
		Spark->SetVisibility(bFire);
		if (!bFire) continue;
		const float Age=FMath::Frac(T*(.49f+(I%3)*.07f)+I*.261f);
		Spark->SetRelativeLocation(FireAnchors[(I*7)%FireAnchors.Num()]+FVector(37.f*Age+14.f*FMath::Sin(I+Age*7.f),26.f*Age,18.f+Age*194.f));
		Spark->SetRelativeScale3D(FVector(.029f,.021f,.042f)*(1.-Age));
	}
	FireLight->SetIntensity(bFire ? 7250.f*(1.f+.08f*FMath::Sin(T*10.1f)+.07f*FMath::Sin(T*17.4f)) : 0.f);
	const bool bRootFire=Burn>0.f && Burn<1.f;
	for (int32 I=0; I<RootEmbers.Num(); ++I)
	{
		auto* Ember=RootEmbers[I].Get();
		const float Drift=FMath::Frac(I*.257f+T*.63f);
		const float Envelope=FMath::SmoothStep(0.f,.14f,Burn)*(1.f-FMath::SmoothStep(.78f,1.f,Burn));
		Ember->SetVisibility(bRootFire);
		if (!bRootFire) continue;
		const float Spread=FMath::Min(1.f,Burn*3.f);
		Ember->SetRelativeLocation({498.+((I%7)-3)*42.*Spread,217.+(I%4)*3.,31.+Burn*198.+36.*Drift});
		Ember->SetRelativeScale3D(FVector(.28f+.10f*(I%3),.32f,.44f+.18f*Drift)*Envelope);
	}
	EmberLight->SetIntensity(bRootFire ? 1350.f*FMath::Sin(Burn*PI)*(1.f+.12f*FMath::Sin(T*12.3f)) : 0.f);
}
