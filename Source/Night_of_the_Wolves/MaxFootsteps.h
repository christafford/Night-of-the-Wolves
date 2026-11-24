#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MaxFootsteps.generated.h"

class USoundBase;

UCLASS(ClassGroup=(Audio))
class UMaxFootsteps : public UActorComponent
{
	GENERATED_BODY()
public:
	UMaxFootsteps();
	void BeginPlay() override;
	void TickComponent(float Seconds, ELevelTick Type, FActorComponentTickFunction* Function) override;

	UPROPERTY(EditDefaultsOnly, Category="Max|Footsteps", meta=(ClampMin="0"))
	float Volume = .72f;
private:
	UPROPERTY(EditDefaultsOnly, Category="Max|Footsteps")
	TArray<TObjectPtr<USoundBase>> TrailSteps;
	UPROPERTY(EditDefaultsOnly, Category="Max|Footsteps")
	TArray<TObjectPtr<USoundBase>> TimberSteps;

	FVector PreviousLocation;
	float JumpPeak = -BIG_NUMBER;
	float SincePlant = 0.f;
	float DistanceSincePlant = 0.f;
	int32 LastFoot = -1;
	int32 LastTake = -1;
};
