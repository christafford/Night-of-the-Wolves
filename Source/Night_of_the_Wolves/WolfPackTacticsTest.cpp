#if !UE_BUILD_SHIPPING

#include "WolfPackTactics.h"
#include "WolfPackNavigation.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

namespace
{
void TestPackTactics(UWorld* World)
{
	if (!World || !World->IsGameWorld()) return;
	// The fixture lives outside the playable map. Feed sight, a single audible
	// footfall and blocked reports explicitly; the routes and feet use real collision.
	const FVector Base(-47000.,68000.,5200.);
	auto* Stage=World->SpawnActor<AActor>();
	auto Box=[&](FVector At,FVector Extent)
	{
		auto* Part=NewObject<UBoxComponent>(Stage);
		Part->InitBoxExtent(Extent);
		Part->SetWorldLocation(Base+At);
		Part->SetCollisionProfileName(TEXT("BlockAll"));
		Part->RegisterComponent();
	};
	Box({0,0,-20},{3400,1600,20});
	Box({0,0,115},{82,155,115});
	FWolfPackNavigation Nav;
	Nav.Reset(World);
	FWolfPackTactics Pack;
	FWolfPackTuning Tune;
	FWolfTacticFrame F;
	F.TrailReference=Base;
	F.PressureLead=45.f;
	F.bAllowEngagement=true;
	constexpr float Dt=.05f;
	static_assert(FWolfPackTactics::Count==5);
	int32 Cursor[5]{}, SeenRevision[5]{};
	double Distance[5]{}, SpecialtyTravel=0.;
	int32 Peaks=0,Reliefs=0,SearchFrames=0,HearingFrames=0,Reacquisitions=0;
	uint64 Queries=0,Decisions=0,SightChecks=0;
	float MaxFrozenDrift=0.f;
	FString Failure;
	auto Check=[&](bool Pass,const TCHAR* Reason){if (!Pass && Failure.IsEmpty()) Failure=Reason;};
	auto Route=[&](int32 Wolf,const FVector& From,const FVector& To,bool bAlternate,FWolfTacticRoute& Out)
	{
		FWolfNavigationPath Path;
		if (!Nav.FindPath(From,To,Path,bAlternate)) return false;
		Out.Points=MoveTemp(Path.Points);
		Out.Length=Path.Length;
		Out.bAlternate=Path.bAlternate;
		return true;
	};
	auto Advance=[&](int32 I) -> double
	{
		const auto& Member=Pack.Wolves[I];
		if (SeenRevision[I]!=Member.Revision) {Cursor[I]=0; SeenRevision[I]=Member.Revision;}
		if (F.bBlocked[I] || !Member.Route.Points.IsValidIndex(Cursor[I])) return 0.;
		const FVector From=F.Feet[I], Waypoint=Member.Route.Points[Cursor[I]];
		const FVector Next=From+(Waypoint-From).GetClampedToMaxSize(F.Speed[I]*Dt);
		FVector Ground;
		if (!Nav.CanTraverse(From,Next,&Ground)) return 0.;
		F.Feet[I]=Ground;
		if (FVector::DistSquared2D(Ground,Waypoint)<FMath::Square(2.f)) ++Cursor[I];
		return FVector::Dist2D(From,Ground);
	};
	for (int32 Leg=0;Leg<3 && Failure.IsEmpty();++Leg)
	{
		Pack.Reset();
		Nav.Reset(World);
		for (int32 I=0;I<5;++I)
		{
			Check(Pack.Wolves[I].Route.Points.IsEmpty() && !Pack.CanAttack(I) && Pack.Wolves[I].Revision==0,TEXT("retry kept a route or attack"));
			F.Feet[I]=Base+FVector(-1620.-68.*I, (I%2 ? 1.:-1.)*(185.+I*22.),0);
			Cursor[I]=SeenRevision[I]=0;
		}
		FVector LastSight;
		bool bRelieved=false,bReacquired=false;
		for (int32 Step=0;Step<1200 && Failure.IsEmpty();++Step)
		{
			const float T=Step*Dt;
			// The player runs back and forth on the north side of the rock.
			const float Cycle=FMath::Fmod(T,16.f);
			const FVector Target=Base+FVector(-1250.+(Cycle<=8.f?Cycle:16.f-Cycle)*305.,260.,0);
			const bool bHidden=T>=24.f && T<36.f;
			const bool bNarrow=T>=46.f && T<53.f;
			F.bAllowFlank=F.bAllowIntercept=!bNarrow;
			F.bBlocked[0]=F.bBlocked[1]=T>=10.f && T<16.f;
			auto Sense=[&](int32 Wolf,FVector& Out)
			{
				if (bHidden) return false;
				Out=Target;
				return true;
			};

			if (Step==480) LastSight=Pack.LastKnown;
			if (Step==648) Pack.Hear(Target,4);
			const uint64 Before=Pack.RouteQueryCount;
			Pack.Tick(Dt,F,Tune,Sense,Route);
			Check(Pack.RouteQueryCount-Before<=5,TEXT("one update exceeded the route budget"));
			int32 Engaged=0;
			for (int32 I=0;I<5;++I)
			{
				const auto& M=Pack.Wolves[I];
				if (Pack.CanAttack(I)) ++Engaged;
				if (bNarrow) Check(M.Assignment!=EWolfAssignment::Intercept && M.Assignment!=EWolfAssignment::Flank,TEXT("narrow lane retained a wide approach"));
				const double Moved=Advance(I);
				Distance[I]+=Moved;
				if (M.Assignment==EWolfAssignment::Flank || M.Assignment==EWolfAssignment::Intercept) SpecialtyTravel+=Moved;
			}
			Peaks=FMath::Max(Engaged,Peaks);
			Check(Engaged<=1,TEXT("simultaneous lethal reservations"));
			if (T>=11.f && T<16.f && Pack.PressureWolf!=0 && Pack.PressureWolf!=1 && Pack.Wolves[Pack.PressureWolf].Assignment==EWolfAssignment::Pressure)
				bRelieved=true;
			if (T>=25.f && T<32.4f)
			{
				MaxFrozenDrift=FMath::Max(MaxFrozenDrift,float(FVector::Distance(Pack.LastKnown,LastSight)));
				Check(!Engaged,TEXT("lost sight retained a lethal reservation"));
				if (Pack.Intention==EWolfIntention::Search && Pack.Wolves[4].Assignment==EWolfAssignment::Search) ++SearchFrames;
			}
			if (T>=32.5f && T<35.f && Pack.Knowledge==EWolfKnowledge::Footfall)
			{
				++HearingFrames;
				Check(Pack.ObservedVelocity.IsNearlyZero() && !Engaged,TEXT("hearing supplied velocity or authorized capture"));
			}
			if (T>36.5f && Pack.Intention==EWolfIntention::Pursue) bReacquired=true;
		}
		Reliefs+=bRelieved;
		Reacquisitions+=bReacquired;
		Queries+=Pack.RouteQueryCount;
		Decisions+=Pack.DecisionCount;
		SightChecks+=Pack.ObservationCount;
		const FVector Remembered=Pack.LastKnown;
		Pack.ReleaseAssignments();
		Check(Pack.LastKnown==Remembered,TEXT("scene staging erased the shared observation"));
		Check(Pack.ReserveEncounterAttack(2) && Pack.ReserveEncounterAttack(4) && !Pack.CanAttack(2) && Pack.CanAttack(4),TEXT("door commitment was not exclusive"));
		Check(!Pack.ReserveEncounterAttack(2),TEXT("displaced commitment bypassed cooldown"));
		Pack.AdvanceWithoutSight(5.f,Tune);
		Check(!Pack.CanAttack(4) && Pack.Intention==EWolfIntention::Search,TEXT("scripted staging retained attack or fresh sight"));
	}
	Check(Peaks==1 && Reliefs==3,TEXT("pressure or relief never engaged"));
	Check(SearchFrames>0 && HearingFrames>0 && Reacquisitions==3 && MaxFrozenDrift<.1f,TEXT("lost sight, footfall-only knowledge, search or reacquisition failed"));
	Check(SpecialtyTravel>600.f,TEXT("support did not travel a different approach"));
	for (double D:Distance) Check(D>9000.,TEXT("a wolf stalled during sustained pursuit"));
	// A minute of total sensory loss. No heading, position or footfall from
	// the hidden player reaches the coordinator after the two-second sighting.
	Pack.Reset();
	for (int32 I=0; I<5; ++I)
	{
		F.Feet[I]=Base+FVector(-2200.-35.*I,255.+45.*I,0);
		F.bBlocked[I]=false;
		Cursor[I]=SeenRevision[I]=0;
	}
	F.bAllowFlank=F.bAllowIntercept=F.bAllowEngagement=true;
	FVector Frozen;
	double Farthest=0., TrackerRetrace=0.;
	int32 TrackerReturns=0;
	bool bPreviouslyReturning=false;
	for (int32 Frame=0; Frame<1240 && Failure.IsEmpty(); ++Frame)
	{
		const float T=Frame*Dt;
		auto Sense=[&](int32 I,FVector& Seen)
		{
			if (Frame>=40) return false;
			Seen=Base+FVector(-2070.+222.*T,330.,0);
			return true;
		};
		if (Frame==40) Frozen=Pack.LastKnown;
		const auto Before=Pack.RouteQueryCount;
		Pack.Tick(Dt,F,Tune,Sense,Route);
		Check(Pack.RouteQueryCount-Before<=5,TEXT("expanding search exceeded route budget"));
		for (int32 I=0; I<5; ++I)
		{
			const double OldRange=FVector::Dist2D(F.Feet[I],Frozen);
			Advance(I);
			if (Frame<60) continue;
			const double Range=FVector::Dist2D(F.Feet[I],Frozen);
			Farthest=FMath::Max(Farthest,Range);
			Check(!Pack.CanAttack(I) && Pack.LastKnown.Equals(Frozen,.01),TEXT("hidden long search fabricated a sighting or attack"));
			if (I==4)
			{
				const bool Returning=Pack.Wolves[I].Sweep.bReturning;
				if (Returning) TrackerRetrace+=FMath::Max(0.,OldRange-Range);
				else if (bPreviouslyReturning && Range<125.) ++TrackerReturns;
				bPreviouslyReturning=Returning;
			}
		}
	}
	Check(Farthest>2000. && TrackerReturns>0 && TrackerRetrace>430. && Pack.BeliefAge>=59.,
		TEXT("search stayed near the last observation or tracker never returned to it"));
	UE_LOG(LogTemp,Display,TEXT("WOLF LONG LOSS: %.1fs without sight or sound, farthest search %.0fcm from frozen belief, tracker retraced %.0fcm and returned %d times"),
		Pack.BeliefAge,Farthest,TrackerRetrace,TrackerReturns);
	Stage->Destroy();
	if (Failure.IsEmpty())
	{
		UE_LOG(LogTemp,Display,TEXT("WOLF TACTICS SOAK PASSED: 180 simulated seconds, %llu bounded decisions, %llu complete-route requests, %llu sight checks, %d reliefs; peak engagements %d; lost-sight drift %.1fcm; support route travel %.0fcm; five distances %.0f/%.0f/%.0f/%.0f/%.0fcm"),
			Decisions,Queries,SightChecks,Reliefs,Peaks,MaxFrozenDrift,SpecialtyTravel,Distance[0],Distance[1],Distance[2],Distance[3],Distance[4]);
	}
	else
	{
		UE_LOG(LogTemp,Error,TEXT("WOLF TACTICS SOAK FAILED: %s; relief %d, peak %d, search %d, hearing %d, drift %.1f, support travel %.0f, five distances %.0f/%.0f/%.0f/%.0f/%.0fcm"),
		*Failure,Reliefs,Peaks,SearchFrames,HearingFrames,MaxFrozenDrift,SpecialtyTravel,Distance[0],Distance[1],Distance[2],Distance[3],Distance[4]);
	}
	FPlatformMisc::RequestExitWithStatus(false,Failure.IsEmpty()?0:1);
}

FAutoConsoleCommandWithWorld TacticsSoak(TEXT("Wolf.TestPackTactics"),TEXT("Exercise five-wolf decisions and collision-supported travel for three simulated minutes"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&TestPackTactics));
}

#endif
