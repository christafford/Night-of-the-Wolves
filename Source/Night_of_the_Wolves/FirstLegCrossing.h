#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FirstLegCrossing.generated.h"

class AWolfTrailGameMode;
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

UCLASS()
class NIGHT_OF_THE_WOLVES_API AFirstLegCrossing : public AActor
{
	GENERATED_BODY()

public:
	AFirstLegCrossing();
	virtual void BeginPlay() override;
	bool TickEncounter(AWolfTrailGameMode& Game, APawn& Pawn, float Delta);
	bool TryAction(AWolfTrailGameMode& Game, APawn& Pawn, bool bItemUse);
	void Restore(int32 Checkpoint);
	bool IsTraversalLocked() const;
	UWolfMeshComponent* GetLeadWolf() const;
#if !UE_BUILD_SHIPPING
	void DriveTest(AWolfTrailGameMode& Game, APawn& Pawn, float Delta);
#endif

	UPROPERTY(EditDefaultsOnly, Category="Crossing", meta=(ClampMin="0.1"))
	float RopeCastSeconds = 2.3f;
	UPROPERTY(EditDefaultsOnly, Category="Crossing", meta=(ClampMin="0.5"))
	float OutwardSwingSeconds = 1.85f;
	UPROPERTY(EditDefaultsOnly, Category="Pursuit", meta=(ClampMin="1"))
	float NearBankSeconds = 16.f;
	UPROPERTY(EditDefaultsOnly, Category="Pursuit", meta=(ClampMin="4"))
	float PackRouteSeconds = 26.f;
	UPROPERTY(EditDefaultsOnly, Category="Pursuit", meta=(ClampMin="1"))
	float DiversionSeconds = 9.f;

private:
	enum class EPhase { Approach, Ready, Casting, Attached, Swinging, Landing, Missed, FarBank, Demand, Diversion, Escaped };
	EPhase Phase = EPhase::Approach;
	float ActionTime = 0.f;
	float BankTime = 0.f;
	float PackTime = 0.f;
	float DiversionTime = 0.f;
	float AimTime = 0.f;
	float AttackTime = -1.f;
	float CueTime = 0.f;
	FString Cue;
	bool bPouchLanded = false;
	bool bGoldDemandSpoken = false;
	bool bReachedBend = false;
	FVector ReleaseLocation;
	FVector TossOrigin;
	FVector GoldLanding;
	FVector AttackOrigin;
	FVector LastLead;
	UPROPERTY() TObjectPtr<UStaticMesh> Cube;
	UPROPERTY() TObjectPtr<UStaticMesh> Round;
	UPROPERTY() TObjectPtr<UStaticMesh> Pole;
	UPROPERTY() TObjectPtr<UStaticMesh> GoldPouch;
	UPROPERTY(VisibleAnywhere, Category="Pack") TObjectPtr<UWolfPackComponent> Pack;
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
	UPROPERTY() TObjectPtr<UMaterialInterface> MoonGlow;
	UPROPERTY() TObjectPtr<UMaterialInterface> RopeMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> FoamMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> BlackMat;
	UPROPERTY() TObjectPtr<UMaterialInterface> GoldMat;
	UPROPERTY() TObjectPtr<USkeletalMesh> BanditMesh;
	UPROPERTY() TObjectPtr<UAnimSequence> AimAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> WalkAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> StoopAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> DeathAnim;
	UPROPERTY() TObjectPtr<USoundBase> Shot;
	UPROPERTY() TObjectPtr<USoundBase> Coins;
	UPROPERTY() TObjectPtr<USoundBase> Stream;
	UPROPERTY() TObjectPtr<USoundBase> Howl;
	UPROPERTY() TObjectPtr<USoundBase> DemandVoice;
	UPROPERTY() TObjectPtr<USoundBase> GoldVoice;
	UPROPERTY() TObjectPtr<USoundBase> StayVoice;
	UPROPERTY() TObjectPtr<UAudioComponent> Speech;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Bandit;
	UPROPERTY() TObjectPtr<USceneComponent> BanditRoot;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Cord;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Bag;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TargetRing;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SpilledCoins;
	struct FBatch { UStaticMesh* Mesh; UMaterialInterface* Material; bool Solid; bool Visible; UInstancedStaticMeshComponent* Instances; };
	TArray<FBatch> Batches;
	UStaticMeshComponent* Part(UStaticMesh* Mesh, UMaterialInterface* Material, USceneComponent* Parent=nullptr);
	void Stamp(UStaticMesh* Mesh, UMaterialInterface* Material, FVector Position, FVector Scale,
		FRotator Rotation=FRotator::ZeroRotator, bool bSolid=false, bool bVisible=true);
	void LogBeam(FVector Start, FVector End, float Radius, UMaterialInterface* Material, bool bSolid=false);
	void BuildBanks();
	void BuildProps();
	void ShowRope(const FVector& End);
	void UpdatePack(float Delta, const FVector& Lead, const FVector& Quarry, bool bSwimming=false);
	UPROPERTY(Transient) TObjectPtr<class UPoseableMeshComponent> HangingPose;
	bool bPreviousBoomCollision=true;
	float PreviousBoomLength=400;
	FVector PreviousBoomOffset=FVector::ZeroVector;
	void TraversalMovement(APawn& Pawn,bool bActive);
	void UpdateSwing(AWolfTrailGameMode& Game, APawn& Pawn, float Delta);
	void UpdateFarBank(AWolfTrailGameMode& Game, APawn& Pawn, float Delta);
	void FinishLanding(AWolfTrailGameMode& Game, APawn& Pawn);
	void MoveBandit(const FVector& Position, const FVector& Facing);
	void StageCue(const FString& Text, float Seconds=2.3f);
	void Speak(USoundBase* Line);
	void SetRing(bool bVisible);
	bool IsAimingAtGold(const APawn& Pawn) const;
	bool IsInsideWater(const FVector& Position) const;
#if !UE_BUILD_SHIPPING
	float TestTime = 0.f;
	int32 TestWaypoint = 0;
#endif
};
