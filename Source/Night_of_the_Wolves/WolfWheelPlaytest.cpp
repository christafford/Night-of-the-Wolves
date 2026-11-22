#include "Night_of_the_WolvesPlayerController.h"

#if !UE_BUILD_SHIPPING
#include "WolfTrailGameMode.h"
#include "Containers/Ticker.h"
#include "InputKeyEventArgs.h"

void ANight_of_the_WolvesPlayerController::RequestTestEquip(FName Item)
{
	auto* Mode = GetWorld()->GetAuthGameMode<AWolfTrailGameMode>();
	if (!Mode || bTestSelecting || Mode->Inventory.Equipped == Item) return;
	bTestSelecting = true;
	const auto Before = Mode->Inventory;
	const bool CheckCancel = Before.bBurning && !FParse::Param(FCommandLine::Get(),TEXT("FirstLegClean"));
	// Drive the same controller bindings as a player. The real-time ticker continues
	// while the game clock is paused and keeps key strokes on distinct frames.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this,
		[this, Item, Before, CheckCancel, Step=0, Pulses=0, PausedAt=0.0, KeyDown=FKey()](float) mutable
	{
		auto* Game = GetWorld()->GetAuthGameMode<AWolfTrailGameMode>();
		if (!Game) return false;
		auto Fail = [&](const TCHAR* Why)
		{
			UE_LOG(LogTemp,Error,TEXT("WHEEL INPUT TEST FAILED: %s, selecting %s at step %d"),Why,*Item.ToString(),Step);
			FPlatformMisc::RequestExitWithStatus(false,1);
			return false;
		};
		auto Tap = [&](FKey Key)
		{
			InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Pressed,1.f));
			KeyDown=Key;
		};
		if (++Pulses>70) return Fail(TEXT("no response to key"));
		if (KeyDown.IsValid())
		{
			InputKey(FInputKeyEventArgs::CreateSimulated(KeyDown,IE_Released,0.f));
			KeyDown=FKey();
			return true;
		}
		if (Step==0 || Step==4) { Tap(EKeys::Tab); ++Step; return true; }
		if (Step==1 || Step==5)
		{
			if (Overlay!=EOverlay::Inventory || !IsPaused()) return Fail(TEXT("Tab did not open paused wheel"));
			PausedAt=GetWorld()->GetTimeSeconds();
			++Step;
			return true;
		}
		if (Step==2 || Step==6)
		{
			if (PausedAt!=GetWorld()->GetTimeSeconds() || Game->Inventory.Equipped!=Before.Equipped || Game->Inventory.bBurning!=Before.bBurning)
				return Fail(TEXT("preview advanced game or changed equipment"));
			const int32 TargetIndex=Game->Inventory.Items.Find(Item)+1;
			if (Selection!=TargetIndex) Tap(EKeys::Right);
			else if (Step==2 && CheckCancel) { Tap(EKeys::Escape); Step=3; }
			else { Tap(EKeys::Enter); Step=7; }
			return true;
		}
		if (Step==3)
		{
			if (Overlay!=EOverlay::None || IsPaused() || !Game->Inventory.bBurning || Game->Inventory.Equipped!=Before.Equipped)
				return Fail(TEXT("cancel altered burning firebrand"));
			++Step;
			return true;
		}
		if (Overlay!=EOverlay::None || IsPaused() || Game->Inventory.Equipped!=Item || (Before.bBurning && Game->Inventory.bBurning))
			return Fail(TEXT("confirm did not commit selection or extinguish"));
		UE_LOG(LogTemp,Display,TEXT("WHEEL INPUT PASSED: equipped %s, cancel with lit firebrand tested=%d"),*Item.ToString(),CheckCancel);
		bTestSelecting=false;
		return false;
	}), .075f);
}
#endif
