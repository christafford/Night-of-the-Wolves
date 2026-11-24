#include "WolfPackNavigation.h"

#include "FirstLegCrossingLayout.h"
#include "WolfTrailGameMode.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/World.h"

namespace
{
constexpr double CellSize = 104.;
constexpr double GroundSampleSpacing = 48.;
constexpr double MaxQueryDistance = 4500.;
constexpr double WolfCapsuleRadius = 25.;
constexpr double HalfHeight = 33.;
constexpr double BodyCenter = 51.;
constexpr double MaxStep = 38.;
constexpr int32 MaxExpansions = 140;
constexpr int32 MaxTraces = 1650;

const FCollisionResponseParams& SolidSurfaces()
{
	static const FCollisionResponseParams Response = [] {
		FCollisionResponseParams Value(ECR_Ignore);
		Value.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
		Value.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Block);
		return Value;
	}();
	return Response;
}

const FCollisionShape& WolfBody()
{
	static const FCollisionShape Shape = FCollisionShape::MakeCapsule(WolfCapsuleRadius, HalfHeight);
	return Shape;
}

double DistanceToEdge(const FVector& P, const FVector& A, const FVector& B)
{
	const FVector FlatA(A.X, A.Y, 0.), FlatB(B.X, B.Y, 0.);
	return FMath::PointDistToSegment(FVector(P.X, P.Y, 0.), FlatA, FlatB);
}

struct FCell
{
	FVector Feet;
	double Cost = 0.;
	FIntPoint Previous = FIntPoint::ZeroValue;
	bool bClosed = false;
};

struct FOpenCell
{
	FIntPoint Grid;
	double Score;
	bool operator<(const FOpenCell& Other) const { return Score < Other.Score; }
};

void AddPoint(FWolfNavigationPath& Path, const FVector& From, const FVector& Point)
{
	const FVector Previous = Path.Points.IsEmpty() ? From : Path.Points.Last();
	Path.Length += FVector::Distance(Previous, Point);
	Path.Points.Add(Point);
}
}

struct FWolfPackNavigation::FQuery
{
	explicit FQuery(FWolfNavStats& InStats) : Stats(InStats) {}
	bool Available() const { return Traces < MaxTraces; }
	bool Trace()
	{
		if (!Available())
		{
			if (!bReported) { ++Stats.BudgetStops; bReported = true; }
			return false;
		}
		++Stats.PhysicsQueries;
		++Traces;
		return true;
	}
	FWolfNavStats& Stats;
	int32 Traces = 0;
	bool bReported = false;
};

void FWolfPackNavigation::Reset(UWorld* InWorld)
{
	World = InWorld;
	Game = InWorld ? InWorld->GetAuthGameMode<AWolfTrailGameMode>() : nullptr;
	bCabinAllowed = false;
	Stats = {};
}

bool FWolfPackNavigation::Ground(FQuery& Q, const FVector& Hint, FVector& Out, bool bCheckBody)
{
	UWorld* W = World.Get();
	if (!W || Hint.ContainsNaN() || !Q.Trace()) return false;
	static const FCollisionQueryParams Params(SCENE_QUERY_STAT(WolfPackGround), false);
	FHitResult Hit;
	if (!W->LineTraceSingleByChannel(Hit, Hint + FVector::UpVector * 54., Hint - FVector::UpVector * 120.,
		ECC_Pawn, Params, SolidSurfaces()) || Hit.bStartPenetrating || Hit.ImpactNormal.Z < .68) return false;
	Out = Hit.ImpactPoint;
	if (!bCabinAllowed && Game.IsValid() && Game->IsOnCabinFloor(Out + FVector::UpVector * HalfHeight)) return false;
	const float Bend = FirstLegLayout::StreamBend(Out.Y);
	if (Out.Z < -120. && Out.X > 9800. + Bend && Out.X < 10900. + Bend) return false;
	return !bCheckBody || (Q.Trace() && !W->OverlapBlockingTestByChannel(Out + FVector::UpVector * BodyCenter,
		FQuat::Identity, ECC_Pawn, WolfBody(), Params, SolidSurfaces()));
}

