#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WolfTrailRoute.generated.h"

UCLASS()
class NIGHT_OF_THE_WOLVES_API AWolfTrailRoute : public AActor
{
	GENERATED_BODY()
public:
	AWolfTrailRoute();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trail")
	TObjectPtr<class USplineComponent> Spline;
};
