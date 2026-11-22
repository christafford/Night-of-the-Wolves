#include "WolfCottageSet.h"
#include "WolfPackComponent.h"
#include "WolfTrailRoute.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AWolfCottageSet::AWolfCottageSet()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Cottage")));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BoxAsset(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BallAsset(TEXT("/Engine/BasicShapes/Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TubeAsset(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Shape(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/NightOfTheWolves/Presentation/M_TreeBark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mud(TEXT("/Game/NightOfTheWolves/Presentation/M_ForestGround"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Ember(TEXT("/Game/NightOfTheWolves/Presentation/M_FireGlow"));
	Cube=BoxAsset.Object; Ball=BallAsset.Object; Tube=TubeAsset.Object; ShapeMaterial=Shape.Object;
	Bark=Wood.Object; Soil=Mud.Object; Fire=Ember.Object;
	Pack = CreateDefaultSubobject<UWolfPackComponent>(TEXT("HuntingPack"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Impact(TEXT("/Game/NightOfTheWolves/FirstLeg/DoorImpact"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Growl(TEXT("/Game/NightOfTheWolves/FirstLeg/Growl"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Hearth(TEXT("/Game/NightOfTheWolves/FirstLeg/Hearth"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Stone(TEXT("/Game/NightOfTheWolves/Presentation/M_CabinStone"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Warning(TEXT("/Game/NightOfTheWolves/FirstLeg/Voices/HermitWarning"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Shout(TEXT("/Game/NightOfTheWolves/FirstLeg/Voices/HermitShout"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Trapped(TEXT("/Game/NightOfTheWolves/FirstLeg/Voices/HermitPit"));
	Masonry=Stone.Object;
	WarningVoice=Warning.Object; ShoutVoice=Shout.Object; TrappedVoice=Trapped.Object;
	ImpactSound=Impact.Object; GrowlSound=Growl.Object; FireSound=Hearth.Object;
}

void AWolfCottageSet::BeginPlay()
{
	Super::BeginPlay();
	Nearby=NewObject<USoundAttenuation>(this);
	Nearby->Attenuation.AttenuationShapeExtents=FVector(380,0,0);
	Nearby->Attenuation.FalloffDistance=1550;
	Nearby->Attenuation.bSpatialize=true;
	Nearby->Attenuation.bAttenuate=true;
	VoiceAudio=NewObject<UAudioComponent>(this);
	VoiceAudio->SetupAttachment(GetRootComponent());
	VoiceAudio->bAutoActivate=false;
	VoiceAudio->AttenuationSettings=Nearby;
	VoiceAudio->RegisterComponent();
	if (FireSound) UGameplayStatics::SpawnSoundAtLocation(this,FireSound,FVector(3065,220,80),FRotator::ZeroRotator,.35f,1,0,Nearby);
	auto ActorsWith=[this](FName Tag,TArray<TObjectPtr<AActor>>& Dest)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(this,Tag,Found);
		for (AActor* A:Found) Dest.Add(A);
	};
	ActorsWith(TEXT("M2Door"),DoorActors); ActorsWith(TEXT("M3PitCover"),PitCover);
	ActorsWith(TEXT("M3Shovel"),Shovel); ActorsWith(TEXT("M2Hermit"),OldHermit);
	for (AActor* A:OldHermit) { A->SetActorHiddenInGame(true); A->SetActorEnableCollision(false); }
	auto Dye=[this](FLinearColor Color)
	{
		auto* M=UMaterialInstanceDynamic::Create(ShapeMaterial,this);
		M->SetVectorParameterValue(TEXT("Color"),Color);
		return M;
	};
	Gold=Dye(FLinearColor(.78,.43,.07)); Cloth=Dye(FLinearColor(.14,.17,.13));
	Iron=Dye(FLinearColor(.085,.09,.10)); RopeMat=Dye(FLinearColor(.51,.34,.16));
	BuildProps();
	BuildRoof();
}

UStaticMeshComponent* AWolfCottageSet::Piece(UStaticMesh* Mesh,UMaterialInterface* Mat,FVector P,FVector Scale,FRotator R,USceneComponent* Parent,bool bSolid)
{
	auto* C=NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetupAttachment(Parent ? Parent : GetRootComponent());
	C->SetStaticMesh(Mesh);
	if (Mat) C->SetMaterial(0,Mat);
	C->SetRelativeTransform(FTransform(R,P,Scale));
	C->SetCollisionProfileName(bSolid ? TEXT("BlockAll") : TEXT("NoCollision"));
	C->SetGenerateOverlapEvents(false);
	C->RegisterComponent();
	return C;
}
UStaticMeshComponent* AWolfCottageSet::Stick(FVector A,FVector B,float Width,UMaterialInterface* Mat,USceneComponent* Parent,bool bSolid)
{
	const FVector D=B-A;
	return Piece(Tube,Mat,(A+B)*.5,FVector(Width*.01,Width*.01,D.Size()*.01),FRotationMatrix::MakeFromZ(D).Rotator(),Parent,bSolid);
}
void AWolfCottageSet::BuildProps()
{
	// Leave a broad gap for the capsule on the main path. The pack takes the long way around.
	Stick(FVector(850,-1620,30),FVector(990,-1055,52),90,Bark,nullptr,true);
	Stick(FVector(1010,-740,41),FVector(1180,-255,70),92,Bark,nullptr,true);
	Stick(FVector(930,-1250,45),FVector(835,-1280,180),24,Bark);
	Stick(FVector(1100,-500,45),FVector(1200,-460,150),21,Bark);

	Piece(Cube,Bark,FVector(2800,-235,77),FVector(1.6,.85,.12));
	for (int X : {-60,60}) for (int Y : {-28,28})
		Piece(Cube,Bark,FVector(2800+X,-235+Y,37),FVector(.10,.11,.74));
	// Stacked polygonal loops keep the open coil recognizable from above.
	for (int Ring=0;Ring<3;Ring++) for (int I=0;I<14;I++)
	{
		const float A=2*PI*I/14.f,B=2*PI*(I+1)/14.f;
		const float Radius=23-Ring*3;
		const FVector Center(2760,-225,86+Ring*2.5);
		TableGoods.Add(Stick(Center+FVector(Radius*FMath::Cos(A),Radius*FMath::Sin(A),0),Center+FVector(Radius*FMath::Cos(B),Radius*FMath::Sin(B),0),4.2,RopeMat));
	}
	TableGoods.Add(Stick(FVector(2780,-223,89),FVector(2785,-191,74),4.2,RopeMat));
	TableGoods.Add(Piece(Ball,Cloth,FVector(2835,-233,95),FVector(.42,.36,.27)));
	TableGoods.Add(Piece(Tube,RopeMat,FVector(2835,-233,104),FVector(.20,.20,.055)));
	for(int I=0;I<6;I++)
		TableGoods.Add(Piece(Tube,Gold,FVector(2818+9*FMath::Cos(I*1.9),-214+15*FMath::Sin(I*1.9),86+I%2),FVector(.08,.08,.016)));
	// A soot-dark firebox, jambs and a deep lintel give the glowing wood a recess.
	// Preserve the mouth of the hearth for the hand-held brand interaction.
	Piece(Cube,Iron,FVector(3107,220,84),FVector(.10,1.5,1.1));
	for(int Side : {-1,1})
		for(int Row=0;Row<4;Row++)
			Piece(Cube,Masonry,FVector(3084,220+Side*82,43+Row*29),
				FVector(.66,.27,.275),FRotator(0,Side*(Row%2),0));
	Piece(Cube,Masonry,FVector(3080,220,154),FVector(.83,1.90,.32));
	Piece(Cube,Bark,FVector(3074,220,176),FVector(.98,2.13,.13));
	Stick(FVector(3022,170,40),FVector(3080,254,43),16,Bark);
	Stick(FVector(3018,259,42),FVector(3073,179,52),14,Bark);
	for(int I=0;I<8;I++)
		Piece(Ball,Fire,FVector(3037+10*FMath::Cos(static_cast<float>(I)),182+I*10,41+I%3),
			FVector(.08,.10,.06));
	Brand.Add(Stick(FVector(2973,192,51),FVector(3058,216,72),11,Bark));
	Brand.Add(Piece(Ball,Fire,FVector(3058,216,74),FVector(.15,.14,.22)));
	// The six warped planks span the same 120 x 160 cm opening as the physical cover.
	// Small gaps, broken ends and iron nail heads keep the rotten patch readable underfoot.
	const float Lengths[] = {143,155,143,136,153,145};
	for (int I=0; I<6; ++I)
	{
		const FVector Centre(2211+I*19.5, (I%2 ? -5 : 5), 2.3f + (I%3)*.32f);
		auto* Board = Piece(Cube,Bark,Centre,FVector(.17f,Lengths[I]/100.f,.035f),FRotator(0,(I%3-1)*2.5f,0));
		WeakPlanks.Add(Board);
		for (int End : {-1,1})
		{
			auto* Nail = Piece(Tube,Iron,FVector::ZeroVector,FVector::OneVector);
			// Preserve the nail proportions independently of the thin board's nonuniform scale.
			Nail->SetAbsolute(false,false,true);
			Nail->AttachToComponent(Board,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			Nail->SetRelativeLocation(FVector(0,End*42.f,53.f));
			Nail->SetWorldScale3D(FVector(.026f,.026f,.007f));
		}
	}
	Bar=Piece(Cube,Bark,FVector(2478,0,122),FVector(.14,2.18,.16));
	for(int Sign: {-1,1})
		Piece(Cube,Iron,FVector(2477,Sign*97,117),FVector(.19,.12,.18));
	// The warm porch lamp picks out the resident's face and the boards under his boots.
	const FVector Lantern(2395,-141,167);
	Stick(Lantern+FVector(47,0,23),Lantern+FVector(0,0,23),4,Iron);
	Stick(Lantern+FVector(0,0,23),Lantern+FVector(0,0,9),3,Iron);
	Piece(Tube,Iron,Lantern+FVector(0,0,8),FVector(.17,.17,.045));
	Piece(Tube,Iron,Lantern-FVector(0,0,8),FVector(.145,.145,.035));
	Piece(Ball,Fire,Lantern,FVector(.07,.07,.13));
	for (int Corner : {-1,1})
		Stick(Lantern+FVector(0,Corner*6,-8),Lantern+FVector(0,Corner*6,8),1.2f,Iron);
	auto* PorchLight = NewObject<UPointLightComponent>(this);
	PorchLight->SetupAttachment(GetRootComponent());
	PorchLight->SetRelativeLocation(Lantern-FVector(12,0,0));
	PorchLight->SetIntensityUnits(ELightUnits::Candelas);
	PorchLight->SetIntensity(2.8f);
	PorchLight->SetLightColor(FLinearColor(1.f,.68f,.40f));
	PorchLight->SetAttenuationRadius(600.f);
	PorchLight->SetSourceRadius(9.f);
	PorchLight->SetCastShadows(false);
	PorchLight->RegisterComponent();
	// A bar across the small recessed window catches the light and frames the silhouette.
	Piece(Cube,Bark,FVector(2451,221,191),FVector(.12,.06,1.0));
	auto Group=[this](FVector Position)
	{
		auto* C=NewObject<USceneComponent>(this); C->SetupAttachment(GetRootComponent());
		C->SetRelativeLocation(Position); C->RegisterComponent(); return C;
	};
	WindowWarning=Group(FVector(2511,222,171));
	Piece(Ball,Cloth,FVector(0,0,1),FVector(.38,.38,.47),FRotator::ZeroRotator,WindowWarning);
	Stick(FVector(-8,-30,-10),FVector(-9,17,11),5,Iron,WindowWarning);
	Pack->SpawnMembers();
}

void AWolfCottageSet::SetPit(bool bDug)
{
	bPitOpen=bDug;
	for(AActor* A:PitCover) { A->SetActorHiddenInGame(bDug); A->SetActorEnableCollision(!bDug); }
	for (int I=0; I<WeakPlanks.Num(); ++I)
	{
		WeakPlanks[I]->SetVisibility(!bDug,true);
		WeakPlanks[I]->SetRelativeRotation(FRotator(0,(I%3-1)*2.5f,0));
	}
}
void AWolfCottageSet::SetShovel(bool bVisible){ for(AActor* A:Shovel) A->SetActorHiddenInGame(!bVisible); }
void AWolfCottageSet::SetSupplies(bool bVisible){ for(auto& C:TableGoods) C->SetVisibility(bVisible); }
void AWolfCottageSet::SetBrand(bool bVisible){ for(auto& C:Brand) C->SetVisibility(bVisible); }
void AWolfCottageSet::Door(bool bOpen,bool bBarred)
{
	SwingDoor(bOpen ? 1.f : 0.f);
	Bar->SetVisibility(bBarred);
}

void AWolfCottageSet::SwingDoor(float Fraction)
{
	const FRotator Swing(0,-90.f * FMath::SmoothStep(0.f,1.f,Fraction),0);
	const FVector Hinge(2450,-90,125);
	for (AActor* Panel : DoorActors)
		Panel->SetActorLocationAndRotation(Hinge + Swing.RotateVector(FVector(0,90,0)),Swing);
}
void AWolfCottageSet::RestoreProps(bool bCaughtBefore)
{
	bWarned=bCaughtBefore;
	if(VoiceAudio) VoiceAudio->Stop();
	WindowWarning->SetVisibility(!bCaughtBefore,true);
	NextGrowl=0; HidePack();
}
void AWolfCottageSet::Animate(float T,float DigTime,float Health,bool bBarred)
{
	if(WindowWarning->IsVisible())
		WindowWarning->SetRelativeLocation(FVector(2511,220+7*FMath::Sin(T*1.5),171));
	if(DigTime>=0 && !bPitOpen)
		for(int I=0;I<WeakPlanks.Num();I++)
			WeakPlanks[I]->SetRelativeRotation(FRotator(FMath::Sin(DigTime*8+I)*9, (I%3-1)*2.5f,0));
	if(bBarred)
	{
		const float Damage=1-FMath::Clamp(Health,0.f,1.f);
		Bar->SetRelativeRotation(FRotator(0,0,Damage*9*Damage + Damage*FMath::Pow(FMath::Max(0.f,FMath::Sin(T*7)),8.f)*3));
	}
}
void AWolfCottageSet::Impact(const FVector& P,float Volume,float Pitch)
{
	if(ImpactSound) UGameplayStatics::PlaySoundAtLocation(this,ImpactSound,P,Volume,Pitch,0.f,Nearby);
}
void AWolfCottageSet::HidePack() { Pack->Hide(); }

UWolfMeshComponent* AWolfCottageSet::GetLeadWolf() const { return Pack->GetLeadWolf(); }

void AWolfCottageSet::TrailPack(AWolfTrailRoute* Route,float Distance,const FVector& Player)
{
	if (!Route) return;
	// The pursuit distance marks the front of the attack range. Keep the
	// hunter's feet behind that mark so the final spring covers real ground.
	const float Marker = FMath::Max(0.f,Distance-195.f);
	const FVector Lead = Route->Spline->GetLocationAtDistanceAlongSpline(Marker,ESplineCoordinateSpace::World);
	const FVector Heading = Route->Spline->GetDirectionAtDistanceAlongSpline(Marker,ESplineCoordinateSpace::World);
	const float Threat = 1.f - FMath::SmoothStep(225.f,740.f,float(FVector::Dist2D(Lead,Player)));
	Pack->Hunt(GetWorld()->GetDeltaSeconds(),Lead,Heading,Player,Threat);
	Growl(Lead,Threat);
}

void AWolfCottageSet::YardPack(const FVector& Target,float Separation,bool bBurning)
{
	const FVector D = FVector(1,.24,0).GetSafeNormal();
	const float Fraction = FMath::Clamp(Separation,0.f,1.f);
	const float Gap = 195.f + Fraction*(bBurning ? 1050.f : 1400.f);
	FVector Lead = Target - D*Gap;
	Lead.Z = Target.Z - 92.f;
	const float Threat = (1.f-Fraction)*(bBurning ? .62f : 1.f);
	Pack->Hunt(GetWorld()->GetDeltaSeconds(),Lead,D,Target,Threat);
	Growl(Lead,Threat);
}

void AWolfCottageSet::DoorPack(const FVector& Door,bool bRepelled)
{
	Pack->GuardDoor(GetWorld()->GetDeltaSeconds(),Door,bRepelled);
	Growl(Door-FVector(80,0,40),bRepelled ? .4f : .85f);
}

void AWolfCottageSet::Growl(const FVector& Lead,float Threat)
{
	const float Time = GetWorld()->GetTimeSeconds();
	if (Threat > .6f && Time >= NextGrowl)
	{
		if (GrowlSound) UGameplayStatics::PlaySoundAtLocation(this,GrowlSound,Lead,.75f,.90f,0,Nearby);
		NextGrowl = Time+3.f;
	}
}

void AWolfCottageSet::BuildRoof()
{
	// Keep the original roof as the interior ceiling. The weathering courses overlap
	// away from the ridge so their ends cast thin, irregular shadows in the moonlight.
	auto* Shakes=NewObject<UInstancedStaticMeshComponent>(this);
	Shakes->SetupAttachment(GetRootComponent());
	Shakes->SetStaticMesh(Cube);
	Shakes->SetMaterial(0,Bark);
	Shakes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shakes->SetCanEverAffectNavigation(false);
	Shakes->RegisterComponent();
	FRandomStream Weather(401);
	for (int Side : {-1,1})
	{
		const FRotator Pitch(0,0,Side*24.0);
		Piece(Cube,Bark,FVector(2850,Side*212,437),FVector(8.9,4.78,.10),Pitch);
		for(int Row=0; Row<8; Row++)
		{
			const float Y=30+Row*53;
			const float Z=538-Y*.445f;
			for(int Tile=0; Tile<18; Tile++)
			{
				const float X=2419+Tile*50+(Row%2 ? 8.f : -8.f);
				const FVector Center(X,Side*(Y+Weather.FRandRange(-3,3)),Z+Weather.FRandRange(-1,1));
				const FRotator R(0,Weather.FRandRange(-1.6,1.6),Pitch.Roll);
				Shakes->AddInstance(FTransform(R,Center,FVector(.505,.67,.044)));
			}
		}
	}
	for(int End : {-1,1})
	{
		const float X=2850+End*402;
		for(int Row=0;Row<11;Row++)
		{
			const float Z=348+Row*16.6f;
			const float HalfWidth=FMath::Max(12.f,(535-Z)/.445f);
			Piece(Cube,Bark,FVector(X,0,Z),FVector(.16,HalfWidth*.02,.175));
		}
	}
	Stick(FVector(2400,0,537),FVector(3300,0,537),21,Bark);
	Piece(Cube,Masonry,FVector(3140,218,452),FVector(1.1,1.17,2.34));
	for(int I=0;I<4;I++)
		Piece(Cube,Masonry,FVector(3140+(I%2)*1.4,218,565+I*24),FVector(1.13-I*.035,1.20-I*.035,.235),FRotator(0,2-I,0));
	Piece(Cube,Masonry,FVector(3140,218,651),FVector(1.29,1.33,.16));
	Piece(Cube,Iron,FVector(3140,218,660),FVector(.86,.90,.04));
}

void AWolfCottageSet::Speak(USoundBase* Line,FVector P,float Volume)
{
	if(!Line || !VoiceAudio) return;
	VoiceAudio->Stop();
	VoiceAudio->SetSound(Line);
	VoiceAudio->SetWorldLocation(P);
	VoiceAudio->SetVolumeMultiplier(Volume);
	VoiceAudio->Play();
}
void AWolfCottageSet::WarnAtWindow()
{
	if(bWarned) return;
	bWarned=true;
	Speak(WarningVoice,FVector(2470,220,177),.76f);
}
void AWolfCottageSet::ShoutAtDoor()
{
	WindowWarning->SetVisibility(false,true);
	Speak(ShoutVoice,FVector(2540,0,155),1.f);
}
void AWolfCottageSet::CurseFromPit() { Speak(TrappedVoice,FVector(2260,0,-102),.79f); }
