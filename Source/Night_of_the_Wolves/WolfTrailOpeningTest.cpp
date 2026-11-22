#if !UE_BUILD_SHIPPING

#include "WolfTrailGameMode.h"
#include "WolfTrailRoute.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
// Observe the real transition and let the first idle capture run all the way through retry.
struct FOpeningObservation
{
	TWeakObjectPtr<ACharacter> FirstMax;
	TWeakObjectPtr<AWolfTrailRoute> Route;
	FVector Spawn = FVector::ZeroVector;
	float FirstRailDistance = 0;
	float FurthestRailDistance = 0;
	float ClosestShot = BIG_NUMBER;
	float PriorHeight = 0;
	float ControlAt = -1;
	float LaunchAfter = -1;
	int32 VerticalTurns = 0;
	int32 SlopeSign = 0;
	bool bHeldTitle = false;
	float LastShotTime = 0;
	bool bDissolved = false;
	bool bClearView = false;
};

void TestOpening(UWorld* World)
{
	if (!World || World->WorldType != EWorldType::Game) return;
	const float Deadline = World->GetTimeSeconds() + 50.f;
	FTimerHandle Timer;
	World->GetTimerManager().SetTimer(Timer, [World,Deadline,Seen=FOpeningObservation()]() mutable
	{
		auto Fail = [](const TCHAR* Why)
		{
			UE_LOG(LogTemp, Error, TEXT("OPENING TEST FAILED: %s"), Why);
			FPlatformMisc::RequestExitWithStatus(false, 1);
		};
		const float Now = World->GetTimeSeconds();
		auto* Mode = World->GetAuthGameMode<AWolfTrailGameMode>();
		auto* PC = World->GetFirstPlayerController();
		auto* Max = PC ? PC->GetCharacter() : nullptr;
		if (Now > Deadline) { Fail(TEXT("opening, capture or retry timed out")); return; }
		if (!Mode || !Max || !PC->PlayerCameraManager) return;
		if (Mode->IsOpeningCinematicPlaying())
		{
			if (Seen.ControlAt >= 0) { Fail(TEXT("retry replayed the opening")); return; }
			if (!Seen.Route.IsValid())
			{
				TActorIterator<AWolfTrailRoute> It(World);
				if (!It) { Fail(TEXT("no pursuit spline")); return; }
				Seen.Route = *It;
			}
			const auto* Spline = Seen.Route->Spline.Get();
			const FVector Eye = PC->PlayerCameraManager->GetCameraLocation();
			const float Key = Spline->FindInputKeyClosestToWorldLocation(Eye);
			const float Along = Spline->GetDistanceAlongSplineAtSplineInputKey(Key);
			const float Height = Eye.Z - Spline->GetLocationAtSplineInputKey(Key, ESplineCoordinateSpace::World).Z;
			const float DistanceToMax = FVector::Dist2D(Eye, Max->GetActorLocation());
			const float Alpha = Mode->GetOpeningTitleOpacity();
			Seen.LastShotTime = Mode->GetOpeningElapsedSeconds();
			if (!Seen.bHeldTitle && Alpha >= .99f)
			{
				Seen.FirstMax = Max;
				Seen.Spawn = Max->GetActorLocation();
				Seen.FirstRailDistance = Along;
				Seen.bHeldTitle = true;
			}
			if (Alpha > .08f && Alpha < .92f) Seen.bDissolved = true;
			if (Alpha <= .01f) Seen.bClearView = true;
			if (Seen.bClearView && DistanceToMax > 1500 && Seen.PriorHeight != 0)
			{
				const float Diff = Height - Seen.PriorHeight;
				const int32 Sign = Diff > .2f ? 1 : Diff < -.2f ? -1 : 0;
				if (Sign && Seen.SlopeSign && Sign != Seen.SlopeSign) ++Seen.VerticalTurns;
				if (Sign) Seen.SlopeSign = Sign;
			}
			Seen.PriorHeight = Height;
			Seen.FurthestRailDistance = FMath::Max(Seen.FurthestRailDistance, Along);
			Seen.ClosestShot = FMath::Min(Seen.ClosestShot, DistanceToMax);
			if (Mode->IsDead() || Mode->Checkpoint != 0 || !Mode->ThreatCue.IsEmpty() ||
				(Seen.bHeldTitle && FVector::DistSquared2D(Max->GetActorLocation(), Seen.Spawn) > 1.f))
				{ Fail(TEXT("Max moved, pursuit advanced or checkpoint changed during sequence")); return; }
			return;
		}
		if (!Seen.bHeldTitle) return;
		if (Seen.ControlAt < 0)
		{
			Seen.ControlAt = Now;
			if (!Seen.bDissolved || !Seen.bClearView || Seen.FirstRailDistance > 250 ||
				Seen.FurthestRailDistance < 4000 || Seen.ClosestShot > 700 || Seen.VerticalTurns < 4)
				{ Fail(TEXT("missing title dissolve, curved trail traversal, eye bob or close approach")); return; }
			if (PC->GetViewTarget() != Max || PC->IsMoveInputIgnored() || PC->IsLookInputIgnored())
				{ Fail(TEXT("third-person camera or input failed to return")); return; }
		}
		if (Mode->IsWolfPouncing() && Seen.LaunchAfter < 0)
		{
			Seen.LaunchAfter = Now - Seen.ControlAt;
			if (FMath::Abs(Seen.LaunchAfter - Mode->IdleCaptureSeconds) > .15f)
				{ Fail(TEXT("five-second lead did not start at handoff")); return; }
		}
		if (Seen.LaunchAfter >= 0 && !Mode->IsDead() && Seen.FirstMax.Get() != Max)
		{
			if (PC->GetViewTarget() != Max || PC->IsMoveInputIgnored() || PC->IsLookInputIgnored())
				{ Fail(TEXT("CP0 retry did not restore third person and input")); return; }
			UE_LOG(LogTemp, Display, TEXT("OPENING TEST PASSED: %.2fs sequence, title held and dissolved, wolf eye travelled %.1fm with %d height reversals, nearest %.1fm, pounce %.2fs after handoff, retry returned directly to Max"),
				Seen.LastShotTime, (Seen.FurthestRailDistance-Seen.FirstRailDistance)/100.f, Seen.VerticalTurns, Seen.ClosestShot/100.f, Seen.LaunchAfter);
			FPlatformMisc::RequestExit(false);
		}
	}, .025f, true);
}
FAutoConsoleCommandWithWorld OpeningProbe(TEXT("Wolf.TestOpening"), TEXT("Observe the opening and first idle retry from a fresh trail"), FConsoleCommandWithWorldDelegate::CreateStatic(&TestOpening));
}
#endif
