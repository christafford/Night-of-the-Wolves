#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HuntingWolf.generated.h"

class UWolfMeshComponent;

// Each member keeps its own pawn, controller and physical identity for the whole leg.
UCLASS()
class AHuntingWolf : public APawn
{
	GENERATED_BODY()
public:
	AHuntingWolf();
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWolfMeshComponent> Body;
};
