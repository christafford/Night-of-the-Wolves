#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WolfTrailGameMode.generated.h"

UCLASS()
class NIGHT_OF_THE_WOLVES_API AWolfTrailGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AWolfTrailGameMode();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pursuit", meta=(ClampMin="0.1"))
	float IdleCaptureSeconds = 5.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pursuit", meta=(ClampMin="0.0"))
	float ShovelBonusSeconds = 10.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pursuit", meta=(ClampMin="1.0"))
	float WolfSpeed = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pursuit", meta=(ClampMin="100.0"))
	float TrailLength = 2000.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pursuit", meta=(ClampMin="0.1"))
	float DeathDisplaySeconds = 2.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Presentation")
	float WolfCaptureTime = 0.f;
	UPROPERTY(EditDefaultsOnly, Category="Presentation")
	TObjectPtr<class USoundBase> WolfHowls;
	UPROPERTY(EditDefaultsOnly, Category="Presentation")
	TObjectPtr<class USoundBase> ForestWind;
	UPROPERTY(EditDefaultsOnly, Category="Presentation", meta=(ClampMin="0.0", ClampMax="1.0"))
	float WolfHowlVolume = 0.25f;
	UPROPERTY(EditDefaultsOnly, Category="Presentation", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ForestWindVolume = 0.12f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cabin")
	FVector CabinDoorLocation = FVector(2450.f, 0.f, 100.f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cabin", meta=(ClampMin="50.0"))
	float KnockDistance = 200.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cabin", meta=(ClampMin="0.1"))
	float HermitReactionSeconds = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cabin")
	FVector ShovelLocation = FVector(2390.f, 310.f, 65.f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cabin")
	FVector PitLocation = FVector(2260.f, 0.f, 0.f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cabin", meta=(ClampMin="50.0"))
	float ShovelDistance = 150.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cabin", meta=(ClampMin="50.0"))
	float DigDistance = 220.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cabin", meta=(ClampMin="0.1"))
	float HermitRunSeconds = 0.7f;

	UPROPERTY(BlueprintReadOnly, Category="Pursuit")
	bool bWolfCaptured = false;
	UPROPERTY(BlueprintReadOnly, Category="Pursuit")
	bool bReachedTrailEnd = false;
	UPROPERTY(BlueprintReadOnly, Category="Cabin")
	bool bCanKnock = false;
	UPROPERTY(BlueprintReadOnly, Category="Cabin")
	bool bHermitReacting = false;
	UPROPERTY(BlueprintReadOnly, Category="Cabin")
	bool bShotByHermit = false;
	UPROPERTY(BlueprintReadOnly, Category="Cabin")
	bool bHasShovel = false;
	UPROPERTY(BlueprintReadOnly, Category="Cabin")
	bool bPitDug = false;
	UPROPERTY(BlueprintReadOnly, Category="Cabin")
	bool bHermitTrapped = false;
	UPROPERTY(BlueprintReadOnly, Category="Cabin")
	bool bSheltered = false;
	UPROPERTY(BlueprintReadOnly, Category="Cabin")
	FString InteractionPrompt;

	// One entry point for all wolf captures. The M1 HUD is a placeholder.
	UFUNCTION(BlueprintCallable, Category="Pursuit")
	void PresentWolfCapture();

private:
	float WolfPosition = 0.f;
	float ShovelBonusRemaining = 0.f;
	float FurthestProgress = 0.f;
	bool bPursuitInitialized = false;
	FVector StartPosition = FVector::ZeroVector;
	FVector TrailDirection = FVector::ForwardVector;
	FTimerHandle RestartTimer;
	FTimerHandle HermitTimer;
	bool bKnockBound = false;
	bool bHermitRunning = false;
	float HermitRunElapsed = 0.f;
	TWeakObjectPtr<AActor> RunningHermit;
	void Knock();
	void UseShovel();
	void CompleteHermitReaction();
	void HermitShoots();
	void FreezePlayer();
	void RestartAttempt();
#if !UE_BUILD_SHIPPING
	void TickM3SelfTest(float DeltaSeconds);
	void TickShovelTimeTest(float DeltaSeconds);
	int32 SelfTestStep = 0;
	float SelfTestElapsed = 0.f;
	bool bSelfTest = false;
	bool bShovelTimeTest = false;
	float ShovelTestElapsed = 0.f;
	float ShovelTestExpected = 0.f;
#endif
};