bool FWolfPackNavigation::ProjectFeet(const FVector& Hint, FVector& Out)
{
	FQuery Query(Stats);
	return Ground(Query, Hint, Out, true);
}

bool FWolfPackNavigation::Segment(FQuery& Q, const FVector& From, const FVector& To, FVector& OutEnd)
{
	UWorld* W = World.Get();
	const double Distance = FVector::Dist2D(From, To);
	if (!W || From.ContainsNaN() || To.ContainsNaN() || Distance > MaxQueryDistance) return false;
	const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / GroundSampleSpacing));
	FVector Previous = From;
	const FVector Lift = FVector::UpVector * BodyCenter;
	static const FCollisionQueryParams Params(SCENE_QUERY_STAT(WolfPackSweep), false);
	for (int32 I = 1; I <= Samples; ++I)
	{
		FVector Hint = FMath::Lerp(From, To, double(I) / Samples);
		Hint.Z = Previous.Z;
		FVector Next;
		if (!Ground(Q, Hint, Next, false) || FMath::Abs(Next.Z - Previous.Z) > MaxStep || !Q.Trace()) return false;
		if (W->SweepTestByChannel(Previous + Lift, Next + Lift, FQuat::Identity, ECC_Pawn, WolfBody(), Params, SolidSurfaces())) return false;
		Previous = Next;
	}
	OutEnd = Previous;
	return FMath::Abs(Previous.Z - To.Z) <= MaxStep;
}

bool FWolfPackNavigation::CanTraverse(const FVector& From, const FVector& To, FVector* GroundedEnd)
{
	FQuery Query(Stats);
	FVector Start, End;
	if (!Ground(Query, From, Start, true) || !Segment(Query, Start, To, End) || FMath::Abs(End.Z - To.Z) > MaxStep) return false;
	if (GroundedEnd) *GroundedEnd = End;
	return true;
}

