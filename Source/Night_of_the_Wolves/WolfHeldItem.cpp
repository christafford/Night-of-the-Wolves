#include "WolfHeldItem.h"
#include "WolfTrailGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UWolfHeldItem::UWolfHeldItem()
{
	PrimaryComponentTick.bCanEverTick = true;
	for (FName Id : {FName(TEXT("Rope")), FName(TEXT("Gold")), FName(TEXT("Shovel")), FName(TEXT("Firebrand"))})
	{
		const FString Path = FString::Printf(TEXT("/Game/NightOfTheWolves/FirstLeg/%s"), *Id.ToString());
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
	const float DigStroke = (Item==TEXT("Shovel") && Mode->DiggingTime()>=0) ? FMath::Sin(Mode->DiggingTime()*7.0f) : 0.f;
	Held->SetRelativeLocation(FVector(12*DigStroke,0,-13*DigStroke));
	const float Snuff = SnuffRemaining/.42f;
	SetWorldRotation(FRotator(-12.f + 38.f*DigStroke - (Snuff>0 ? 58.f*(1-Snuff) : 0.f), GetOwner()->GetActorRotation().Yaw, 8.f));
	const float Flicker = 1.f + .11f*FMath::Sin(GetWorld()->GetTimeSeconds()*16.f);
	Flame->SetVisibility(Lit || Snuff>0);
	const float Brightness = (Lit ? 1.f : Snuff)*Flicker;
	Flame->SetRelativeScale3D(FVector(.075,.065,.23)*Brightness);
	Glow->SetIntensity(860.f*Brightness);
}
