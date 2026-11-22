#if !UE_BUILD_SHIPPING

#include "WolfCaptureCamera.h"
#include "WolfCottageSet.h"
#include "WolfMeshComponent.h"
#include "WolfTrailGameMode.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
// Start with -NoTrailSave at CP0. The test only watches the first real capture;
// during the second capture it requests the same retry used by the pause menu.
void TestWolfCapture(UWorld* World)
{
	if (!World || World->WorldType != EWorldType::Game) return;
	const float Deadline = World->GetTimeSeconds() + 40.f;
	FTimerHandle Timer;
	World->GetTimerManager().SetTimer(Timer, [World,Deadline,Attempt=0,
		PreviousDead=false,CameraSeen=false,CardSeen=false,ReactionSeen=false,
		LaunchZ=0.,PeakRise=0.,ContactGap=DBL_MAX,PreviousPawn=TWeakObjectPtr<APawn>()]() mutable
	{
		auto* Mode = World->GetAuthGameMode<AWolfTrailGameMode>();
		auto* PC = World->GetFirstPlayerController();
		auto* Max = PC ? PC->GetCharacter() : nullptr;
		auto Fail = [](const TCHAR* Reason)
		{
			UE_LOG(LogTemp,Error,TEXT("WOLF CAPTURE TEST FAILED: %s"),Reason);
			FPlatformMisc::RequestExitWithStatus(false,1);
		};
		if (World->GetTimeSeconds() > Deadline) { Fail(TEXT("capture timed out")); return; }
		if (!Mode || !Max) return;
		if (Mode->Checkpoint != 0) { Fail(TEXT("start at the trailhead with -NoTrailSave")); return; }
		if (Mode->IsDead() && !PreviousDead)
		{
			++Attempt;
			TActorIterator<AWolfCottageSet> Cottage(World);
			if (!Cottage || !Cottage->GetLeadWolf() || !Mode->IsWolfPouncing())
				{ Fail(TEXT("missing live lead or world-space pounce")); return; }
			LaunchZ = Cottage->GetLeadWolf()->GetComponentLocation().Z;
			PreviousPawn = Max;
		}
		if (Mode->IsWolfPouncing())
		{
			if (!PC->IsMoveInputIgnored() || !PC->IsLookInputIgnored()
				|| Max->GetCharacterMovement()->MovementMode != MOVE_None)
				{ Fail(TEXT("player could escape during committed capture")); return; }
			const bool bShotActive = Cast<AWolfCaptureCamera>(PC->GetViewTarget()) != nullptr;
			CameraSeen |= bShotActive;
			if (Attempt == 2 && bShotActive)
			{
				PreviousDead = true;
				Mode->RetryCheckpoint();
				return;
			}
			TActorIterator<AWolfCottageSet> Cottage(World);
			if (Cottage && Cottage->GetLeadWolf())
				PeakRise = FMath::Max(PeakRise, Cottage->GetLeadWolf()->GetComponentLocation().Z - LaunchZ);
			auto* Anim = Max->GetMesh()->GetSingleNodeInstance();
			ReactionSeen |= Anim && Anim->GetCurrentAsset();
			if (ReactionSeen && Cottage && Cottage->GetLeadWolf())
			{
				UWolfMeshComponent* Lead = Cottage->GetLeadWolf();
				const FVector Paws = .5 * (Lead->GetSocketLocation(TEXT("ForeLPaw")) + Lead->GetSocketLocation(TEXT("ForeRPaw")));
				const FVector Chest = Max->GetMesh()->GetSocketLocation(TEXT("spine_04"));
				ContactGap = FMath::Min(ContactGap,FVector::Dist(Paws,Chest));
			}
		}
		else if (Mode->IsDead())
		{
			if (!CameraSeen || !ReactionSeen || PeakRise < 45. || ContactGap > 25.)
				{ Fail(TEXT("card preceded airborne/contact animation")); return; }
			CardSeen = true;
		}
		else if (PreviousDead)
		{
			if (!CardSeen || PreviousPawn.Get() == Max || PC->IsMoveInputIgnored() || PC->IsLookInputIgnored()
				|| PC->GetViewTarget() != Max || Max->GetCharacterMovement()->MovementMode == MOVE_None)
				{ Fail(TEXT("retry did not restore Max, movement and gameplay camera")); return; }
			if (Attempt == 2)
			{
				UE_LOG(LogTemp, Display,TEXT("WOLF CAPTURE TEST PASSED: lead rose %.1fcm, paws within %.1fcm of chest centre, impact before card, automatic and mid-pounce retry restored controls"),PeakRise,ContactGap);
				FPlatformMisc::RequestExit(false);
			}
		}
		PreviousDead = Mode->IsDead();
	}, .025f, true);
}

FAutoConsoleCommandWithWorld CaptureProbe(TEXT("Wolf.TestCapture"),
	TEXT("Observe the opening wolf leap, impact and two checkpoint restores"),FConsoleCommandWithWorldDelegate::CreateStatic(&TestWolfCapture));
}

#endif
