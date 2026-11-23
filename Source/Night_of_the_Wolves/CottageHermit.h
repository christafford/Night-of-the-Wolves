#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CottageHermit.generated.h"

class ACameraActor;
class APawn;
class UAnimSequence;
class UDialogueVoiceComponent;
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

	void RestoreEncounter(bool bTrapped, FVector Door, FVector Hollow, float HalfWidth);
	void AnswerKnock(APawn& Max);
	void WarnAtWindow();
	void ShoutAtMax();
	void CryFromPit();
	void LoseFooting();
	void RestInPit();
	void StopSpeech();
	void Stage(float Run, float Turn, float Drop);
	void Fire();
	void EndReveal(bool bBlend);

private:
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Face;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Workwear;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Whiskers;
	UPROPERTY() TObjectPtr<USceneComponent> Shotgun;
	UPROPERTY() TObjectPtr<UPointLightComponent> Flash;
	UPROPERTY() TObjectPtr<UDialogueVoiceComponent> Voice;
	UPROPERTY() TObjectPtr<USoundBase> WarningVoice;
	UPROPERTY() TObjectPtr<USoundBase> ShoutVoice;
	UPROPERTY() TObjectPtr<USoundBase> PitVoice;
	UPROPERTY() TObjectPtr<USoundBase> FallYell;
	UPROPERTY() TObjectPtr<UAnimSequence> Aim;
	UPROPERTY() TObjectPtr<UAnimSequence> Jog;
	UPROPERTY() TObjectPtr<UAnimSequence> Collapse;
	UPROPERTY() TObjectPtr<USoundBase> Shot;
	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	TWeakObjectPtr<APawn> Viewer;
	FVector Doorway = FVector::ZeroVector;
	FVector Timbers = FVector::ZeroVector;
	float HollowHalfWidth = 0;
	float SidestepY = 0;
	float FlashSeconds = 0;
	float Fall = 0;
	float AimFraming = 0;
#if !UE_BUILD_SHIPPING
	float PortraitHold = 0;
#endif
	bool bRunning = false;
	bool bWarned = false;
	bool bJumpWasAllowed = true;
	bool bTurnedWithMovement = false;
	bool bHoldingMark = false;
};
