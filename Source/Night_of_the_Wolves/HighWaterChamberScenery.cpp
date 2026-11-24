#include "HighWaterChamber.h"
#include "HighWaterLayout.h"
#include "WolfTrailGameMode.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundWave.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FVector TroughPivot(535,2322,339);
const FVector WallSocket(251,1910,401);
const FVector FloatSocket(133,26,180);
template<class T> T* ChamberAsset(const FString& Path)
{
	ConstructorHelpers::FObjectFinder<T> Found(*Path);
	return Found.Object;
}
}

void AHighWaterChamber::BuildScenery()
{
	using namespace HighWaterLayout;
	auto Part = [this](FName Name, UStaticMesh* Shape, const FVector& At, USceneComponent* Parent=nullptr)
	{
		auto* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(Parent ? Parent : GetRootComponent());
		Mesh->SetRelativeLocation(At);
		Mesh->SetStaticMesh(Shape);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		return Mesh;
	};
	auto Sculpt = [&Part](const TCHAR* Name, const FVector& At, USceneComponent* Parent=nullptr)
	{
		return Part(Name,ChamberAsset<UStaticMesh>(FString(TEXT("/Game/NightOfTheWolves/HighWater/SM_"))+Name),At,Parent);
	};
	auto Solid = [this](FName Name, FVector Center, FVector Size, USceneComponent* Parent=nullptr)
	{
		auto* Box=CreateDefaultSubobject<UBoxComponent>(Name);
		Box->SetupAttachment(Parent ? Parent : GetRootComponent());
		Box->SetRelativeLocation(Center);
		Box->InitBoxExtent(Size*.5);
		Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Box->SetCanEverAffectNavigation(false);
		return Box;
	};
	auto Ramp = [&Solid](FName Name,FVector A,FVector B,double Width)
	{
		const FVector Span=B-A;
		const FRotator Grade=Span.Rotation();
		auto* Surface=Solid(Name,(A+B)*.5-Grade.RotateVector(FVector(0,0,16)),{Span.Size()+3,Width,32});
		Surface->SetRelativeRotation(Grade);
	};

	// The third-person boom can enter the sloped vault long before it reaches a walking blocker.
	auto* Vault=Sculpt(TEXT("SurveyCavern"),FVector::ZeroVector);
	Vault->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Vault->SetCollisionResponseToAllChannels(ECR_Ignore);
	Vault->SetCollisionResponseToChannel(ECC_Camera,ECR_Block);
	Ramp(TEXT("RisingCleft"),{540,910,22},{540,1710,264},275);
	Solid(TEXT("Apron"),{467,1760,244},{556,190,40});
	Solid(TEXT("DryShelf"),{367,2507,142},{394,1320,244});
	Solid(TEXT("BoardingTongue"),{709,2710,152},{306,210,224});
	Ramp(TEXT("StoneStairs"),{550,2005,264},{1217,2005,26},225);
	Solid(TEXT("SiltBed"),{885,2550,3},{1500,1640,46});
	Solid(TEXT("HighLanding"),{1412,2742,272},{480,306,520});
	Solid(TEXT("UpperGallery"),{1500,3118,277},{346,471,532});
	Solid(TEXT("UpperTunnelFloor"),{1527,3510,528},{263,333,30});
	Solid(TEXT("UpperBlindEnd"),{1490,3672,680},{313,30,330});
	Solid(TEXT("CleftWest"),{360,1301,257},{34,756,485});
	Solid(TEXT("CleftEast"),{723,1301,257},{34,756,485});
	Solid(TEXT("ChamberWest"),{127,2540,417},{42,1690,874});
	Solid(TEXT("ChamberEast"),{1678,2570,450},{47,1612,930});
	Solid(TEXT("LowBlindEnd"),{735,3345,407},{1310,39,803});
	Solid(TEXT("GalleryWest"),{1341,3518,669},{30,390,335});
	Solid(TEXT("GalleryEast"),{1703,3484,670},{37,397,327});
	Solid(TEXT("EndAtLip"),{1515,3348,245},{343,36,512});
	Solid(TEXT("FrontWest"),{247,1710,454},{163,48,452});
	Solid(TEXT("FrontEast"),{1265,1710,351},{1020,62,717});
	Solid(TEXT("CleftRoof"),{542,1300,627},{420,790,65})->SetRelativeRotation({0,0,17});
	Solid(TEXT("ChamberVault"),{915,2532,938},{1700,1700,88});

	auto* Slab = ChamberAsset<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube"));
	Scenery.Pool = Part(TEXT("BlackPool"),Slab,{1115,2579,HighWater-1.});
	Scenery.Pool->SetRelativeScale3D({11.0,14.83,.018});
	Scenery.Pool->SetMaterial(0,ChamberAsset<UMaterialInterface>(TEXT("/Game/NightOfTheWolves/HighWater/M_BlackPool")));
	Scenery.Pool->SetCastShadow(false);
	Scenery.Float = Sculpt(TEXT("SurveyFloat"),LooseFloat);
	Solid(TEXT("WorkDeck"),{0,0,-11.},{273,185,22},Scenery.Float);
	Scenery.Trough = Sculpt(TEXT("SpringTrough"),TroughPivot);
	Scenery.Timber = Sculpt(TEXT("WedgedTimber"),Pin+FVector(0,0,15));
	auto* Fork = Sculpt(TEXT("IronTorchFork"),WallSocket);
	auto* UpperFork = Part(TEXT("LadderTorchFork"),Fork->GetStaticMesh(),FloatSocket,Scenery.Float);
	UpperFork->SetRelativeRotation({0,165,0});

	Scenery.Brand = Sculpt(TEXT("SurveyBrand"),WallSocket);
	Scenery.Flame = Part(TEXT("PitchOnBrand"),ChamberAsset<UStaticMesh>(TEXT("/Game/NightOfTheWolves/Stormfall/SM_PitchFlame")),
		{0,0,32},Scenery.Brand);
	Scenery.Flame->SetRelativeScale3D({.56,.56,.65});
	Scenery.Flame->SetCastShadow(false);
	Scenery.TorchLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("BrandLight"));
	Scenery.TorchLight->SetupAttachment(Scenery.Brand);
	Scenery.TorchLight->SetMobility(EComponentMobility::Movable);
	Scenery.TorchLight->SetIntensityUnits(ELightUnits::Candelas);
	Scenery.TorchLight->SetRelativeLocation({3,0,55});
	Scenery.TorchLight->SetAttenuationRadius(1790.f);
	Scenery.TorchLight->SetLightColor({1.f,.47f,.18f});
	Scenery.TorchLight->SetSourceRadius(12.f);
	Scenery.TorchLight->SetCastShadows(true);
	Scenery.TorchLight->SetIntensity(0.f);
	// Diffuse firelight returning from the low vault opens the torch-side shadows.
	// The point follows the carried brand or its fork, including the final rise.
	Scenery.ReflectedFire=CreateDefaultSubobject<UPointLightComponent>(TEXT("TorchBounceFromVault"));
	Scenery.ReflectedFire->SetupAttachment(RootComponent);
	Scenery.ReflectedFire->SetMobility(EComponentMobility::Movable);
	Scenery.ReflectedFire->SetIntensityUnits(ELightUnits::Candelas);
	Scenery.ReflectedFire->SetSourceRadius(100.f);
	Scenery.ReflectedFire->SetAttenuationRadius(1310.f);
	Scenery.ReflectedFire->SetLightColor({.88f,.57f,.37f});
	Scenery.ReflectedFire->SetCastShadows(false);
	Scenery.ReflectedFire->SetVolumetricScatteringIntensity(.11f);
	Scenery.ReflectedFire->SetSpecularScale(.42f);
	Scenery.ReflectedFire->SetIntensity(0.f);
	Scenery.Stream = Sculpt(TEXT("FallingWater"),{534,2322,576});
	Scenery.Stream->SetRelativeScale3D({.56,.58,2.35});
	Scenery.Spill = Part(TEXT("SwungSpillway"),Scenery.Stream->GetStaticMesh(),TroughPivot);
	Scenery.Bypass = Sculpt(TEXT("BypassFlow"),FVector::ZeroVector);
	Scenery.Stream->SetCastShadow(false);
	Scenery.Spill->SetCastShadow(false);
	Scenery.Bypass->SetCastShadow(false);

	auto Sound = [this](const TCHAR* Name,const TCHAR* Asset,FVector Where,float Level,float Radius,float Falloff)
	{
		auto* Voice=CreateDefaultSubobject<UAudioComponent>(Name);
		Voice->SetupAttachment(RootComponent);
		Voice->SetRelativeLocation(Where);
		Voice->bAutoActivate=false;
		Voice->SetSound(ChamberAsset<USoundWave>(Asset));
		Voice->bOverrideAttenuation=true;
		Voice->AttenuationOverrides.AttenuationShapeExtents=FVector(Radius);
		Voice->AttenuationOverrides.FalloffDistance=Falloff;
		Voice->SetVolumeMultiplier(Level);
		return Voice;
	};
	Scenery.Spring=Sound(TEXT("RockSpring"),TEXT("/Game/NightOfTheWolves/Crossing/CreekCurrent"),TroughPivot,.32f,165.f,1550.f);
	Scenery.Seep=Sound(TEXT("DrainHiss"),TEXT("/Game/NightOfTheWolves/Stormfall/CaveSeep"),{1440,2300,45},.24f,145.f,1350.f);
	Scenery.Tap=Sound(TEXT("TappingTimber"),TEXT("/Game/NightOfTheWolves/HighWater/FloatTap"),LooseFloat,.74f,145.f,1320.f);
	Scenery.Grind=Sound(TEXT("WorkingWood"),TEXT("/Game/NightOfTheWolves/HighWater/CedarGroan"),Pin,.68f,125.f,1330.f);
}

