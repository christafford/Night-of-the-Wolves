#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FirstLegCrossing.generated.h"

class AWolfTrailGameMode;
class ACharacter;
class ACameraActor;
class APlayerController;
class UWolfPackComponent;
class UWolfMeshComponent;
class USceneComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class USkeletalMeshComponent;
class UAnimSequence;
class USoundBase;
class UAudioComponent;
class UDialogueVoiceComponent;

UCLASS()
class NIGHT_OF_THE_WOLVES_API AFirstLegCrossing : public AActor
{
	GENERATED_BODY()

public:
	AFirstLegCrossing();
	virtual void BeginPlay() override;
	bool TickEncounter(AWolfTrailGameMode& Game, APawn& Pawn, float Delta);
	bool TryAction(AWolfTrailGameMode& Game, APawn& Pawn, bool bItemUse);
	void Restore(int32 Checkpoint, const FVector& SavedPouchSite = FVector::ZeroVector);
	FVector GetGoldLanding() const { return GoldLanding; }
	bool HasSurvivedRobbery() const;
	void InterruptEncounter();
	bool IsTraversalLocked() const;
	bool IsWatchingCreekPack() const;
	UWolfMeshComponent* GetLeadWolf() const;
#if !UE_BUILD_SHIPPING
	void DriveTest(AWolfTrailGameMode& Game, APawn& Pawn, float Delta);
	void StartRobberyTest();
#endif

