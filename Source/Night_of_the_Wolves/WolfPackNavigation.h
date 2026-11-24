#pragma once

#include "CoreMinimal.h"

class AWolfTrailGameMode;
class UWorld;

struct FWolfNavigationPath
{
	// Supported feet positions, excluding the origin. The last point is always the requested goal.
	TArray<FVector> Points;
	float Length = 0.f;
	bool bAlternate = false;
};

struct FWolfNavStats
{
	uint64 Searches = 0;
	uint64 Expanded = 0;
	uint64 PhysicsQueries = 0;
	uint64 BudgetStops = 0;
};

// A short-range surface planner for the runtime-built terrain. One instance per pack, game thread only.
class FWolfPackNavigation
{
public:
	void Reset(UWorld* InWorld);
	void SetCabinAllowed(bool bAllowed) { bCabinAllowed = bAllowed; }
	bool ProjectFeet(const FVector& Hint, FVector& Out);
	bool CanTraverse(const FVector& From, const FVector& To, FVector* GroundedEnd = nullptr);
	bool FindPath(const FVector& From, const FVector& To, FWolfNavigationPath& Out, bool bWantAlternate = false);
	bool FindPathVia(const FVector& From, const FVector& Via, const FVector& To, FWolfNavigationPath& Out);
	const FWolfNavStats& GetStats() const { return Stats; }

private:
	struct FQuery;
	bool Ground(FQuery& Q, const FVector& Hint, FVector& Out, bool bCheckBody);
	bool Segment(FQuery& Q, const FVector& From, const FVector& To, FVector& OutEnd);
	bool Plan(FQuery& Q, const FVector& From, const FVector& To, FWolfNavigationPath& Out,
		const FVector* Avoid = nullptr, float AvoidRadius = 0.f);
	bool ViaRoute(FQuery& Q, const FVector& From, const FVector& Via, const FVector& To, FWolfNavigationPath& Out);
	bool Alternate(FQuery& Q, const FVector& From, const FVector& To, const FWolfNavigationPath& Primary, FWolfNavigationPath& Out);

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AWolfTrailGameMode> Game;
	FWolfNavStats Stats;
	bool bCabinAllowed = false;
};
