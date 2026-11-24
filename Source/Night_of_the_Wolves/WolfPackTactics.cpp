#include "WolfPackTactics.h"

namespace
{
	constexpr int32 Leader = 0, PressureBeta = 1, Flanker = 2, Interceptor = 3, Tracker = 4;
	constexpr float Arrival = 62.f;

	FVector Horizontal(const FVector& Vector, const FVector& Fallback)
	{
		return Vector.GetSafeNormal2D(SMALL_NUMBER, Fallback);
	}
}

const TCHAR* FWolfPackTactics::IdentityName(int32 Wolf)
{
	static const TCHAR* Names[] = {TEXT("Leader"),TEXT("Pressure"),TEXT("Flanker"),TEXT("Interceptor"),TEXT("Tracker")};
	return Wolf >= 0 && Wolf < Count ? Names[Wolf] : TEXT("None");
}

const TCHAR* FWolfPackTactics::AssignmentName(EWolfAssignment Task)
{
	static const TCHAR* Names[] = {TEXT("follow"),TEXT("coordinate"),TEXT("pressure"),TEXT("flank"),TEXT("intercept"),TEXT("track"),TEXT("search"),TEXT("regroup")};
	return Names[static_cast<int32>(Task)];
}

const TCHAR* FWolfPackTactics::IntentionName(EWolfIntention Intent)
{
	static const TCHAR* Names[] = {TEXT("pursue"),TEXT("investigate"),TEXT("search")};
	return Names[static_cast<int32>(Intent)];
}

void FWolfPackTactics::Reset()
{
	*this = FWolfPackTactics();
}

void FWolfPackTactics::ReleaseAssignments()
{
	for (auto& Member : Wolves)
	{
		const float Sight = Member.LastSightTime;
		const int32 Revision = Member.Revision+1;
		Member = FWolfTacticMember();
		Member.LastSightTime = Sight;
		Member.Revision = Revision;
	}
	PredictedTarget = FVector::ZeroVector;
	PressureWolf = PressureBeta;
	PlanCursor = PressureWolf;
	CueTime = 0.f;
	CueWolf = CueReceiver = INDEX_NONE;
	NextPlan = Time;
}

void FWolfPackTactics::AdvanceWithoutSight(float Dt, const FWolfPackTuning& Tune)
{
	Time += FMath::Max(0.f,Dt);
	FreshSight = Tune.FreshSightSeconds;
	AttackCooldown = Tune.EngagementCooldown;
	CueTime = FMath::Max(0.f,CueTime-Dt);
	BeliefAge = Knowledge == EWolfKnowledge::None ? BIG_NUMBER : Time-(Knowledge == EWolfKnowledge::Sight ? LastSeen : NoiseTime);
	Intention = Knowledge == EWolfKnowledge::Sight && BeliefAge <= FreshSight ? EWolfIntention::Pursue :
		BeliefAge <= Tune.MemorySeconds ? EWolfIntention::Investigate : EWolfIntention::Search;
	for (int32 I=0; I<Count; ++I)
		if (Wolves[I].bEngaging && Time-Wolves[I].LastPlan > .8f)
			Release(I,TEXT("encounter commitment expired"),Tune,false);
}

bool FWolfPackTactics::ReserveEncounterAttack(int32 Wolf)
{
	if (Wolf < 0 || Wolf >= Count || Time < Wolves[Wolf].EngagementReady || Wolves[Wolf].bWasBlocked) return false;
	for (int32 I=0; I<Count; ++I)
	{
		auto& Member = Wolves[I];
		if (I!=Wolf && Member.bEngaging)
		{
			Member.bEngaging = false;
			Member.Route = FWolfTacticRoute();
			Member.Assignment = EWolfAssignment::Regroup;
			Member.Destination = FVector::ZeroVector;
			Member.Reason = TEXT("yielding the encounter attack");
			Member.EngagementReady = Time+AttackCooldown;
			++Member.Revision;
		}
	}
	auto& Chosen = Wolves[Wolf];
	if (!Chosen.bEngaging) ++Chosen.Revision;
	Chosen.bEngaging = true;
	Chosen.Assignment = EWolfAssignment::Pressure;
	Chosen.LastPlan = Time;
	Chosen.Reason = TEXT("clear encounter contact");
	return true;
}

