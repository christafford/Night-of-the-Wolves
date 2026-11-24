#pragma once

#include "CoreMinimal.h"
#include "WolfPackTactics.generated.h"

USTRUCT(BlueprintType)
struct FWolfPackTuning
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, Category="Senses", meta=(ClampMin="0.02", Units="s")) float SenseInterval = .045f;
	UPROPERTY(EditAnywhere, Category="Senses", meta=(ClampMin="0.1", Units="s")) float FreshSightSeconds = .65f;
	UPROPERTY(EditAnywhere, Category="Senses", meta=(ClampMin="0.2", Units="s")) float MemorySeconds = 3.6f;
	UPROPERTY(EditAnywhere, Category="Plans", meta=(ClampMin="0.02", Units="s")) float DecisionInterval = .075f;
	UPROPERTY(EditAnywhere, Category="Plans", meta=(ClampMin="0.1", Units="s")) float CommitmentSeconds = .70f;
	UPROPERTY(EditAnywhere, Category="Plans", meta=(ClampMin="0.2", Units="s")) float FailedRouteCooldown = 1.65f;
	UPROPERTY(EditAnywhere, Category="Plans", meta=(ClampMin="0.5", Units="s")) float PredictionSeconds = 2.4f;
	UPROPERTY(EditAnywhere, Category="Plans", meta=(ClampMin="0", Units="s")) float InterceptSlackSeconds = .60f;
	UPROPERTY(EditAnywhere, Category="Search", meta=(ClampMin="2")) int32 StationsPerSweep = 3;
	UPROPERTY(EditAnywhere, Category="Search", meta=(ClampMin="4")) int32 TrackerReturnAfter = 7;
	UPROPERTY(EditAnywhere, Category="Plans", meta=(ClampMin="0.1", Units="s")) float SignalCooldown = 4.f;
	UPROPERTY(EditAnywhere, Category="Space", meta=(ClampMin="30", Units="cm")) float Spacing = 112.f;
	UPROPERTY(EditAnywhere, Category="Space", meta=(ClampMin="60", Units="cm")) float StandOff = 172.f;
	UPROPERTY(EditAnywhere, Category="Space", meta=(ClampMin="80", Units="cm")) float FlankWidth = 245.f;
	UPROPERTY(EditAnywhere, Category="Roles", meta=(ClampMin="0", Units="cm")) float PressurePreference = 410.f;
	UPROPERTY(EditAnywhere, Category="Roles", meta=(ClampMin="0", Units="cm")) float FlankerReliefPreference = 285.f;
	UPROPERTY(EditAnywhere, Category="Roles", meta=(ClampMin="20", Units="cm/s")) float ScoutMinimumQuarrySpeed = 105.f;
	UPROPERTY(EditAnywhere, Category="Engagement", meta=(ClampMin="1", ClampMax="5")) int32 MaxEngagements = 1;
	UPROPERTY(EditAnywhere, Category="Engagement", meta=(ClampMin="0", Units="s")) float EngagementCooldown = .40f;
};

enum class EWolfAssignment : uint8 { Follow, Coordinate, Pressure, Flank, Intercept, Track, Search, Regroup };
enum class EWolfKnowledge : uint8 { None, Sight, Footfall };
enum class EWolfIntention : uint8 { Pursue, Investigate, Search };

struct FWolfTacticRoute
{
	// Grounded waypoints exclude the start and include the destination.
	TArray<FVector> Points;
	float Length = 0.f;
	bool bAlternate = false;
};

struct FWolfTacticFrame
{
	FVector Feet[5]{};
	float Speed[5]{685.f,760.f,730.f,770.f,700.f};
	bool bBlocked[5]{};
	FVector Forward = FVector::ForwardVector;
	FVector TrailReference = FVector::ZeroVector;
	float PressureLead = 0.f;
	bool bAllowEngagement = false;
	bool bAllowFlank = true;
	bool bAllowIntercept = true;
};

struct FWolfSearchSweep
{
	TOptional<FVector> Center;
	int32 Bearing = 0;
	int32 Arrivals = 0;
	int32 ReachedRevision = INDEX_NONE;
	int32 LastReturn = 0;
	bool bReturning = false;
};

struct FWolfTacticMember
{
	EWolfAssignment Assignment = EWolfAssignment::Follow;
	FWolfTacticRoute Route;
	FVector Destination = FVector::ZeroVector;
	FString Reason = TEXT("forming up");
	int32 Revision = 0;
	bool bEngaging = false;
	float LastSightTime = -BIG_NUMBER;
	float CommittedUntil = 0.f;
	float SuppressedUntil = 0.f;
	float EngagementReady = 0.f;
	float LastPlan = -BIG_NUMBER;
	FWolfSearchSweep Sweep;
	bool bWasBlocked = false;
};

using FWolfSenseQuery = TFunctionRef<bool(int32, FVector&)>;
using FWolfRouteQuery = TFunctionRef<bool(int32,const FVector&,const FVector&,bool,FWolfTacticRoute&)>;

struct FWolfPackTactics
{
	static constexpr int32 Count = 5;
	FWolfTacticMember Wolves[Count];
	EWolfKnowledge Knowledge = EWolfKnowledge::None;
	EWolfIntention Intention = EWolfIntention::Search;
	FVector LastKnown = FVector::ZeroVector;
	FVector ObservedVelocity = FVector::ZeroVector;
	FVector PredictedTarget = FVector::ZeroVector;
	float BeliefAge = BIG_NUMBER;
	int32 LastObserver = INDEX_NONE;
	int32 PressureWolf = 1;
	int32 CueWolf = INDEX_NONE;
	int32 CueReceiver = INDEX_NONE;
	float CueTime = 0.f;
	uint64 DecisionCount = 0, RouteQueryCount = 0, ObservationCount = 0;

	void Reset();
	void ReleaseAssignments();
	void AdvanceWithoutSight(float Dt, const FWolfPackTuning& Tune);
	bool ReserveEncounterAttack(int32 Wolf);
	void Hear(const FVector& Feet, int32 Listener);
	float Elapsed() const { return Time; }
	void Tick(float Dt, const FWolfTacticFrame& Frame, const FWolfPackTuning& Tuning, FWolfSenseQuery Sense, FWolfRouteQuery FindRoute);
	bool CanAttack(int32 Wolf) const;
	static const TCHAR* IdentityName(int32 Wolf);
	static const TCHAR* AssignmentName(EWolfAssignment Task);
	static const TCHAR* IntentionName(EWolfIntention Intent);

private:
	float FreshSight = .65f, NoiseTime = -BIG_NUMBER, AttackCooldown = .4f;
	float Time = 0, NextSight = 0, NextPlan = 0, NextSignal = 0, LastSeen = -BIG_NUMBER;
	int32 SenseCursor = 0, PlanCursor = 1;
	FVector KnownHeading = FVector::ForwardVector;
	FVector SearchOrigin = FVector::ZeroVector;
	bool bStarted = false;
	void Release(int32 Wolf, const TCHAR* Why, const FWolfPackTuning& Tune, bool bFailed);
	void Observe(const FVector& Feet, int32 Observer, const FWolfPackTuning& Tune);
	void Plan(int32 Wolf, const FWolfTacticFrame& F, const FWolfPackTuning& Tune, FWolfRouteQuery Find);
	void Signal(const FWolfTacticFrame& F, const FWolfPackTuning& Tune, int32 Receiver = INDEX_NONE);
};