void AHighWaterChamber::UpdateScenery(float Dt)
{
	using namespace HighWaterLayout;
	const float Level = FMath::Lerp(LowWater,HighWater,Water);
	const float Time = GetWorld()->GetTimeSeconds();
	Scenery.Pool->SetRelativeLocation({1115,2579,Level-1.f});
	Scenery.Float->SetRelativeLocation(FloatLocal);
	// The sill holds the far corner under water. The exposed ladder nudges the rock nose
	// on the return stroke, just as its short wooden report begins.
	const float Lift = bUnpinned ? 0.f : FMath::SmoothStep(.38f,.87f,Water);
	const float Contact = 1.f-FMath::SmoothStep(0.f,.72f,Scenery.TapIn);
	Scenery.Float->SetRelativeRotation({Lift*(2.8f+.9f*FMath::Sin(Time*2.8f)-2.5f*Contact),
		0.f,Lift*(1.2f*FMath::Sin(Time*1.9f)+1.8f*Contact)});
	Scenery.Timber->SetRelativeLocation(Pin+FVector(64.*TimberRoll,97.*TimberRoll,15.));
	Scenery.Timber->SetRelativeRotation({0.,0.,-151.*TimberRoll});
	const FRotator Swing(0.,FMath::Lerp(-31.,41.,double(TroughAngle)),0.);
	Scenery.Trough->SetRelativeRotation(Swing);
	const FVector Lip = TroughPivot+Swing.RotateVector({153.,0.,-16.});
	Scenery.Spill->SetRelativeLocation(Lip);
	Scenery.Spill->SetRelativeRotation(Swing);
	const double CatchesStream = FMath::SmoothStep(.72,1.,double(TroughAngle));
	const double Outflow = FMath::Lerp(FMath::Max(double(Level),32.),294.,CatchesStream);
	Scenery.Spill->SetRelativeScale3D({.8,.83,FMath::Max(.12,(Lip.Z-Outflow)/100.)});
	Scenery.Bypass->SetVisibility(CatchesStream>.28);

	const bool Mounted = TorchMount != EHighWaterTorch::Carried;
	Scenery.Brand->SetVisibility(Mounted,true);
	if (Mounted)
	{
		const bool AtWall = TorchMount == EHighWaterTorch::Wall;
		const FVector Socket = AtWall ? WallSocket+FVector(18,0,1) : FloatLocal+FloatSocket+FVector(-10,4,0);
		Scenery.Brand->SetRelativeLocation(Socket);
		const float Flicker=1.f+.055f*FMath::Sin(Time*8.7f)+.035f*FMath::Sin(Time*16.8f+.83f);
		const float Strength=AtWall ? 18.f : FMath::Lerp(17.f,31.f,Water);
		Scenery.TorchLight->SetIntensity(Strength*Flicker);
	}
	else Scenery.TorchLight->SetIntensity(0.f);
	const auto* Mode = GetWorld()->GetAuthGameMode<AWolfTrailGameMode>();
	const auto* PC=GetWorld()->GetFirstPlayerController();
	const APawn* Max=PC ? PC->GetPawn() : nullptr;
	if (Max && Mode && Mode->Checkpoint>=6 && (Mounted || Mode->HasTreeFlame()))
	{
		const FVector Origin=Mounted ? Scenery.Brand->GetRelativeLocation()+FVector(0,0,65) :
			GetActorTransform().InverseTransformPosition(Max->GetActorLocation())+FVector(0,0,55);
		const FVector Bounce=Mounted ? FMath::Lerp(Origin,FVector(784,2360,Origin.Z+160),.31) :
			Origin+FVector(5,34,47);
		Scenery.ReflectedFire->SetRelativeLocation(Bounce);
		Scenery.ReflectedFire->SetIntensity(Mounted ? 5.2f : 8.f);
	}
	else Scenery.ReflectedFire->SetIntensity(0.f);

	if (Dt <= 0.f)
	{
		Scenery.Spring->Stop();
		Scenery.Seep->Stop();
		Scenery.Grind->Stop();
		Scenery.Tap->Stop();
		Scenery.TapIn=1.4f;
		Scenery.bTurning=Scenery.bRolling=false;
		return;
	}
	if (!Scenery.Spring->IsPlaying()) Scenery.Spring->Play();
	if (!Scenery.Seep->IsPlaying()) Scenery.Seep->Play();
	Scenery.Spring->SetRelativeLocation(Lip);
	Scenery.Spring->SetVolumeMultiplier(.24f+.15f*TroughAngle);
	Scenery.Seep->SetVolumeMultiplier(.12f+.21f*TroughAngle);
	const bool Turning=TroughAngle>.001f && TroughAngle<.999f;
	const bool Rolling=TimberRoll>.001f && TimberRoll<.999f;
	if ((Turning && !Scenery.bTurning) || (Rolling && !Scenery.bRolling))
	{
		Scenery.Grind->SetRelativeLocation(Rolling ? Pin : TroughPivot);
		Scenery.Grind->SetPitchMultiplier(Rolling ? .77f : 1.06f);
		Scenery.Grind->Play();
	}
	Scenery.bTurning=Turning;
	Scenery.bRolling=Rolling;
	Scenery.TapIn -= Dt;
	if (Scenery.TapIn <= 0.f && Water>.68f)
	{
		Scenery.Tap->SetRelativeLocation(FloatLocal+FVector(73,0,21));
		Scenery.Tap->SetPitchMultiplier(.94f+.09f*FMath::Sin(Time));
		Scenery.Tap->Play();
		Scenery.TapIn=(bGuided ? 4.2f : 2.75f)+.44f*FMath::Sin(Time*1.7f);
	}
}