bool FWolfPackTactics::CanAttack(int32 Wolf) const
{
	return Wolf >= 0 && Wolf < Count && Wolves[Wolf].bEngaging;
}

void FWolfPackTactics::Hear(const FVector& Feet, int32 Listener)
{
	if (Listener < 0 || Listener >= Count || Time-LastSeen <= FreshSight || Time-NoiseTime < .35f) return;
	if (!bStarted) return;
	// A sound can suggest the next place to investigate. It supplies no
	// measured heading, and never permits a prediction or attack commitment.
	const FVector Rounded(FMath::GridSnap(Feet.X,100.),FMath::GridSnap(Feet.Y,100.),FMath::GridSnap(Feet.Z,25.));
	if (Knowledge!=EWolfKnowledge::Footfall || FVector::Dist2D(LastKnown,Rounded)>140.)
		for (auto& Wolf : Wolves) Wolf.Sweep = {};
	LastKnown = Rounded;
	LastObserver = Listener;
	NoiseTime = Time;
	Knowledge = EWolfKnowledge::Footfall;
	ObservedVelocity = FVector::ZeroVector;
	BeliefAge = 0.f;
}

void FWolfPackTactics::Release(int32 Wolf, const TCHAR* Why, const FWolfPackTuning& Tune, bool bFailed)
{
	auto& Member = Wolves[Wolf];
	if (CueReceiver==Wolf) CueReceiver=INDEX_NONE;
	if (Member.Assignment==EWolfAssignment::Intercept) PredictedTarget = FVector::ZeroVector;
	if (Member.bEngaging) Member.EngagementReady = Time + Tune.EngagementCooldown;
	Member.bEngaging = false;
	Member.Route = FWolfTacticRoute();
	Member.Destination = FVector::ZeroVector;
	Member.Assignment = EWolfAssignment::Regroup;
	Member.Reason = Why;
	Member.CommittedUntil = 0.f;
	if (bFailed) Member.SuppressedUntil = Time + Tune.FailedRouteCooldown;
	++Member.Revision;
}

void FWolfPackTactics::Observe(const FVector& Feet, int32 Observer, const FWolfPackTuning& Tune)
{
	const float Gap = Time-LastSeen;
	if (Knowledge == EWolfKnowledge::Sight && Gap >= .012f && Gap <= Tune.FreshSightSeconds)
	{
		const FVector Measured = ((Feet-LastKnown) / Gap).GetClampedToMaxSize2D(1000.);
		const bool bTurnedAway = ObservedVelocity.SizeSquared2D() > FMath::Square(Tune.ScoutMinimumQuarrySpeed) &&
			Measured.SizeSquared2D() > FMath::Square(Tune.ScoutMinimumQuarrySpeed) &&
			FVector::DotProduct(Measured.GetSafeNormal2D(),ObservedVelocity.GetSafeNormal2D()) < .20;
		if (bTurnedAway && Wolves[Interceptor].Assignment == EWolfAssignment::Intercept)
			Release(Interceptor, TEXT("quarry turned away from the intercept"), Tune, true);
		ObservedVelocity = bTurnedAway ? Measured : FMath::Lerp(ObservedVelocity, Measured, 1.f-FMath::Exp(-Gap*9.f));
		ObservedVelocity.Z = 0.f;
		if (ObservedVelocity.Size2D() > 55.) KnownHeading = ObservedVelocity.GetSafeNormal2D();
	}
	else ObservedVelocity = FVector::ZeroVector;
	LastKnown = Feet;
	LastSeen = Time;
	LastObserver = Observer;
	Wolves[Observer].LastSightTime = Time;
	BeliefAge = 0.f;
	Knowledge = EWolfKnowledge::Sight;
}

void FWolfPackTactics::Signal(const FWolfTacticFrame& F, const FWolfPackTuning& Tune, int32 Receiver)
{
	if (Time < NextSignal && !(CueWolf==Leader && F.bBlocked[Leader]))
	{
		// A specialist can answer a just-issued general call as its path is
		// confirmed. Keep the original sound and gesture duration.
		if (CueTime>.05f && CueReceiver==INDEX_NONE && Receiver!=CueWolf) CueReceiver=Receiver;
		return;
	}
	CueWolf = Leader;
	if (F.bBlocked[Leader] || Wolves[Leader].SuppressedUntil > Time)
	{
		CueWolf = INDEX_NONE;
		for (const int32 I : {Tracker, Flanker, PressureBeta, Interceptor})
			if (!F.bBlocked[I] && Wolves[I].SuppressedUntil <= Time) { CueWolf = I; break; }
	}
	CueReceiver = CueWolf!=INDEX_NONE && Receiver!=CueWolf ? Receiver : INDEX_NONE;
	CueTime = CueWolf == INDEX_NONE ? 0.f : .57f;
	NextSignal = Time+Tune.SignalCooldown;
}

