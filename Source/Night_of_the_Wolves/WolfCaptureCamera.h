#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "WolfCaptureCamera.generated.h"

class ACharacter;
class AWolfTrailGameMode;
class UAnimSequence;
class USoundBase;
class UPointLightComponent;
class UWolfMeshComponent;
class UWolfPackComponent;

// A short world-space shot that follows the pursuing wolf from its planted feet to Max.
UCLASS()
class AWolfCaptureCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	AWolfCaptureCamera();
	void Start(AWolfTrailGameMode& Game, ACharacter& Player, UWolfMeshComponent& Leader);
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UWolfMeshComponent> Wolf;
	UPROPERTY() TObjectPtr<UWolfPackComponent> Pack;
	UPROPERTY() TObjectPtr<ACharacter> Max;
	UPROPERTY() TObjectPtr<AWolfTrailGameMode> Mode;
	UPROPERTY() TObjectPtr<UAnimSequence> FrontFall;
	UPROPERTY() TObjectPtr<UAnimSequence> BackFall;
	UPROPERTY() TObjectPtr<UAnimSequence> LeftFall;
	UPROPERTY() TObjectPtr<UAnimSequence> RightFall;
	UPROPERTY() TObjectPtr<USoundBase> Snarl;
	UPROPERTY() TObjectPtr<USoundBase> Impact;
	UPROPERTY() TObjectPtr<UPointLightComponent> MoonFill;

	FVector AttackDirection = FVector::ForwardVector;
	FVector StartFeet = FVector::ZeroVector;
	FVector RunStart = FVector::ZeroVector;
	FVector LaunchFeet = FVector::ZeroVector;
	FVector FramingCentre = FVector::ZeroVector;
	float PitBound = 0;
	float RunIn = 0;
	float Elapsed = 0;
	bool bHit = false;
	bool bFinished = false;
	FVector TargetChest() const;
	void FrameAttack(APlayerController& Controller);
	void KeepSilhouettesInFrame();
	float ViewAspect = 16.f / 9.f;
	void HitPlayer();

#if !UE_BUILD_SHIPPING
	int32 ReviewFrame = 0;
	bool bPhotograph = false;
	void Review(const TCHAR* Stage);
#endif
};
