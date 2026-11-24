#include "WolfHeldItem.h"
#include "WolfTrailGameMode.h"
#include "Night_of_the_WolvesCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UWolfHeldItem::UWolfHeldItem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Clod(TEXT("/Game/NightOfTheWolves/Cascades/SM_CreekStoneA/StaticMeshes/SM_CreekStoneA"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Earth(TEXT("/Game/NightOfTheWolves/Presentation/M_ForestGround"));
	ClodMesh = Clod.Object;
	Soil = Earth.Object;
	for (FName Id : {FName(TEXT("Rope")), FName(TEXT("Gold")), FName(TEXT("Shovel")), FName(TEXT("Firebrand"))})
	{
		const FString Path = Id == TEXT("Shovel")
			? TEXT("/Game/NightOfTheWolves/FirstLeg/CottageYard/SM_FieldSpade")
			: FString::Printf(TEXT("/Game/NightOfTheWolves/FirstLeg/%s"), *Id.ToString());
		ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(*Path);
		if (Mesh.Succeeded()) Props.Add(Id, Mesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Shape(TEXT("/Engine/BasicShapes/Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Ember(TEXT("/Game/NightOfTheWolves/Presentation/M_FireGlow"));
	FlameShape = Shape.Object;
	EmberMaterial = Ember.Object;
}

void UWolfHeldItem::BeginPlay()
{
	Super::BeginPlay();
	if (!GetWorld()->GetAuthGameMode<AWolfTrailGameMode>()) { SetComponentTickEnabled(false); return; }
	auto MakeMesh = [this]()
	{
		auto* Mesh = NewObject<UStaticMeshComponent>(GetOwner());
		Mesh->SetupAttachment(this);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->RegisterComponent();
		return Mesh;
	};
	Held = MakeMesh();
	Flame = MakeMesh();
	Flame->SetStaticMesh(FlameShape);
	Flame->SetMaterial(0, EmberMaterial);
	Flame->SetRelativeLocation(FVector(3,0,65));
	Flame->SetCastShadow(false);
	Flame->SetVisibility(false);
	Glow = NewObject<UPointLightComponent>(GetOwner());
	Glow->SetupAttachment(this);
	Glow->SetRelativeLocation(FVector(3,0,65));
	Glow->SetLightColor(FLinearColor(1,.38,.09));
	Glow->SetAttenuationRadius(580);
	Glow->SetCastShadows(false);
	Glow->SetIntensity(0.f);
	Glow->RegisterComponent();
	// Keep the held object upright as the hand follows the running animation.
	SetAbsolute(false, true, false);
}

void UWolfHeldItem::TickComponent(float Dt,ELevelTick T,FActorComponentTickFunction* Tick)
{
	Super::TickComponent(Dt,T,Tick);
	const auto* Mode = GetWorld()->GetAuthGameMode<AWolfTrailGameMode>();
	if (!Mode || !Held) return;
	const auto& Bag = Mode->Inventory;
	const bool Lit = Bag.bBurning && Bag.Equipped == TEXT("Firebrand");
	if (bWasBurning && !Lit) SnuffRemaining = .42f;
	bWasBurning = Lit;
	SnuffRemaining = FMath::Max(0.f, SnuffRemaining-Dt);
	const FName Item = SnuffRemaining > 0 ? FName(TEXT("Firebrand")) : Bag.Equipped;
	if (Item != Shown)
	{
		Shown = Item;
		Held->SetStaticMesh(Props.FindRef(Shown));
	}
	const float Snuff = SnuffRemaining/.42f;
	SetWorldRotation(FRotator(-12.f - (Snuff>0 ? 58.f*(1-Snuff) : 0.f), GetOwner()->GetActorRotation().Yaw, 8.f));
	auto* Max = Cast<ANight_of_the_WolvesCharacter>(GetOwner());
	if (Max && Shown == TEXT("Shovel") && !Mode->IsDead() && Mode->DiggingTime() >= 0.f)
	{
		UpdateSpadePose(*Max, Mode->DiggingTime() / Mode->DigSeconds);
	}
	else if (DigBody) EndDigPose();
	const float Flicker = 1.f + .11f*FMath::Sin(GetWorld()->GetTimeSeconds()*16.f);
	Flame->SetVisibility(Lit || Snuff>0);
	const float Brightness = (Lit ? 1.f : Snuff)*Flicker;
	Flame->SetRelativeScale3D(FVector(.075,.065,.23)*Brightness);
	const bool bCaveLight = Mode->Checkpoint >= 5;
	if (Glow->CastShadows != bCaveLight) Glow->SetCastShadows(bCaveLight);
	Glow->SetIntensity((bCaveLight ? 1500.f : 860.f)*Brightness);
}
