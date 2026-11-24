#include "WolfCottageSet.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

void AWolfCottageSet::CreateYardProps()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Spade(TEXT("/Game/NightOfTheWolves/FirstLeg/CottageYard/SM_FieldSpade"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cask(TEXT("/Game/NightOfTheWolves/FirstLeg/CottageYard/SM_OakBarrel"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Bale(TEXT("/Game/NightOfTheWolves/FirstLeg/CottageYard/SM_BoundHay"));
	auto YardProp = [this](FName Name, UStaticMesh* Mesh, const FVector& Location, const FRotator& Rotation)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(GetRootComponent());
		Component->SetStaticMesh(Mesh);
		Component->SetRelativeLocationAndRotation(Location, Rotation);
		Component->SetCanEverAffectNavigation(false);
		Component->SetGenerateOverlapEvents(false);
		return Component;
	};

	// The spade shares its grip pivot with Max's equipped mesh. Plant the cutting edge
	// and lean the handle against the cask just outside the window's sightline.
	const FVector Lean = FVector(.18, .39, 1.).GetSafeNormal();
	const FVector BladeTip(2370., 289., 1.);
	const FRotator LeanRotation = FRotationMatrix::MakeFromZX(Lean, FVector(-.32, 1., 0.)).Rotator();
	YardShovel = YardProp(TEXT("Field spade"), Spade.Object, BladeTip + 70.1 * Lean, LeanRotation);
	YardShovel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	YardProp(TEXT("Oak cask"), Cask.Object, FVector(2395., 371., .4), FRotator(0., -27., 0.));
	YardProp(TEXT("Hay bale"), Bale.Object, FVector(2384., 467., 1.), FRotator(0., 73., 0.));
}
