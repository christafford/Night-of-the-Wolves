#if !UE_BUILD_SHIPPING

#include "WolfPackNavigation.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "FirstLegCrossingLayout.h"
#include "WolfTrailRoute.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

namespace
{
void TestWolfPackNavigation(UWorld* World)
{
	if (!World || !World->IsGameWorld()) return;
	const FVector Base(-50000., 75000., 5000.);
	AActor* Fixture = World->SpawnActor<AActor>();
	auto Box = [&](FVector Center, FVector Extent)
	{
		UBoxComponent* Part = NewObject<UBoxComponent>(Fixture);
		Part->InitBoxExtent(Extent);
		Part->SetWorldLocation(Base + Center);
		Part->SetCollisionProfileName(TEXT("BlockAll"));
		Part->RegisterComponent();
		return Part;
	};
	bool bPassed = true;
	auto Check = [&](bool Value, const TCHAR* Label)
	{
		if (!Value)
		{
			UE_LOG(LogTemp, Error, TEXT("WOLF NAV FAILED: %s"), Label);
			bPassed = false;
		}
	};
	UBoxComponent* Floor = Box({0,0,-20}, {1100,540,20});
	const FVector Start = Base + FVector(-650,0,0), End = Base + FVector(650,0,0);
	FWolfPackNavigation Nav;
	Nav.Reset(World);
	FWolfNavigationPath Main, Fork;
	auto Verified = [&](const FWolfNavigationPath& Path)
	{
		FVector Last = Start;
		float Length = 0.f;
		for (const FVector& P : Path.Points)
		{
			if (!Nav.CanTraverse(Last, P)) return false;
			Length += FVector::Distance(Last, P);
			Last = P;
		}
		return !Path.Points.IsEmpty() && Last.Equals(End, 2.) && FMath::IsNearlyEqual(Length, Path.Length, 1.f);
	};
	auto SideAtWall = [&](const FWolfNavigationPath& Path)
	{
		FVector Last = Start;
		for (const FVector& P : Path.Points)
		{
			if (Last.X < Base.X && P.X >= Base.X)
				return FMath::Lerp(Last.Y, P.Y, (Base.X - Last.X) / (P.X - Last.X)) - Base.Y;
			Last = P;
		}
		return 0.;
	};
	Check(Nav.FindPath(Start, End, Main) && Verified(Main) && Main.Points.Num() == 1, TEXT("clear surface was not direct"));
	Check(!Nav.FindPath(Start, End, Fork, true), TEXT("open ground claimed a separate branch"));
	UBoxComponent* Wall = Box({0,0,115}, {86,175,115});
	Check(!Nav.CanTraverse(Start, End), TEXT("closed obstacle left an old route traversable"));
	Check(Nav.FindPath(Start, End, Main) && Verified(Main), TEXT("supported detour around wall"));
	Check(Nav.FindPath(Start, End, Fork, true) && Fork.bAlternate && Verified(Fork), TEXT("second complete supported route"));
	const float MainSide = SideAtWall(Main), ForkSide = SideAtWall(Fork);
	Check(MainSide * ForkSide < -FMath::Square(175.), TEXT("fork did not pass opposite sides"));
	// Shut both lanes while the wolves still hold the previously returned routes.
	Wall->SetBoxExtent({86,700,115});
	Check(!Verified(Main) && !Verified(Fork), TEXT("cached path survived physical closure"));
	Check(!Nav.FindPath(Start, End, Main) && Main.Points.IsEmpty(), TEXT("sealed goal accepted a partial path"));
	Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Check(Nav.FindPath(Start, End, Main) && Main.Points.Num() == 1, TEXT("route did not reopen"));
	Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box({-580,0,-20}, {500,540,20});
	Box({580,0,-20}, {500,540,20});
	Check(!Nav.FindPath(Start, End, Main) && Main.Points.IsEmpty(), TEXT("unsupported gap crossed"));
	const FWolfNavStats Work = Nav.GetStats();
	Nav.Reset(World);
	Check(Nav.GetStats().Searches == 0, TEXT("navigation work survived reset"));
	UE_LOG(LogTemp, Display, TEXT("WOLF NAV physical fixture: passage sides %.0f / %.0f cm; %llu searches, %llu expansions, %llu physics probes, %llu exhausted budgets"),
		MainSide, ForkSide, Work.Searches, Work.Expanded, Work.PhysicsQueries, Work.BudgetStops);
	Fixture->Destroy();

	TActorIterator<AWolfTrailRoute> Trail(World);
	if (Trail && Trail->Spline)
	{
		USplineComponent* Spline = Trail->Spline;
		const double Length = Spline->GetSplineLength();
		auto RouteFeet = [&](double Station) { return Spline->GetLocationAtDistanceAlongSpline(Station, ESplineCoordinateSpace::World); };
		const double Began = FPlatformTime::Seconds();
		for (const double Fraction : {.43, .5, .58, .65, .72, .79})
		{
			const double Station = Fraction * Length;
			const FVector A = RouteFeet(Station), B = RouteFeet(Station + 250.);
			const bool bReachable = Nav.FindPath(A, B, Main);
			if (!bReachable) UE_LOG(LogTemp, Error, TEXT("WOLF NAV map segment %.2f at %s to %s"), Fraction, *A.ToCompactString(), *B.ToCompactString());
			Check(bReachable, TEXT("supported opening trail segment"));
		}
		Check(Nav.FindPath({1450,-890,0}, {2000,-620,0}, Main), TEXT("cottage exterior route"));
		FVector Interior;
		Check(!Nav.ProjectFeet({2740,0,0}, Interior), TEXT("sheltered cottage interior accepted"));
		Check(Nav.FindPath(FirstLegLayout::FarRoad[2], FirstLegLayout::FarRoad[3], Main), TEXT("far bank road"));
		Check(Nav.FindPath({9410,-1307,49}, {9729,-1194,123}, Main), TEXT("near bank to top of loading ramp"));
		const double RouteMillis = (FPlatformTime::Seconds() - Began) * 1000.;
		FVector Feet, Next;
		const double Station = Length * .58;
		Check(Nav.ProjectFeet(RouteFeet(Station), Feet), TEXT("opening movement start"));
		Check(Nav.ProjectFeet(RouteFeet(Station + 10.), Next), TEXT("opening movement end"));
		const uint64 Before = Nav.GetStats().PhysicsQueries;
		const double MoveStart = FPlatformTime::Seconds();
		bool bWalked = true;
		for (int32 I = 0; I < 1000; ++I) bWalked &= Nav.CanTraverse(Feet, Next);
		Check(bWalked, TEXT("movement edge repeatedly traversable"));
		UE_LOG(LogTemp, Display, TEXT("WOLF NAV map: nine complete routes in %.3f ms; 1000 checked 10 cm steps %.3f ms, %llu physics probes"),
			RouteMillis, (FPlatformTime::Seconds() - MoveStart) * 1000., Nav.GetStats().PhysicsQueries - Before);
	}
	if (Trail && Trail->Spline)
	{
		// The two approaches are on the playable shoulder beyond the creek landing.
		const FVector A(11818.63,-1069.27,48), B(12681.54,-1241.85,48);
		const FVector Center = (A+B)*.5, Outside(12188.90,-1461.50,48);
		auto WalkPath = [&](const FWolfNavigationPath& Route)
		{
			FVector Feet = A;
			for (const FVector& Point : Route.Points)
			{
				if (!Nav.CanTraverse(Feet, Point)) return false;
				Feet = Point;
			}
			return FVector::Dist2D(Feet,B) < 1.;
		};
		Check(Nav.FindPath(A,B,Main) && Main.Points.Num()==1 && WalkPath(Main), TEXT("playable far-road direct approach"));
		Check(Nav.FindPath(A,B,Fork,true) && Fork.bAlternate && WalkPath(Fork) && Fork.Length > Main.Length + 160.,
			TEXT("playable far-road second approach through scenery"));
		Check(!Nav.CanTraverse(Center,Outside), TEXT("far-road branches separated by a physical barrier"));
		// The renderer places SM_CreekStoneA and two neighboring stones around
		// their matching simple collision, between the two traversable shoulders.
		FHitResult Barrier;
		const FVector Boulder(12182.89,-1335.57,48);
		const FCollisionQueryParams Params(SCENE_QUERY_STAT(WolfForkFixture), false);
		const bool bSolid = World->LineTraceSingleByChannel(Barrier, Boulder + FVector(0,0,230), Boulder - FVector(0,0,65), ECC_Pawn, Params);
		Check(bSolid && Barrier.ImpactPoint.Z > Boulder.Z + 15., TEXT("roadside center boulder has raised pawn collision"));
		const UStaticMeshComponent* Surface = Cast<UStaticMeshComponent>(Barrier.GetComponent());
		UE_LOG(LogTemp, Display, TEXT("WOLF NAV playable far-road fork: %.0f cm direct, %.0f cm alternate; boulder collision %s at %s"),
			Main.Length, Fork.Length, *GetNameSafe(Surface ? Surface->GetStaticMesh() : nullptr), *Barrier.ImpactPoint.ToCompactString());
	}

	if (bPassed) UE_LOG(LogTemp, Display, TEXT("WOLF NAV PASSED: direct, opposite traversable branches, immediate invalidation, sealed wall, reopening and unsupported gap"));
	FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}

FAutoConsoleCommandWithWorld NavFixture(TEXT("Wolf.TestPackNavigation"), TEXT("Check physical reachability on an isolated two-lane fixture"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&TestWolfPackNavigation));
}

#endif
