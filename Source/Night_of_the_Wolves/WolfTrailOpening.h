#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "WolfTrailOpening.generated.h"

class APawn;
class UAudioComponent;
class USplineComponent;

UCLASS()
class AWolfTrailOpening : public ACameraActor
{
	GENERATED_BODY()
public:
	AWolfTrailOpening();
	void Start(USplineComponent& Trail, APawn& Max, float MaxDistance);
	bool Advance(float DeltaSeconds);
	float TitleOpacity() const;
	float ElapsedSeconds() const { return Elapsed; }

private:
	UPROPERTY(VisibleAnywhere, Category="Opening") TObjectPtr<UAudioComponent> Breath;
	UPROPERTY(EditDefaultsOnly, Category="Opening", meta=(ClampMin="0", Units="s")) float TitleHold = 2.6f;
	UPROPERTY(EditDefaultsOnly, Category="Opening", meta=(ClampMin="0.1", Units="s")) float TitleDissolve = 1.45f;
	UPROPERTY(EditDefaultsOnly, Category="Opening", meta=(ClampMin="100", Units="cm/s")) float RunSpeed = 1050.f;
	UPROPERTY(EditDefaultsOnly, Category="Opening", meta=(ClampMin="0.1", Units="s")) float CameraHandoff = 1.7f;
	UPROPERTY(EditDefaultsOnly, Category="Opening", meta=(ClampMin="10", Units="cm")) float EyeHeight = 74.f;
	UPROPERTY(EditDefaultsOnly, Category="Opening", meta=(ClampMin="0", Units="cm")) float HeadBob = 8.f;
	UPROPERTY(EditDefaultsOnly, Category="Opening", meta=(ClampMin="0")) float BreathingVolume = .86f;
	TWeakObjectPtr<USplineComponent> Route;
	TWeakObjectPtr<APawn> Player;
	float Elapsed = 0.f;
	float RunDuration = 0.f;
	float Destination = 0.f;
	bool bBreathing = false;
	bool bBreathFading = false;
	void FrameCamera(float RunTime);
};
