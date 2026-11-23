#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WolfInventory.h"
#include "WolfTrailGameMode.generated.h"

class AFirstLegCrossing;
class AWolfCottageSet;
class ACottageHermit;
class AWolfCaptureCamera;
class AWolfTrailOpening;
class AWolfTrailRoute;
class APointLight;
class USoundBase;
class UAudioComponent;

UCLASS()
class NIGHT_OF_THE_WOLVES_API AWolfTrailGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AWolfTrailGameMode();
	virtual void BeginPlay() override;
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool AllowPausing(APlayerController* PC = nullptr) override;
	bool IsOpeningCinematicPlaying() const { return !bReady || OpeningCamera != nullptr; }
	float GetOpeningTitleOpacity() const;
	float GetOpeningElapsedSeconds() const;

	UPROPERTY(EditDefaultsOnly, Category="Opening")
	TSubclassOf<AWolfTrailOpening> OpeningCameraClass;

	UPROPERTY(EditDefaultsOnly, Category="Opening", meta=(ClampMin="0.1"))
	float IdleCaptureSeconds = 5.f;
	UPROPERTY(EditDefaultsOnly, Category="Opening", meta=(ClampMin="1.0"))
	float WolfSpeed = 115.f;
	UPROPERTY(EditDefaultsOnly, Category="Cottage", meta=(ClampMin="1.0"))
	float ExteriorSeconds = 35.f;
	UPROPERTY(EditDefaultsOnly, Category="Cottage", meta=(ClampMin="1.0"))
	float DoorEnduranceSeconds = 40.f;
	UPROPERTY(EditDefaultsOnly, Category="Cottage", meta=(ClampMin="1.0"))
	float FireDeterrenceSeconds = 36.f;
	UPROPERTY(EditDefaultsOnly, Category="Cottage", meta=(ClampMin="0.1"))
	float DigSeconds = 3.2f;
	UPROPERTY(EditDefaultsOnly, Category="Cottage", meta=(ClampMin="0.1"))
	float HermitReactionSeconds = 1.2f;
	UPROPERTY(EditDefaultsOnly, Category="Cottage", meta=(ClampMin="0.1"))
	float HermitRunSeconds = 1.55f;
	UPROPERTY(EditDefaultsOnly, Category="Cottage", meta=(ClampMin="0.4"))
	float HermitAimSeconds = 2.1f;
	UPROPERTY(EditDefaultsOnly, Category="Presentation", meta=(ClampMin="0.1"))
	float DeathDisplaySeconds = 2.f;
	UPROPERTY(EditDefaultsOnly, Category="Presentation|Wolf capture", meta=(ClampMin="0.15", Units="s"))
	float WolfFlightSeconds = .68f;
	UPROPERTY(EditDefaultsOnly, Category="Presentation|Wolf capture", meta=(ClampMin="0.15", Units="s"))
	float WolfImpactSeconds = .65f;
	UPROPERTY(EditDefaultsOnly, Category="Cottage")
	FVector CabinDoorLocation = FVector(2450,0,100);
	UPROPERTY(EditDefaultsOnly, Category="Cottage")
	FVector ShovelLocation = FVector(2390,310,65);
	UPROPERTY(EditDefaultsOnly, Category="Presentation")
	TObjectPtr<USoundBase> WolfHowls;
	UPROPERTY(EditDefaultsOnly, Category="Presentation")
	TObjectPtr<USoundBase> ForestWind;
	UPROPERTY(EditDefaultsOnly, Category="Presentation")
	float WolfHowlVolume = .25f;
	UPROPERTY(EditDefaultsOnly, Category="Presentation")
	float ForestWindVolume = .12f;

	FWolfInventory Inventory;
	int32 Checkpoint = 0;
	bool bLegComplete = false;
	bool bWolfCaptured = false;
	bool bShotByHermit = false;
	bool bSheltered = false;
	bool bPitDug = false;
	bool bHermitReacting = false;
	bool bHermitTrapped = false;
	float WolfCaptureTime = 0;
	FString DeathCause;
	FString InteractionPrompt;
	FString ThreatCue;

	bool IsDead() const { return !DeathCause.IsEmpty(); }
	bool IsWolfPouncing() const { return bWolfPouncing; }
	bool IsTraversalLocked() const;
	float DiggingTime() const { return DigElapsed; }
	void Interact();
	void UseEquipped();
	void RetryCheckpoint();
	void NewTrail();
	bool CommitCheckpoint(int32 Id, FVector Location, FRotator Facing);
	void Die(const FString& Cause);
	UFUNCTION(BlueprintCallable, Category="Pursuit") void PresentWolfCapture();

private:
	void InitializeTrail();
	void BeginOpening(APawn& Pawn);
	void FinishOpening();
	UPROPERTY(Transient) TObjectPtr<AWolfTrailOpening> OpeningCamera;
	friend class AWolfCaptureCamera;
	void ShowDeathCard();
	UPROPERTY(Transient) TObjectPtr<AWolfCaptureCamera> CaptureCamera;
	bool bWolfPouncing = false;
	friend class UWolfTrailSave;
	UPROPERTY() TObjectPtr<AWolfCottageSet> Cottage;
	UPROPERTY() TObjectPtr<ACottageHermit> Hermit;
	UPROPERTY() TObjectPtr<AFirstLegCrossing> Crossing;
	UPROPERTY() TObjectPtr<UAudioComponent> HowlLoop;
	UPROPERTY() TObjectPtr<UAudioComponent> WindLoop;
	TWeakObjectPtr<AWolfTrailRoute> Route;
	TWeakObjectPtr<APointLight> FireplaceLight;
	TWeakObjectPtr<APointLight> WindowLight;
	FTransform CheckpointTransform;
	FWolfInventory CheckpointItems;
	FTimerHandle RetryTimer;
	bool bReady = false;
	bool bDoorOpen = false;
	bool bDoorwayGuarded = false;
	bool bDoorBreached = false;
	bool bWarnedTable = false;
	float OpeningSeconds = 0;
	float StartDistance = 0;
	float WolfDistance = 0;
	float ExteriorRemaining = 0;
	float DoorRemaining = 0;
	float FleeRemaining = 0;
	float FireRemaining = 0;
	float BestDepartureDistance = 0;
	float DigElapsed = -1;
	float HermitElapsed = -1;
	bool bHermitFired = false;
	bool bHermitShouted = false;
	float DoorImpactIn = 0;
	float NoticeRemaining = 0;
	FString Notice;
	enum class ETarget { None, Shovel, Earth, Knock, Bar, Open, Close, Supplies, Brand };
	ETarget Target(const APawn& Pawn, bool bItem) const;
	void Act(bool bItem);
	void UpdatePrompt(const APawn& Pawn);
	void UpdateCottage(float Dt, APawn& Pawn);
	void UpdatePressure(float Dt, APawn& Pawn);
	void RestoreWorld(int32 Id);
	void SetDoorOpen(bool bOpen);
	void Say(const FString& Text, float Seconds = 2.5f);
	float RouteDistance(const FVector& Position) const;
	bool IsInside(const FVector& Position) const;
#if !UE_BUILD_SHIPPING
	bool bFirstLegTest = false;
	bool bCottageTest = false;
	bool bRecoveryTest = false;
	int32 TestStep = 0;
	float TestElapsed = 0;
	int32 DoorChallengeAttempt = 0;
	float DoorOpenedAt = 0;
	void DriveTest(float Dt, APawn& Pawn);
#endif
};
