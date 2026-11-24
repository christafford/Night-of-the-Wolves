#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "RiverDrowningShot.generated.h"

class ACharacter;
class ANight_of_the_WolvesCharacter;
class AWolfTrailGameMode;
class UAudioComponent;
class UPoseableMeshComponent;
class UStaticMeshComponent;

UCLASS()
class ARiverDrowningShot : public ACameraActor
{
	GENERATED_BODY()
public:
	ARiverDrowningShot();
	void Start(AWolfTrailGameMode& Game, ACharacter& Player);
	virtual void Tick(float Dt) override;

	UPROPERTY(EditDefaultsOnly, Category="River", meta=(ClampMin="2.2",Units="s"))
	float StruggleSeconds = 3.4f;

private:
	UPROPERTY() TObjectPtr<AWolfTrailGameMode> Mode;
	UPROPERTY() TObjectPtr<ACharacter> Victim;
	UPROPERTY() TObjectPtr<UAudioComponent> Cry;
	UPROPERTY() TObjectPtr<UPoseableMeshComponent> Swimmer;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Foam;
	FVector Entry, StartLocation, TowardCamera;
	float Elapsed = 0.f;
	bool bStartedCry = false;
	void AnimateSwimmer(float Beat, float Sink);
};
