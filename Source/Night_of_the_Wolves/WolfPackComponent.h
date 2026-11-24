#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WolfPackNavigation.h"
#include "WolfPackTactics.h"
#include "WolfPackComponent.generated.h"

class AHuntingWolf;
class USplineComponent;
class UWolfMeshComponent;
class UPointLightComponent;
class USoundBase;
class USoundAttenuation;

enum class EWolfGroundEncounter : uint8 { Trail, Clearing, NearBank, FarBank };

UCLASS()
class UWolfPackComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	static constexpr int32 Count = 5;
	UWolfPackComponent();
	void SpawnMembers();
	void ResetAttempt();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void RunOpening(USplineComponent& Trail, float AlphaDistance, float Destination, float Dt, bool bRevealLeader);
	void PlaceAtStart(const FVector& PressureFeet, FVector Heading);
	void Hunt(float Dt, FVector TrailGuide, FVector Heading, float HoldDistance, EWolfGroundEncounter Terrain, bool bPermitAttack);
	void HearFootfall(const FVector& Feet, float Force);
	void GuardDoor(float Dt, FVector Door, bool bRepelled, bool bExitExposed=false, bool bBreached=false);
	void ApproachBrokenBridge(float Dt);
	void FaceFire(float Dt, const FVector& BurningBranch);
	void FleeFire(float Dt, const FVector& BurningBranch, bool bAlreadyAtCover=false);
	void PursueDiversion(float Dt, const FVector& Target);
	UWolfMeshComponent* ClaimCapture(const APawn& Max);
	UWolfMeshComponent* GetLeadWolf() const;
	UWolfMeshComponent* GetMember(int32 Index) const;
	UWolfMeshComponent* GetAttacker() const;
	const FWolfPackTactics& GetTactics() const { return Tactics; }
	void FollowCreek(float Dt, const FVector (&Feet)[Count], const FVector (&Heading)[Count], const float (&Peer)[Count], bool bSnap);
	void CoverPounce(float Dt, const FVector& PreyFeet);
	void FlankBandit(float Dt, FVector Target, FVector AttackAxis, float SinceHit);
	UPROPERTY(EditAnywhere, Category="Wolf pack") FWolfPackTuning Tuning;
	UPROPERTY(EditAnywhere, Category="Wolf pack|Senses", meta=(ClampMin="100",Units="cm")) float SightRange = 2350.f;
	UPROPERTY(EditAnywhere, Category="Wolf pack|Movement", meta=(ClampMin="50",Units="cm/s")) float PursuitSpeed = 645.f;
	UPROPERTY(EditAnywhere, Category="Wolf pack|Movement", meta=(ClampMin="350",Units="cm")) float FireRetreatDistance = 1120.f;
	UPROPERTY(EditAnywhere, Category="Wolf pack|Engagement", meta=(ClampMin="50",Units="cm")) float PounceReach = 280.f;
private:
	UPROPERTY(Transient) TArray<TObjectPtr<AHuntingWolf>> Members;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> MoonBounce;
	UPROPERTY() TObjectPtr<USoundBase> LeadCall;
	UPROPERTY(Transient) TObjectPtr<USoundAttenuation> SignalAttenuation;
	FWolfPackNavigation Nav;
	FWolfPackTactics Tactics;
	struct FWalker
	{
		TArray<FVector> ScriptPath;
		FVector ScriptGoal = FVector::ZeroVector;
		float RetryIn = 0.f;
		float BlockTime = 0.f;
		float GroundSpeed = 0.f;
		int32 Cursor = 0;
		int32 Revision = -1;
	};
	FWalker Walkers[Count];
	enum class EMovement : uint8 { Hunt, Guard, Bridge, Creek, Diversion, FireAlert, Fire };
	EMovement Movement = EMovement::Hunt;
	bool bPlaced = false;
	bool bCanCapture = false;
	bool bThreshold = false;
	bool bPlayedSignal = false;
	int32 Attacker = INDEX_NONE;
	FVector DoorFeet = FVector::ZeroVector;
	FVector FireStart[Count];
	float FireClock = 0.f;
	bool AtFireCover[Count] = {};
	float FirePlanAge[Count] = {};
	FVector FirePlanFeet[Count];
	bool FindFireCover(int32 I, const FVector& BurningBranch);
	float BreachProgress = -1.f;
	FVector BreachStart = FVector::ZeroVector;
	void BeginMovement(EMovement Next);
	bool ClearSight(const FVector& Eyes, const FVector& Target, float Radius=0.f) const;
	void MoveGround(int32 I, const TArray<FVector>& Points, float Dt, float Speed, float Threat, const FVector& Watch, bool bSeparate=true);
	void FollowMark(int32 I, const FVector& Mark, const FVector& Watch, float Dt, float Speed, float Threat);
	void Pose(int32 I, const FVector& Next, const FVector& Facing, float Dt, float Threat, bool bSwimming=false, float Peer=0.f, bool bSnap=false);
	void PlaceMoonBounce(const FVector& Ground);
	void Debug(const APawn* Max);
};