bool FWolfPackNavigation::Plan(FQuery& Q, const FVector& From, const FVector& To, FWolfNavigationPath& Out,
	const FVector* Avoid, float AvoidRadius)
{
	Out = {};
	const double Distance = FVector::Dist2D(From, To);
	if (Distance > MaxQueryDistance) return false;
	const auto Allowed = [&](const FVector& A, const FVector& B) {
		return !Avoid || DistanceToEdge(*Avoid, A, B) >= AvoidRadius;
	};
	FVector End;
	if (Allowed(From, To) && Segment(Q, From, To, End) && FMath::Abs(End.Z - To.Z) <= MaxStep)
	{
		AddPoint(Out, From, End);
		return true;
	}

	// Four-connected edges preclude diagonal corner cutting. A clear capsule sweep can
	// subsequently smooth a right angle, including across grade changes.
	const FVector Forward = (To - From).GetSafeNormal2D();
	const FVector Side(-Forward.Y, Forward.X, 0.);
	if (Forward.IsNearlyZero()) return false;
	static const FIntPoint Directions[] = {{1,0}, {0,1}, {0,-1}, {-1,0}};
	TMap<FIntPoint, FCell> Cells;
	Cells.Reserve(MaxExpansions * 3);
	Cells.Add(FIntPoint::ZeroValue, {From, 0., FIntPoint::ZeroValue, false});
	TArray<FOpenCell> Open;
	Open.HeapPush({FIntPoint::ZeroValue, Distance});
	// The node cache lasts one search. A newly closed door must affect the very
	// next movement check, even when the destination is unchanged.
	int32 Expanded = 0;
	while (!Open.IsEmpty() && Expanded < MaxExpansions && Q.Available())
	{
		FOpenCell Item;
		Open.HeapPop(Item, EAllowShrinking::No);
		FCell& Stored = Cells.FindChecked(Item.Grid);
		if (Stored.bClosed) continue;
		Stored.bClosed = true;
		const FVector Current = Stored.Feet;
		const double Cost = Stored.Cost;
		++Expanded;
		++Stats.Expanded;
		if (FVector::DistSquared2D(Current, To) <= FMath::Square(CellSize * 1.6) &&
			Allowed(Current, To) && Segment(Q, Current, To, End) && FMath::Abs(End.Z - To.Z) <= MaxStep)
		{
			TArray<FVector> Reverse;
			Reverse.Add(End);
			FIntPoint Walk = Item.Grid;
			while (Walk != FIntPoint::ZeroValue)
			{
				const FCell& Node = Cells.FindChecked(Walk);
				Reverse.Add(Node.Feet);
				Walk = Node.Previous;
			}
			FVector Last = From;
			for (int32 I = Reverse.Num() - 1; I >= 0;)
			{
				// Limit shortcut probes to keep collision work bounded on long fallback routes.
				int32 Next = I;
				for (int32 J = FMath::Max(0, I - 3); J < I; ++J)
				{
					FVector Snapped;
					if (Q.Available() && Allowed(Last, Reverse[J]) && Segment(Q, Last, Reverse[J], Snapped) &&
						FMath::Abs(Snapped.Z - Reverse[J].Z) < 3.) { Next = J; break; }
				}
				AddPoint(Out, From, Reverse[Next]);
				Last = Reverse[Next];
				I = Next - 1;
			}
			return true;
		}

		for (int32 D = 0; D < UE_ARRAY_COUNT(Directions); ++D)
		{
			const FIntPoint Key = Item.Grid + Directions[D];
			const FCell* Existing = Cells.Find(Key);
			if (Existing && (Existing->bClosed || Existing->Cost <= Cost + CellSize)) continue;
			FVector Next = From + CellSize * (Forward * Key.X + Side * Key.Y);
			Next.Z = Current.Z;
			if (!Allowed(Current, Next) || !Segment(Q, Current, Next, End))
				continue;
			const double NewCost = Cost + FVector::Distance(Current, End);
			if (Existing && Existing->Cost <= NewCost) continue;
			Cells.Add(Key, {End, NewCost, Item.Grid, false});
			Open.HeapPush({Key, NewCost + FVector::Dist2D(End, To)});
		}
	}
	return false;
}

bool FWolfPackNavigation::ViaRoute(FQuery& Q, const FVector& From, const FVector& Via, const FVector& To, FWolfNavigationPath& Out)
{
	FWolfNavigationPath First, Second;
	if (!Plan(Q, From, Via, First) || !Plan(Q, First.Points.Last(), To, Second)) return false;
	Out = MoveTemp(First);
	Out.Points.Append(Second.Points);
	Out.Length += Second.Length;
	return true;
}

bool FWolfPackNavigation::FindPathVia(const FVector& From, const FVector& Via, const FVector& To, FWolfNavigationPath& Out)
{
	Out = {};
	++Stats.Searches;
	FQuery Query(Stats);
	FVector A, B, C;
	return Ground(Query, From, A, true) && Ground(Query, Via, B, true) && Ground(Query, To, C, true) &&
		ViaRoute(Query, A, B, C, Out);
}

