#include "HuntingWolf.h"
#include "WolfMeshComponent.h"
#include "AIController.h"

AHuntingWolf::AHuntingWolf()
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	Body = CreateDefaultSubobject<UWolfMeshComponent>(TEXT("Wolf"));
	SetRootComponent(Body);
}