	UPROPERTY(EditDefaultsOnly, Category="Crossing", meta=(ClampMin="0.1"))
	float CastRopeSeconds = 1.9f;
	UPROPERTY(EditDefaultsOnly, Category="Crossing", meta=(ClampMin="0.5"))
	float TrolleyRideSeconds = 3.0f;
	UPROPERTY(EditDefaultsOnly, Category="Pursuit", meta=(ClampMin="1"))
	float NearBankSeconds = 16.f;
	UPROPERTY(EditDefaultsOnly, Category="Pursuit", meta=(ClampMin="4"))
	float PackRouteSeconds = 26.f;
	UPROPERTY(EditDefaultsOnly, Category="Crossing|Camera", meta=(ClampMin="4", Units="s"))
	float CreekLookbackSeconds = 8.1f;
	UPROPERTY(EditDefaultsOnly, Category="Pursuit", meta=(ClampMin="1"))
	float DiversionSeconds = 9.f;
	UPROPERTY(EditDefaultsOnly, Category="Robbery|Gold", meta=(ClampMin="100",Units="cm/s"))
	float GoldThrowSpeed = 640.f;
	UPROPERTY(EditDefaultsOnly, Category="Robbery|Gold", meta=(ClampMin="50",Units="cm"))
	float PackInterceptWidth = 260.f;
	UPROPERTY(EditDefaultsOnly, Category="Robbery", meta=(ClampMin="0.25",Units="s"))
	float BanditAimSeconds = 1.95f;
	UPROPERTY(EditDefaultsOnly, Category="Robbery", meta=(ClampMin="100",Units="cm"))
	float RobberyMoveAllowance = 760.f;
	UPROPERTY(EditDefaultsOnly, Category="Robbery|Camera", meta=(ClampMin="0.6", Units="s"))
	float RealizationSeconds = 1.15f;
	UPROPERTY(EditDefaultsOnly, Category="Robbery|Camera", meta=(ClampMin="0.4", Units="s"))
	float PullbackSeconds = .78f;

private:
	enum class EPhase { Approach, Ready, Casting, Rigged, Riding, Landing, Missed, FarBank, Demand, Diversion, Escaped, CreekLookback };
	EPhase Phase = EPhase::Approach;
	float ActionTime = 0.f;
	float RideTime = 0.f;
	float RideFraction = 0.f;
	FVector CastingStart = FVector::ZeroVector;
	float BankTime = 0.f;
	float PackTime = 0.f;
	float DiversionTime = 0.f;
	float AimTime = 0.f;
	float TimeUntilBanditFires = -1.f;
	float AttackTime = -1.f;
	float CueTime = 0.f;
	FString Cue;
	bool bPouchLanded = false;
	bool bWolfIntercept = false;
	bool bGoldDemandSpoken = false;
	bool bBanditStruck = false;
	bool bJumpWasAllowed = true;
	FVector PounceStart = FVector::ZeroVector;
	FVector AttackAxis = FVector::ForwardVector;
	FVector CameraSide = FVector::RightVector;
	FRotator CollectingRotation = FRotator::ZeroRotator;
	TWeakObjectPtr<APlayerController> AttackViewer;
	TWeakObjectPtr<APawn> AttackPawn;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> AttackCamera;
	FVector TossOrigin;
	FVector TossVelocity;
	FVector GoldLanding;
	FVector CreekStart[5]{};
	float CreekShotTime = 0.f;
	bool bReturningFromCreek = false;
	bool bCreekJumpAllowed = true;
	static constexpr float CreekHandoff = 8.1f;
	TWeakObjectPtr<APlayerController> CreekViewer;
	TWeakObjectPtr<ACharacter> CreekPawn;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> CreekCamera;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> CreekWakes;
	void BeginCreekLookback(APawn& Pawn);
	void TickCreekLookback(float Delta);
	void EndCreekLookback();
	void FrameCreekPack(float StoryTime);
	void PlaceCreekPack(float StoryTime, float Delta, bool bSnap=false);
	void HideCreekWakes();
	UPROPERTY() TObjectPtr<UStaticMesh> Cube;
	UPROPERTY() TObjectPtr<UStaticMesh> Round;
	UPROPERTY() TObjectPtr<UStaticMesh> Pole;
	UPROPERTY() TObjectPtr<UStaticMesh> GoldPouch;
	UPROPERTY(Transient) TObjectPtr<UWolfPackComponent> Pack;
	UPROPERTY() TObjectPtr<UStaticMesh> Snag;
	UPROPERTY() TObjectPtr<UStaticMesh> Fir;
	UPROPERTY() TObjectPtr<UStaticMesh> Pine;
	UPROPERTY() TObjectPtr<UStaticMesh> Fern;
	UPROPERTY() TObjectPtr<UMaterialInterface> ShapeMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> Bark;
	UPROPERTY() TObjectPtr<UMaterialInterface> Trail;
	UPROPERTY() TObjectPtr<UStaticMesh> CreekValley;
	UPROPERTY() TObjectPtr<UStaticMesh> StreamStoneA;
	UPROPERTY() TObjectPtr<UStaticMesh> StreamStoneB;
	UPROPERTY() TObjectPtr<UStaticMesh> StreamStoneC;
	UPROPERTY() TObjectPtr<UStaticMesh> NurseLog;
	UPROPERTY() TObjectPtr<UMaterialInterface> Water;
	UPROPERTY() TObjectPtr<UMaterialInterface> WarmGlow;
	UPROPERTY() TObjectPtr<UMaterialInterface> RopeMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> FoamMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> BlackMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> GoldMat;
	UPROPERTY() TObjectPtr<UAnimSequence> AimAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> WalkAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> StoopAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> DeathAnim;
	UPROPERTY() TObjectPtr<USoundBase> Shot;
	UPROPERTY() TObjectPtr<USoundBase> Coins;
	UPROPERTY() TObjectPtr<USoundBase> Stream;
	UPROPERTY() TObjectPtr<UAudioComponent> WheelAudio;
	UPROPERTY() TObjectPtr<UAudioComponent> HardwareAudio;
	UPROPERTY() TObjectPtr<USoundBase> EyeClink;
	UPROPERTY() TObjectPtr<USoundBase> StopClank;
	void InitializeCableSounds();
	void StartCableRoll(const FVector& Position);
	void UpdateCableRoll(const FVector& Position, float SpeedFraction);
	void SoundCableLatch(const FVector& Position, bool bBuffer);
	UPROPERTY() TObjectPtr<USoundBase> Howl;
	UPROPERTY() TObjectPtr<USoundBase> DemandVoice;
	UPROPERTY() TObjectPtr<USoundBase> GoldVoice;
	UPROPERTY() TObjectPtr<USoundBase> StayVoice;
	UPROPERTY() TObjectPtr<USoundBase> PainYell;
	UPROPERTY() TObjectPtr<USoundBase> Snarl;
	UPROPERTY() TObjectPtr<UDialogueVoiceComponent> Speech;
	UPROPERTY(Transient) TArray<TObjectPtr<USoundBase>> PendingSpeech;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Bandit;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> BanditFace;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> BanditHair;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> BanditClothes;
	UPROPERTY() TObjectPtr<USceneComponent> BanditRoot;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Cord;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Bag;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SpilledCoins;
	struct FBatch { UStaticMesh* Mesh; UMaterialInterface* Material; bool Solid; bool Visible; FName SurfaceTag; UInstancedStaticMeshComponent* Instances; };
	TArray<FBatch> Batches;
	UStaticMeshComponent* Part(UStaticMesh* Mesh, UMaterialInterface* Material, USceneComponent* Parent=nullptr);
	// Timber stations, standing skyline, rolling sheave and the player's knotted sling.
	UPROPERTY() TObjectPtr<UStaticMesh> SkylineHead;
	UPROPERTY() TObjectPtr<UStaticMesh> SkylineTail;
	UPROPERTY() TObjectPtr<UStaticMesh> SkylineCarriage;
	UPROPERTY() TObjectPtr<UStaticMesh> SkylineSheave;
	UPROPERTY() TObjectPtr<UStaticMesh> SkylineSling;
	UPROPERTY() TObjectPtr<UStaticMesh> SkylineBight;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> CastBight;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CastTail;
	FVector CastOrigin = FVector::ZeroVector;
	void ShowRopeCast(float Progress);
	UPROPERTY() TObjectPtr<UMaterialInterface> SkylineSteel;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Ironwork;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> TrolleyWheel;
	void BuildCableway();
	void SetTrolleyPose(float FractionOnCable, float SlingFraction);
	void Stamp(UStaticMesh* Mesh, UMaterialInterface* Material, FVector Position, FVector Scale,
		FRotator Rotation=FRotator::ZeroRotator, bool bSolid=false, bool bVisible=true, FName SurfaceTag=NAME_None);
	void LogBeam(FVector Start, FVector End, float Radius, UMaterialInterface* Material, bool bSolid=false);
	void BuildBanks();
	void BuildProps();
	void UpdatePack(float Delta, float HoldDistance, bool bPermitAttack);
	UPROPERTY(Transient) TObjectPtr<class UPoseableMeshComponent> HangingPose;
	TWeakObjectPtr<class USpringArmComponent> CableBoom;
	bool bPreviousBoomCollision=true;
	float PreviousBoomLength=400;
	FVector PreviousBoomOffset=FVector::ZeroVector;
	void UpdateCableView(APawn& Pawn, float Delta);
	void RestoreCableView();
	void TraversalMovement(APawn& Pawn,bool bActive);
	void BeginRopePose(ACharacter& Max);
	void UpdateRopePose(ACharacter& Max, float CastProgress);
	void EndRopePose(ACharacter& Max);
	bool IsAboveCatchDeck() const;
	void BeginDismount(APawn& Pawn);
	void AdvanceTrolley(float Delta);
	void UpdateRide(AWolfTrailGameMode& Game, APawn& Pawn, float Delta);
	void UpdateFarBank(AWolfTrailGameMode& Game, APawn& Pawn, float Delta);
	void FinishLanding(AWolfTrailGameMode& Game, APawn& Pawn);
	void MoveBandit(const FVector& Position, const FVector& Facing);
	bool IsBreakingAway(const FVector& Position) const;
	void CoverMax(AWolfTrailGameMode& Game, const APawn& Pawn, float Delta, bool bBreakingAway);
	void StageCue(const FString& Text, float Seconds=2.3f);
	void QueueSpeech(USoundBase* Line);
	void NoticeGold();
	void BeginBanditAttack(APawn& Pawn);
	void AdvanceBanditAttack(float Delta);
	void FrameBanditAttack();
	void SetFallenPose(float Weight);
	void EndBanditShot(bool bBlend);
	float BanditImpactTime() const;
	float BanditShotEnd() const;
	void ThrowPouch(const APawn& Pawn);
	void AdvancePouch(const APawn& Pawn, float Delta);
	bool IsOnPackApproach() const;
	bool IsInsideWater(const FVector& Position) const;
#if !UE_BUILD_SHIPPING
	float TestTime = 0.f;
	int32 TestWaypoint = 0;
#endif
};