bool FWolfPackNavigation::Alternate(FQuery& Q, const FVector& From, const FVector& To, const FWolfNavigationPath& Primary, FWolfNavigationPath& Out)
{
	// Find two supported corridors with a physical obstruction between them. On
	// open ground, offsetting an otherwise straight route is not an alternate branch.
	const FVector Direction = (To - From).GetSafeNormal2D();
	const FVector Across(-Direction.Y, Direction.X, 0.);
	const double Distance = FVector::Dist2D(From, To);
	if (Distance < CellSize * 3.) return false;

	// Sample the main path at the actual station of an obstruction. Choosing its
	// most lateral vertex can put the probe just *before* the obstacle on a bend.
	FVector Prior = From;
	bool bHasDetour = false;
	for (const FVector& P : Primary.Points)
	{
		const FVector EdgeStart = Prior;
		Prior = P;
		for (const double T : {.5, .25, .75, 1.})
		{
			const FVector Peak = FMath::Lerp(EdgeStart, P, T);
			const double Station = FVector::DotProduct(Peak - From, Direction);
			const double Lateral = FVector::DotProduct(Peak - From, Across);
			if (Station < Distance * .06 || Station > Distance * .94 || FMath::Abs(Lateral) < WolfCapsuleRadius * 1.8) continue;
			bHasDetour = true;
			FVector Median = From + Direction * Station;
			Median.Z = Peak.Z;
			for (double Width : {FMath::Abs(Lateral), FMath::Abs(Lateral) + CellSize})
			{
				FVector Branch, Crossing;
				if (!Ground(Q, Median - Across * FMath::Sign(Lateral) * Width, Branch, true) ||
					Segment(Q, Peak, Branch, Crossing)) continue;
				FWolfNavigationPath First, Second;
				const double KeepAway = FMath::Max(WolfCapsuleRadius * 2., FMath::Abs(Lateral) * .75);
				if (!Plan(Q, From, Branch, First, &Peak, KeepAway) || !Plan(Q, Branch, To, Second, &Peak, KeepAway)) continue;
				if (First.Length + Second.Length > Primary.Length * 1.8f) continue;
				Out = MoveTemp(First);
				Out.Points.Append(Second.Points);
				Out.Length += Second.Length;
				Out.bAlternate = true;
				return true;
			}
		}
	}
	if (bHasDetour) return false;

	// A straight main path can have a parallel path behind a thicket. Qualify
	// both approaches and the far-side exit independently; avoid spurs that force
	// the scout to walk back the same way it entered.
	for (double Fraction : {.5, .33, .67})
	{
		FVector Middle;
		if (!Ground(Q, FMath::Lerp(From, To, Fraction), Middle, true)) continue;
		for (double Sign : {1., -1.})
		{
			FVector Wing, Unused;
			if (!Ground(Q, Middle + Across * Sign * CellSize * 3., Wing, true) ||
				Segment(Q, Middle, Wing, Unused)) continue;
			const double Stride = FMath::Min(Distance * .17, CellSize * 1.8);
			FVector Entry, Exit;
			if (!Ground(Q, Wing - Direction * Stride, Entry, true) ||
				!Ground(Q, Wing + Direction * Stride, Exit, true) ||
				!Segment(Q, Entry, Wing, Unused) || !Segment(Q, Wing, Exit, Unused)) continue;
			FWolfNavigationPath First, Last;
			if (!Plan(Q, From, Entry, First, &Middle, CellSize) || !Plan(Q, Exit, To, Last, &Middle, CellSize)) continue;
			AddPoint(First, From, Wing);
			AddPoint(First, From, Exit);
			if (First.Length + Last.Length > Primary.Length * 1.65f) continue;
			Out = MoveTemp(First);
			Out.Points.Append(Last.Points);
			Out.Length += Last.Length;
			Out.bAlternate = true;
			return true;
		}
	}
	return false;
}

bool FWolfPackNavigation::FindPath(const FVector& From, const FVector& To, FWolfNavigationPath& Out, bool bWantAlternate)
{
	Out = {};
	++Stats.Searches;
	FQuery Query(Stats);
	FVector A, B;
	if (!Ground(Query, From, A, true) || !Ground(Query, To, B, true)) return false;
	if (!bWantAlternate) return Plan(Query, A, B, Out);
	FWolfNavigationPath Primary;
	return Plan(Query, A, B, Primary) && Alternate(Query, A, B, Primary, Out);
}
