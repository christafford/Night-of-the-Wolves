#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CottageHermit.generated.h"

class ACameraActor;
class APawn;
class UAnimSequence;
class UPointLightComponent;
class USkeletalMeshComponent;
class USoundBase;

UCLASS()
class ACottageHermit : public AActor
{
	GENERATED_BODY()
public:
	ACottageHermit();
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

	void RestoreEncounter(bool bTrapped, FVector Door, FVector Hollow);
	void AnswerKnock(APawn& Max);
	void Stage(float Run, float Turn, float Drop);
	void Fire();
	void EndReveal(bool bBlend);

private:
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Face;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Workwear;
	UPROPERTY() TObjectPtr<USceneComponent> Shotgun;
	UPROPERTY() TObjectPtr<UPointLightComponent> Flash;
	UPROPERTY() TObjectPtr<UAnimSequence> Aim;
	UPROPERTY() TObjectPtr<UAnimSequence> Jog;
	UPROPERTY() TObjectPtr<USoundBase> Shot;
	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	TWeakObjectPtr<APawn> Viewer;
	FVector Doorway = FVector::ZeroVector;
	FVector Timbers = FVector::ZeroVector;
	float SidestepY = 0;
	float FlashSeconds = 0;
	float Fall = 0;
	float AimFraming = 0;
#if !UE_BUILD_SHIPPING
	float PortraitHold = 0;
#endif
	bool bRunning = false;
	bool bJumpWasAllowed = true;
	bool bTurnedWithMovement = false;
	bool bHoldingMark = false;
};
