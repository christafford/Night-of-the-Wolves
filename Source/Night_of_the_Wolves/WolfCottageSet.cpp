#include "WolfCottageSet.h"
#include "MaxDigStroke.h"
#include "WolfPackComponent.h"
#include "WolfMeshComponent.h"
#include "WolfTrailGameMode.h"
#include "WolfTrailRoute.h"
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
	CreateYardProps();
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BoxAsset(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BallAsset(TEXT("/Engine/BasicShapes/Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TubeAsset(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Coil(TEXT("/Game/NightOfTheWolves/FirstLeg/Rope"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Pouch(TEXT("/Game/NightOfTheWolves/FirstLeg/Gold"));
	RopeMesh = Coil.Object;
	PouchMesh = Pouch.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Shape(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(TEXT("/Game/NightOfTheWolves/Presentation/M_TreeBark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mud(TEXT("/Game/NightOfTheWolves/Presentation/M_ForestGround"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Ember(TEXT("/Game/NightOfTheWolves/Presentation/M_FireGlow"));
	Cube=BoxAsset.Object; Ball=BallAsset.Object; Tube=TubeAsset.Object; ShapeMaterial=Shape.Object;
	Bark=Wood.Object; Soil=Mud.Object; Fire=Ember.Object;
	static ConstructorHelpers::FObjectFinder<USoundBase> Impact(TEXT("/Game/NightOfTheWolves/FirstLeg/DoorImpact"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Growl(TEXT("/Game/NightOfTheWolves/FirstLeg/Growl"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Hearth(TEXT("/Game/NightOfTheWolves/FirstLeg/Hearth"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Stone(TEXT("/Game/NightOfTheWolves/Presentation/M_CabinStone"));
	Masonry=Stone.Object;
	ImpactSound=Impact.Object; GrowlSound=Growl.Object; FireSound=Hearth.Object;
}

void AWolfCottageSet::BeginPlay()
{
	Super::BeginPlay();
	Pack = CastChecked<AWolfTrailGameMode>(GetWorld()->GetAuthGameMode())->GetHuntingPack();
	Nearby=NewObject<USoundAttenuation>(this);
	Nearby->Attenuation.AttenuationShapeExtents=FVector(380,0,0);
	Nearby->Attenuation.FalloffDistance=1550;
	Nearby->Attenuation.bSpatialize=true;
	Nearby->Attenuation.bAttenuate=true;
	if (FireSound) UGameplayStatics::SpawnSoundAtLocation(this,FireSound,FVector(3065,220,80),FRotator::ZeroRotator,.35f,1,0,Nearby);
	auto ActorsWith=[this](FName Tag,TArray<TObjectPtr<AActor>>& Dest)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(this,Tag,Found);
		for (AActor* A:Found) Dest.Add(A);
	};
	ActorsWith(TEXT("M2Door"),DoorActors); ActorsWith(TEXT("M3PitCover"),PitCover);
	for (const AActor* Cover : PitCover)
		PitBounds += Cover->GetComponentsBoundingBox(true);
	// Legacy maps used two primitive actors to represent the pickup.
	TArray<AActor*> BlockoutShovels;
	UGameplayStatics::GetAllActorsWithTag(this, TEXT("M3Shovel"), BlockoutShovels);
	for (AActor* Actor : BlockoutShovels)
	{
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);
	}
	ActorsWith(TEXT("M2Hermit"),OldHermit);
	for (AActor* A:OldHermit) { A->SetActorHiddenInGame(true); A->SetActorEnableCollision(false); }
	auto Dye=[this](FLinearColor Color)
	{
		auto* M=UMaterialInstanceDynamic::Create(ShapeMaterial,this);
		M->SetVectorParameterValue(TEXT("Color"),Color);
		return M;
	};
	Gold=Dye(FLinearColor(.78,.43,.07)); Cloth=Dye(FLinearColor(.14,.17,.13));
	Iron=Dye(FLinearColor(.085,.09,.10));
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
	// Keep the same shapes that Max carries. The coil hangs on the back wall;
	// the partly spilled purse sits within reach from the front of the worktable.
	Stick(FVector(3208,-84,144), FVector(3166,-84,151), 5.5f, Bark);
	RopeCoil = Piece(RopeMesh,nullptr,FVector(3188,-84,123),FVector(1.85),FRotator(0,90,0));
	GoldSack = Piece(PouchMesh,nullptr,FVector(2800,-220,110),FVector(1.7));
	for (int I=0; I<7; ++I)
		Piece(Tube,Gold,FVector(12+I*1.6,4+5*FMath::Sin(2.4*I),-15.4+I%2*.4),
			FVector(.030,.030,.005),FRotator::ZeroRotator,GoldSack);
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
	// Span the physical cover with weathered boards running from the yard to the sill.
	if (PitBounds.IsValid)
	{
		const FVector Surface = PitSurface();
		const FVector Size = PitBounds.GetSize();
		const int32 Count = FMath::Max(1,FMath::RoundToInt(Size.Y / 23.0));
		const double Spacing = Size.Y / Count;
		for (int32 I=0; I<Count; ++I)
		{
			const double Y = PitBounds.Min.Y + (I+.5)*Spacing;
			const double Length = Size.X - (I*7 % 17);
			const FVector Centre(Surface.X + (I%3-1)*2.0,Y,Surface.Z+2.3+(I%3)*.32);
			auto* Board = Piece(Cube,Bark,Centre,FVector(Length/100.,Spacing*.0088,.035),FRotator(0,(I%3-1)*1.5,0));
			WeakPlanks.Add(Board);
			for (int End : {-1,1})
			{
				auto* Nail = Piece(Tube,Iron,FVector::ZeroVector,FVector::OneVector);
				Nail->SetAbsolute(false,false,true);
				Nail->AttachToComponent(Board,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				Nail->SetRelativeLocation(FVector(End*42.f,0,53.f));
				Nail->SetWorldScale3D(FVector(.026f,.026f,.007f));
			}
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
}

void AWolfCottageSet::SetPit(bool bDug)
{
	bPitOpen=bDug;
	for(AActor* A:PitCover) { A->SetActorHiddenInGame(bDug); A->SetActorEnableCollision(!bDug); }
	for (int I=0; I<WeakPlanks.Num(); ++I)
	{
		WeakPlanks[I]->SetVisibility(!bDug,true);
		WeakPlanks[I]->SetRelativeRotation(FRotator(0,(I%3-1)*1.5f,0));
	}
}
void AWolfCottageSet::SetShovel(bool bVisible){ YardShovel->SetVisibility(bVisible); }
void AWolfCottageSet::SetRope(bool bVisible) { RopeCoil->SetVisibility(bVisible); }
void AWolfCottageSet::SetGold(bool bVisible) { GoldSack->SetVisibility(bVisible,true); }
FVector AWolfCottageSet::GetRopeLocation() const { return RopeCoil->GetComponentLocation(); }
FVector AWolfCottageSet::GetGoldLocation() const { return GoldSack->GetComponentLocation(); }
void AWolfCottageSet::SetBrand(bool bVisible){ for(auto& C:Brand) C->SetVisibility(bVisible); }
void AWolfCottageSet::Door(bool bOpen,bool bBarred)
{
	SwingDoor(bOpen ? 1.f : 0.f);
	Bar->SetVisibility(bBarred);
}

void AWolfCottageSet::SwingDoor(float Fraction)
{
	const FRotator Swing(0,-90.f * FMath::SmoothStep(0.f,1.f,Fraction),0);
	for (AActor* Panel : DoorActors)
	{
		const FVector Hinge(2450,-90,Panel->GetActorLocation().Z);
		Panel->SetActorLocationAndRotation(Hinge + Swing.RotateVector(FVector(0,90,0)),Swing);
	}
}
void AWolfCottageSet::RestoreProps(bool bCaughtBefore)
{
	WindowWarning->SetVisibility(!bCaughtBefore,true);
	NextGrowl=0;
}
void AWolfCottageSet::Animate(float T,float DigTime,float Health,bool bBarred)
{
	if(WindowWarning->IsVisible())
		WindowWarning->SetRelativeLocation(FVector(2511,220+7*FMath::Sin(T*1.5),171));
	if(DigTime>=0 && !bPitOpen)
	{
		const float Cycle = MaxDigStroke::Cycle(DigTime);
		const float Phase = FMath::Frac(Cycle);
		const float Pry = FMath::SmoothStep(.31f,.46f,Phase)*(1.f-FMath::SmoothStep(.48f,.67f,Phase));
		for(int I=0;I<WeakPlanks.Num();I++)
		{
			const float Lift = FMath::Cos(.72f*I + FMath::FloorToInt(Cycle))*Pry*7.f;
			WeakPlanks[I]->SetRelativeRotation(FRotator(Lift, (I%3-1)*1.5f,0));
		}
	}
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


UWolfMeshComponent* AWolfCottageSet::GetLeadWolf() const { return Pack->GetAttacker() ? Pack->GetAttacker() : Pack->GetLeadWolf(); }

void AWolfCottageSet::TrailPack(AWolfTrailRoute* Route,float Lead)
{
	if (!Route || !Pack->GetLeadWolf()) return;
	const FVector Front=Pack->GetMember(1)->GetComponentLocation();
	const auto* Spline=Route->Spline.Get();
	const float Here=Spline->GetDistanceAlongSplineAtSplineInputKey(Spline->FindInputKeyClosestToWorldLocation(Front));
	const FVector Guide=Spline->GetLocationAtDistanceAlongSpline(Here+470.f,ESplineCoordinateSpace::World);
	const FVector Heading=Spline->GetDirectionAtDistanceAlongSpline(Here,ESplineCoordinateSpace::World);
	// Encounter progress sets a permitted stand-off. The path guide is sampled
	// from the wolf's own position and stays there if Max disappears behind cover.
	Pack->Hunt(GetWorld()->GetDeltaSeconds(),Guide,Heading,145.f+Lead,EWolfGroundEncounter::Trail,Lead<180.f);
	Growl(Front,1.f-FMath::Clamp(Lead/600.f,0.f,1.f));
}

void AWolfCottageSet::YardPack(const AWolfTrailRoute* Route,float Separation,bool bBurning)
{
	if (!Route || !Pack->GetLeadWolf()) return;
	const auto* Spline=Route->Spline.Get();
	const FVector Front=Pack->GetMember(1)->GetComponentLocation();
	const float Here=Spline->GetDistanceAlongSplineAtSplineInputKey(Spline->FindInputKeyClosestToWorldLocation(Front));
	const FVector Heading=Spline->GetDirectionAtDistanceAlongSpline(Here,ESplineCoordinateSpace::World);
	const FVector Guide=Spline->GetLocationAtDistanceAlongSpline(Here+480.f,ESplineCoordinateSpace::World);
	const float Fraction=FMath::Clamp(Separation,0.f,1.f);
	Pack->Hunt(GetWorld()->GetDeltaSeconds(),Guide,Heading,145.f+Fraction*(bBurning ? 850.f : 590.f),
		EWolfGroundEncounter::Clearing,Fraction<.14f);
	Growl(Front,1.f-Fraction);
}

void AWolfCottageSet::DoorPack(const FVector& Door,bool bRepelled,bool bExitExposed,bool bBreached)
{
	Pack->GuardDoor(GetWorld()->GetDeltaSeconds(),Door,bRepelled,bExitExposed,bBreached);
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
	// The map contains the notched log shell and split-shake roof. The flue rises from the hearth.
	Piece(Cube,Masonry,FVector(3140,218,452),FVector(1.1,1.17,2.34));
	for(int I=0;I<4;I++)
		Piece(Cube,Masonry,FVector(3140+(I%2)*1.4,218,565+I*24),FVector(1.13-I*.035,1.20-I*.035,.235),FRotator(0,2-I,0));
	Piece(Cube,Masonry,FVector(3140,218,651),FVector(1.29,1.33,.16));
	Piece(Cube,Iron,FVector(3140,218,660),FVector(.86,.90,.04));
}

void AWolfCottageSet::HideWindowBarrel()
{
	WindowWarning->SetVisibility(false,true);
}