void FWolfPackTactics::Tick(float Dt, const FWolfTacticFrame& F, const FWolfPackTuning& Tune, FWolfSenseQuery Sense, FWolfRouteQuery Find)
{
	if (!bStarted)
	{
		bStarted = true;
		KnownHeading = Horizontal(F.Forward,FVector::ForwardVector);
		SearchOrigin = F.TrailReference;
	}
	Time += FMath::Max(0.f, Dt);
	FreshSight = Tune.FreshSightSeconds;
	AttackCooldown = Tune.EngagementCooldown;
	CueTime = FMath::Max(0.f,CueTime-Dt);
	if (Time >= NextSight)
	{
		FVector Seen;
		++ObservationCount;
		if (Sense(SenseCursor, Seen)) Observe(Seen,SenseCursor,Tune);
		SenseCursor = (SenseCursor+1)%Count;
		NextSight = Time + FMath::Max(.02f,Tune.SenseInterval);
	}
	BeliefAge = Knowledge == EWolfKnowledge::None ? BIG_NUMBER : Time-(Knowledge == EWolfKnowledge::Sight ? LastSeen : NoiseTime);
	const bool bVisible = Knowledge == EWolfKnowledge::Sight && BeliefAge <= FreshSight;
	const EWolfIntention Previous = Intention;
	Intention = bVisible ? EWolfIntention::Pursue : BeliefAge <= Tune.MemorySeconds ? EWolfIntention::Investigate : EWolfIntention::Search;
	if (Previous != Intention && LastObserver != INDEX_NONE)
	{
		Signal(F,Tune);
		if (Previous == EWolfIntention::Pursue)
			for (auto& Wolf : Wolves) Wolf.Sweep = {};
	}

	for (int32 I = 0; I < Count; ++I)
	{
		auto& Wolf = Wolves[I];
		if (F.bBlocked[I] && !Wolf.bWasBlocked)
			Release(I,TEXT("way blocked; yielding the assignment"),Tune,true);
		Wolf.bWasBlocked = F.bBlocked[I];
		if (Wolf.Assignment == EWolfAssignment::Intercept && (!bVisible || !F.bAllowIntercept || ObservedVelocity.Size2D() < Tune.ScoutMinimumQuarrySpeed))
			Release(I,TEXT("intercept lost its sightline or movement prediction"),Tune,true);
		if (Wolf.Assignment == EWolfAssignment::Flank && (!bVisible || !F.bAllowFlank))
			Release(I,TEXT("side approach no longer available"),Tune,false);
		if (Wolf.bEngaging && (!bVisible || !F.bAllowEngagement || Time-Wolf.LastSightTime > FreshSight || Time-Wolf.LastPlan > 1.8f))
			Release(I,TEXT("engagement window closed"),Tune,false);
	}
	if (Time < NextPlan) return;
	NextPlan = Time + FMath::Max(.02f,Tune.DecisionInterval);
	// Prefer the beta. A trapped wolf has no vote, and a much better placed
	// support can relieve a pressure wolf once the short commitment expires.
	if (bVisible && (Wolves[PressureWolf].CommittedUntil <= Time || F.bBlocked[PressureWolf] || Wolves[PressureWolf].SuppressedUntil > Time))
	{
		float Best = BIG_NUMBER;
		int32 Chosen = PressureWolf;
		for (int32 I = 0; I < Count; ++I)
		{
			if (F.bBlocked[I] || Wolves[I].SuppressedUntil > Time || Wolves[I].Assignment == EWolfAssignment::Intercept) continue;
			const float Preference = I==PressureBeta ? Tune.PressurePreference : I==Flanker ? Tune.FlankerReliefPreference : 0.f;
			const float Score = FVector::Dist2D(F.Feet[I],LastKnown) - Preference - (I==PressureWolf ? 85.f : 0.f);
			if (Score < Best) {Best = Score; Chosen = I;}
		}
		if (PressureWolf != Chosen)
		{
			if (Wolves[PressureWolf].Assignment == EWolfAssignment::Pressure) Release(PressureWolf,TEXT("another wolf can maintain pressure"),Tune,false);
			Signal(F,Tune,Chosen);
		}
		PressureWolf = Chosen;
	}
	const int32 Wolf = PlanCursor;
	PlanCursor = (PlanCursor+1)%Count;
	++DecisionCount;
	Plan(Wolf,F,Tune,Find);
}

