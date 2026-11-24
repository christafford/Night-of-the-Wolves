#if !UE_BUILD_SHIPPING
#include "HighWaterChamber.h"
#include "HighWaterLayout.h"
#include "WolfTrailGameMode.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputKeyEventArgs.h"
#include "TimerManager.h"

namespace
{
enum class ESurveyCheck
{
	Walk, EnterPool, Ashore, Interact, UseItem, RefuseHeld, Mounted, Low, High, Released, Drifted, Guided, Ascended,
	Descended, Forgotten, Held, Loaded, Risen, Saved
};

struct FSurveyAction
{
	ESurveyCheck Check;
	FVector Point = FVector::ZeroVector;
};

// Look beyond trunk pivots: crown cards from outdoor stands once filled the black pool.
bool SurveyCanopyIsClear(UWorld* Level)
{
	for (TActorIterator<AActor> Actor(Level); Actor; ++Actor)
	{
		TInlineComponentArray<UInstancedStaticMeshComponent*> Batches(*Actor);
		for (const UInstancedStaticMeshComponent* Batch : Batches)
		{
			const UStaticMesh* Tree = Batch->GetStaticMesh();
			if (!Tree || !Batch->IsVisible()) continue;
			const FString Species = Tree->GetName();
			if (!Species.Contains(TEXT("Fir")) && !Species.Contains(TEXT("Pine")) && !Species.Contains(TEXT("dead_tree"))) continue;
			for (int32 I=0; I<Batch->GetInstanceCount(); ++I)
			{
				FTransform Instance;
				if (Batch->GetInstanceTransform(I,Instance,true) &&
					HighWaterLayout::IntersectsSurveyCut(Tree->GetBoundingBox().TransformBy(Instance)))
				{
					UE_LOG(LogTemp,Error,TEXT("Outdoor %s on %s overlaps survey cut, pivot %s"),*Species,*Actor->GetName(),*Instance.GetLocation().ToCompactString());
					return false;
				}
			}
		}
	}
	return true;
}
void PlaySurveyChamber(UWorld* Level)
{
	if (!Level || Level->WorldType != EWorldType::Game) return;
	using namespace HighWaterLayout;
	using E = ESurveyCheck;
	TArray<FSurveyAction> Plan;
	auto Walk = [&](FVector Point) { Plan.Add({E::Walk, Point}); };
	auto Check = [&](E What) { Plan.Add({What}); };
	auto Use = [&](E Outcome) { Check(Outcome==E::Mounted || Outcome==E::Loaded ? E::UseItem : E::Interact); Check(Outcome); };
	auto ToSteps = [&] { Walk(StepsTop); Walk(StepsBottom); };
	auto ToTrough = [&] { Walk(StepsBottom); Walk(StepsTop); Walk(Trough); };
	auto ToLadder = [&] { Walk({460,2710,ShelfZ}); Walk(Board); Walk(LowerClimb); };
	auto FromLadder = [&] { Walk(Board); Walk({460,2710,ShelfZ}); Walk(Trough); };

	Walk(Entrance); Walk(Shelf); Walk(Trough); Use(E::RefuseHeld);
	Walk({460,2710,ShelfZ}); Walk(Board); Check(E::EnterPool); Check(E::Ashore);
	Walk(LowerFork); Use(E::Mounted); Walk(Trough); Use(E::Low);
	ToSteps(); Walk(Pin); Use(E::Released);
	ToTrough(); Use(E::Drifted); Use(E::Low);
	ToSteps(); Walk(WrongFloat); Use(E::Guided);
	ToTrough(); Use(E::High); ToLadder(); Use(E::Ascended);
	Walk({1490,2800,LandingZ}); Walk(Exit); Check(E::Forgotten);
	Walk({1490,2800,LandingZ}); Walk(Landing); Use(E::Descended); FromLadder(); Use(E::Low);
	Walk(LowerFork); Use(E::Held);
	ToSteps(); Walk(UpperForkLow); Use(E::Loaded);
	ToTrough(); Use(E::Risen); ToLadder(); Use(E::Ascended);
	Use(E::Held); Walk({1490,2800,LandingZ}); Walk(Exit); Check(E::Saved);

	const bool bResume = FParse::Param(FCommandLine::Get(),TEXT("WatermarkResume"));
	FTimerHandle Timer;
	Level->GetTimerManager().SetTimer(Timer, [Level,Plan=MoveTemp(Plan),bResume,Idx=0,
		Start=Level->GetTimeSeconds(),Since=Level->GetTimeSeconds(),HeldKey=FKey(),bWitnessedRise=false,bRetried=false]() mutable
	{
		auto* Mode = Level->GetAuthGameMode<AWolfTrailGameMode>();
		auto* PC = Level->GetFirstPlayerController();
		auto* Max = PC ? PC->GetCharacter() : nullptr;
		TActorIterator<AHighWaterChamber> Chambers(Level);
		if (!Mode || !Max || !Chambers) return;
		const AHighWaterChamber& Room = **Chambers;
		const FVector Feet = StormfallLayout::LocalPoint(Max->GetCharacterMovement()->GetActorFeetLocation());
		const float Now = Level->GetTimeSeconds();
		auto Fail = [&](const TCHAR* Why)
		{
			UE_LOG(LogTemp,Error,TEXT("HIGH WATER TEST FAILED: %s, step %d, CP%d, feet %s, water %.2f, float %s, prompt [%s]"),
				Why,Idx,Mode->Checkpoint,*Feet.ToCompactString(),Room.WaterLevel(),*Room.GetFloatLocal().ToCompactString(),*Mode->InteractionPrompt);
			FPlatformMisc::RequestExitWithStatus(false,1);
		};
		if (bResume)
		{
			if (Now-Start < .75f) return;
			if (Mode->Checkpoint!=7 || !Room.IsComplete() || !Mode->HasTreeFlame() ||
				!Max->GetCharacterMovement()->IsMovingOnGround() || FVector::Dist2D(Feet,Exit)>120.)
				{ Fail(TEXT("CP7 on a cold launch")); return; }
			UE_LOG(LogTemp,Display,TEXT("HIGH WATER RESUME PASSED: CP7, lit firebrand in hand on the upper passage"));
			FPlatformMisc::RequestExit(false); return;
		}
		if (Mode->IsDead() || Now-Since>12.f || Mode->Checkpoint<6) { Fail(TEXT("death, wrong checkpoint or no progress")); return; }
		if (HeldKey.IsValid())
		{
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(HeldKey, IE_Released, 0.f));
			HeldKey=FKey();
		}
		const FSurveyAction& Action=Plan[Idx];
		bool bDone=false;
		switch (Action.Check)
		{
		case E::Walk:
		{
			const FVector Direction=World(Action.Point)-World(Feet);
			if (Direction.Size2D()<14. && FMath::Abs(Direction.Z)<55.)
			{
				Max->GetCharacterMovement()->StopMovementImmediately();
				bDone=true;
			}
			else
			{
				Max->AddMovementInput(Direction.GetSafeNormal2D(), FMath::Clamp(Direction.Size2D()/75.,.15,1.));
				PC->SetControlRotation(FRotator(-8.,Direction.Rotation().Yaw,0.));
			}
			break;
		}
		case E::EnterPool:
			Max->AddMovementInput(StormfallLayout::Facing.Vector());
			bDone=Room.IsSwimmingOut(); break;
		case E::Ashore:
			bDone=!Room.IsSwimmingOut() && Max->GetCharacterMovement()->IsMovingOnGround() &&
				Feet.Z>=ShelfZ-5.f && Mode->HasTreeFlame(); break;
		case E::UseItem:
		case E::Interact:
			HeldKey = Action.Check == E::UseItem ? EKeys::F : EKeys::E;
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(HeldKey,IE_Pressed,1.f)); bDone=true; break;
		case E::RefuseHeld:
			if (!SurveyCanopyIsClear(Level)) { Fail(TEXT("forest canopy in the cave")); return; }
			if (Room.IsDiverted() || Room.HasMountedTorch() || !Mode->HasTreeFlame() ||
				Mode->CommitCheckpoint(7,Max->GetActorLocation(),Max->GetActorRotation()))
				{ Fail(TEXT("hands-full trough or early CP7")); return; }
			bDone=true; break;
		case E::Mounted:
			bDone=Room.HasMountedTorch() && !Room.IsTorchOnFloat() && !Mode->Inventory.Has(TEXT("Firebrand")); break;
		case E::Low: bDone=Room.IsDiverted() && Room.WaterLevel()<.005f; break;
		case E::High: bDone=!Room.IsDiverted() && Room.WaterLevel()>.995f; break;
		case E::Released: bDone=Room.IsUnpinned() && !Room.IsGuided(); break;
		case E::Drifted: bDone=Room.WaterLevel()>.995f && FVector::Dist2D(Room.GetFloatLocal(),WrongFloat)<22.; break;
		case E::Guided: bDone=Room.IsGuided() && FVector::Dist2D(Room.GetFloatLocal(),Guides)<12.; break;
		case E::Ascended:
			bDone=!Room.IsClimbing() && Feet.Z>LandingZ-18. && Mode->Checkpoint==6; break;
		case E::Descended: bDone=!Room.IsClimbing() && Feet.Z<ShelfZ+22. && Mode->Checkpoint==6; break;
		case E::Forgotten:
			if (Mode->Checkpoint!=6 || Room.IsComplete() || !Room.HasMountedTorch() || Mode->Inventory.Has(TEXT("Firebrand")))
				{ Fail(TEXT("exit accepted the torch still burning on the wall")); return; }
			bDone=true; break;
		case E::Held: bDone=!Room.HasMountedTorch() && Mode->HasTreeFlame(); break;
		case E::Loaded: bDone=Room.IsTorchOnFloat() && !Mode->Inventory.Has(TEXT("Firebrand")); break;
		case E::Risen:
			if (!bRetried && Room.IsFramingRefill() && Room.WaterLevel()>.4f && FParse::Param(FCommandLine::Get(),TEXT("WatermarkRetry")))
			{
				Mode->RetryCheckpoint();
				if (Room.IsFramingRefill() || Room.IsClimbing() || Room.HasMountedTorch() || Room.IsGuided() ||
					Room.IsUnpinned() || Room.IsDiverted() || Room.WaterLevel()!=1.f || !Mode->HasTreeFlame() ||
					PC->IsMoveInputIgnored() || PC->GetViewTarget()!=PC->GetPawn() || Mode->Checkpoint!=6)
					{ Fail(TEXT("retry during the lit refill")); return; }
				bRetried=true; bWitnessedRise=false; Idx=0; Since=Now;
				UE_LOG(LogTemp,Display,TEXT("HIGH WATER RETRY PASSED: recovered CP6 torch, controls, camera and pinned float during refill"));
				return;
			}
			if (Room.IsTorchOnFloat() && Room.IsFramingRefill() && Room.WaterLevel()>.15f && Room.WaterLevel()<.9f)
				bWitnessedRise=true;
			bDone=bWitnessedRise && Room.IsTorchOnFloat() && Room.WaterLevel()>.995f && !Room.IsFramingRefill(); break;
		case E::Saved:
			if (Mode->Checkpoint!=7 || !Room.IsComplete() || !Mode->HasTreeFlame()) { Fail(TEXT("upper passage did not store the recovered flame")); return; }
			UE_LOG(LogTemp,Display,TEXT("HIGH WATER TEST PASSED: hands-full refusal, drain, release, wrong-wall refill, guide, forgotten torch climb and return, final lit refill and CP7 in %.1fs"),Now-Start);
			FPlatformMisc::RequestExit(false); return;
		}
		if (bDone)
		{
			UE_LOG(LogTemp,Display,TEXT("HIGH WATER TEST step %d check %d, feet %s water %.2f"),Idx,int32(Action.Check),*Feet.ToCompactString(),Room.WaterLevel());
			++Idx; Since=Now;
		}
	},.016f,FTimerManagerTimerParameters{.bLoop=true,.bMaxOncePerFrame=true});
}
FAutoConsoleCommandWithWorld SurveyProbe(TEXT("Wolf.TestHighWater"), TEXT("Play the high-water chamber from an earned survey checkpoint"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&PlaySurveyChamber));
}
#endif