void FWolfPackTactics::Plan(int32 I, const FWolfTacticFrame& F, const FWolfPackTuning& Tune, FWolfRouteQuery Find)
{
	auto& Wolf = Wolves[I];
	const bool bVisible = Intention == EWolfIntention::Pursue;
	const bool bFree = !F.bBlocked[I] && Time >= Wolf.SuppressedUntil;
	const FVector From = F.Feet[I];
	const FVector Anchor = Knowledge == EWolfKnowledge::None ? SearchOrigin : LastKnown;
	const FVector Ahead = KnownHeading;
	const FVector Side(-Ahead.Y,Ahead.X,0);
	const float Lead = FMath::Max(0.f,F.PressureLead);
	const float Pad = FMath::Max(Lead,Tune.StandOff);
	const double Lateral = FVector::DotProduct(From-Anchor,Side);
	double Sign = FMath::Abs(Lateral) > 18. ? FMath::Sign(Lateral) : (I%2 ? -1. : 1.);
	if (Wolf.Assignment == EWolfAssignment::Flank && Time < Wolf.CommittedUntil)
		Sign = FMath::Sign(FVector::DotProduct(Wolf.Destination-Anchor,Side));
	if (Wolf.Assignment == EWolfAssignment::Intercept && bVisible && bFree &&
		Time < Wolf.CommittedUntil && FVector::DistSquared2D(From,Wolf.Destination) > FMath::Square(Arrival) &&
		FVector::DotProduct(LastKnown-Wolf.Destination,Ahead) < 0.)
		return; // Follow the actual alternate branch instead of replacing it each tick.

	int32 Queries = 0;
	const int32 QueryLimit = 5;
	auto Assign = [&](EWolfAssignment Job, const FVector& Goal, const TCHAR* Why, bool bEngage=false, bool bAlternate=false, float WolfETA=BIG_NUMBER) -> bool
	{
		if (Queries >= QueryLimit) return false;
		const bool bSame = Job == Wolf.Assignment && bEngage == Wolf.bEngaging && !Wolf.Route.Points.IsEmpty();
		if (bSame && FVector::DistSquared2D(Goal,Wolf.Destination) < FMath::Square(38.f) && !F.bBlocked[I])
		{
			Wolf.LastPlan = Time;
			return true;
		}
		// The attack line is the one shared exception to ordinary destination reservations.
		if (!bEngage)
		{
			for (int32 J=0; J<Count; ++J)
				if (J != I && !Wolves[J].Route.Points.IsEmpty() && FVector::DistSquared2D(Goal,Wolves[J].Destination) < FMath::Square(Tune.Spacing*.80f))
					return false;
		}
		FWolfTacticRoute Route;
		++Queries;
		++RouteQueryCount;
		if (!Find(I,From,Goal,bAlternate,Route) || Route.Points.IsEmpty() || !FMath::IsFinite(Route.Length)) return false;
		if (FVector::Dist2D(Route.Points.Last(),Goal) > 82.f || (bAlternate && !Route.bAlternate)) return false;
		if (Job == EWolfAssignment::Flank && Route.Length > FVector::Dist2D(From,Goal)*1.7f+100.f) return false;
		if (Route.Length > WolfETA * FMath::Max(1.f,F.Speed[I])) return false;
		if (bVisible && !bEngage)
		{
			// A legal end point can still send a support wolf through Max. Keep
			// the route out of his personal space, allowing retreat if already inside.
			FVector Previous = From;
			Previous.Z = 0.;
			FVector Center = Anchor;
			Center.Z = 0.;
			const double Inner = FMath::Min<double>(Tune.StandOff*.57, FVector::Dist(Previous,Center));
			for (FVector Point : Route.Points)
			{
				Point.Z = 0.;
				if (FMath::PointDistToSegmentSquared(Center,Previous,Point)+1. < FMath::Square(Inner)) return false;
				Previous = Point;
			}
		}
		if (Wolf.bEngaging && !bEngage) Wolf.EngagementReady = Time + Tune.EngagementCooldown;
		if (!bSame)
		{
			Wolf.CommittedUntil = Time + (bAlternate ? Tune.PredictionSeconds+Tune.InterceptSlackSeconds : Tune.CommitmentSeconds);
			if (Job==EWolfAssignment::Flank || Job==EWolfAssignment::Intercept) Signal(F,Tune,I);
		}
		Wolf.Assignment = Job;
		Wolf.Destination = Route.Points.Last();
		Wolf.Route = MoveTemp(Route);
		Wolf.Reason = Why;
		Wolf.bEngaging = bEngage;
		Wolf.LastPlan = Time;
		++Wolf.Revision;
		return true;
	};

	// A recently rejected specialty waits before querying the same branch again.
	bool bFailedSpecialty = false;
	if (bVisible && bFree)
	{
		int32 Engaged = 0;
		for (const auto& Member : Wolves) if (Member.bEngaging) ++Engaged;
		const bool bSlot = Wolf.bEngaging || Engaged < FMath::Clamp(Tune.MaxEngagements,1,Count);
		const bool bCandidate = I==PressureWolf || (I==Flanker && Tune.MaxEngagements>1);
		if (bCandidate && bSlot && F.bAllowEngagement && Time>=Wolf.EngagementReady && Time-Wolf.LastSightTime <= FreshSight)
		{
			// The encounter still owns contact eligibility. While the chase has a
			// positive lead even the reserved wolf pursues that standoff.
			if (Assign(EWolfAssignment::Pressure, Anchor-Ahead*Lead,TEXT("visible quarry; reserved attack approach"),true)) return;
		}
		if (I == PressureWolf)
		{
			if (Assign(EWolfAssignment::Pressure, Anchor-Ahead*Pad,TEXT("holding direct pressure"))) return;
			bFailedSpecialty = true;
		}
		else if (I == Interceptor && F.bAllowIntercept && ObservedVelocity.Size2D() >= Tune.ScoutMinimumQuarrySpeed && Lead < 720.)
		{
			const FVector Prediction = LastKnown + ObservedVelocity*Tune.PredictionSeconds;
			// Keep the main exit open. If the quarry continues at the speed we
			// actually observed, can this wolf reach a parallel approach in time?
			const float Deadline = Tune.PredictionSeconds + Tune.InterceptSlackSeconds - BeliefAge;
			for (double S : {Sign,-Sign})
				if (Assign(EWolfAssignment::Intercept, Prediction + Side*S*Tune.StandOff,
					TEXT("alternate branch can meet the observed line of travel"),false,true,Deadline)) {PredictedTarget=Prediction; return;}
			bFailedSpecialty = true;
		}
		else if (I == Flanker && F.bAllowFlank)
		{
			// Only a rear quarter is closed. A side projection is accepted after
			// the *whole* approach is proved traversable, including its end.
			for (double S : {Sign,-Sign})
				if (Assign(EWolfAssignment::Flank,Anchor-Ahead*FMath::Max(Lead,65.f)+Side*S*Tune.FlankWidth,TEXT("reachable exposed quarter"))) return;
			bFailedSpecialty = true;
		}
		else if (I==Leader)
		{
			const float Shoulder = F.bAllowFlank ? .56f*Tune.FlankWidth : 0.f;
			if (Assign(EWolfAssignment::Coordinate,Anchor-Ahead*(Pad+Tune.Spacing*1.3f)+Side*Sign*Shoulder,TEXT("watching the pressure wolf and trail"))) return;
		}
		else if (I==Tracker)
		{
			const FVector Track = Anchor-Ahead*(Pad+Tune.Spacing*2.45f);
			if (Assign(EWolfAssignment::Track,Track+Side*(F.bAllowFlank ? Sign*Tune.FlankWidth*.72 : 0.),TEXT("checking the last observed approach"))) return;
		}
	}
	if (bFailedSpecialty) Wolf.SuppressedUntil = Time+Tune.FailedRouteCooldown;

	if (!bVisible)
	{
		const bool bInvestigating = Intention==EWolfIntention::Investigate;
		auto& Sweep = Wolf.Sweep;
		const bool bWorking = (Wolf.Assignment==EWolfAssignment::Search || Wolf.Assignment==EWolfAssignment::Track) && !Wolf.Route.Points.IsEmpty();
		const bool bArrived = bWorking && FVector::Dist2D(From,Wolf.Destination)<Arrival;
		if (bWorking && !bInvestigating && !bArrived) return;
		if (bArrived && Sweep.ReachedRevision!=Wolf.Revision &&
			(Wolf.Assignment==EWolfAssignment::Search || Sweep.bReturning || FVector::Dist2D(Wolf.Destination,Anchor)<Arrival))
		{
			Sweep.ReachedRevision = Wolf.Revision;
			++Sweep.Bearing;
			++Sweep.Arrivals;
			if (!bInvestigating && Sweep.Arrivals%FMath::Max(2,Tune.StationsPerSweep)==0)
				Sweep.Center = From; // Only physically visited ground becomes the next sweep center.
		}
		if (I==Tracker && bInvestigating && Sweep.Bearing==0)
		{
			if (Assign(EWolfAssignment::Track,Anchor,TEXT("investigating the last shared observation"))) return;
		}
		if (!bInvestigating && I==Tracker && Knowledge!=EWolfKnowledge::None)
		{
			if (!Sweep.bReturning && Sweep.Arrivals-Sweep.LastReturn>=FMath::Max(4,Tune.TrackerReturnAfter))
			{
				Sweep.bReturning = true;
				Sweep.LastReturn = Sweep.Arrivals;
			}
			if (Sweep.bReturning)
			{
				const FVector Back = Anchor-From;
				if (Back.Size2D()>Arrival && Assign(EWolfAssignment::Track,From+Back.GetClampedToMaxSize(850.),
					TEXT("retracing the trail to the last observation"))) return;
				Sweep.bReturning = false;
				Sweep.Center = From;
			}
		}
		FVector SearchBase = Anchor;
		FVector Direction = Ahead;
		if (bInvestigating && Knowledge==EWolfKnowledge::Sight)
			SearchBase += ObservedVelocity*FMath::Min(.42f,BeliefAge);
		else if (!bInvestigating && Sweep.Center.IsSet())
		{
			SearchBase = Sweep.Center.GetValue();
			// Follow the terrain heading in the last observed direction. The
			// current position of the hidden quarry is unavailable here.
			Direction = Horizontal(F.Forward,Ahead);
			if (FVector::DotProduct(Direction,Ahead)<0.) Direction *= -1.;
		}
		static const double Bearings[] = {14.,-28.,47.,-56.,0.,73.,-78.,31.,-8.};
		for (int32 Attempt=0; Attempt<2; ++Attempt)
		{
			const int32 Step = Sweep.Bearing;
			const double Angle = Bearings[(I*2+Step)%UE_ARRAY_COUNT(Bearings)];
			const double Radius = (I==Tracker ? 160. : 215.+I*32.) + 140.*(Step%7);
			const FVector Point = SearchBase+Direction.RotateAngleAxis(Angle,FVector::UpVector)*Radius;
			if (Assign(EWolfAssignment::Search,Point,TEXT("checking a reachable continuation of the trail"))) return;
			++Sweep.Bearing;
		}
	}

	// The pressure file fits a narrow passage without asking wolves to pass on
	// a nonexistent shoulder. Rank changes whenever pressure has to change hands.
	const int32 Rank = (I-PressureWolf+Count)%Count;
	const FVector FileGoal = Anchor-Ahead*(Pad+(1+Rank)*Tune.Spacing);
	if (Assign(bFailedSpecialty || !bFree ? EWolfAssignment::Regroup : EWolfAssignment::Follow, FileGoal,
		bFailedSpecialty ? TEXT("specialty route rejected; returning to the file") : TEXT("following a clear approach"))) return;
	// If the whole file goal is cut off, first make a real short move toward it
	// or out to a local shoulder. Retry the blocked tactic only after cooldown.
	const FVector Toward = Horizontal(FileGoal-From,Ahead);
	for (double Offset : {0., 68., -68.})
	{
		const FVector Nearby = From+Toward.RotateAngleAxis(Offset*Sign,FVector::UpVector)*Tune.Spacing*1.4f;
		if (Assign(EWolfAssignment::Regroup,Nearby,TEXT("working around a blocked approach"))) return;
	}
	Release(I,TEXT("no complete route; checking the next approach"),Tune,true);
}
