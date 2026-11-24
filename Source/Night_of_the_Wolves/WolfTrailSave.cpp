#include "WolfTrailSave.h"
#include "WolfTrailGameMode.h"
#include "FirstLegCrossingLayout.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	const FString& Slot()
	{
#if !UE_BUILD_SHIPPING
		static const FString Probe(TEXT("Automation-MoonlitTrail"));
		if (FParse::Param(FCommandLine::Get(),TEXT("FirstLegSaveProbe"))) return Probe;
#endif
		static const FString Live(TEXT("MoonlitTrail-v1"));
		return Live;
	}
}

bool UWolfTrailSave::Enabled(AWolfTrailGameMode& Mode)
{
	if (Mode.GetWorld()->WorldType != EWorldType::Game || FParse::Param(FCommandLine::Get(), TEXT("NoTrailSave"))) return false;
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(),TEXT("FirstLegSaveProbe"))) return true;
	if (Mode.bFirstLegTest || Mode.bCottageTest || Mode.bRecoveryTest) return false;
#endif
	return true;
}

bool UWolfTrailSave::IsUsable() const
{
	if ((Format!=1 && Format!=2) || Stage<1 || Stage>7 || !Spawn.IsValid() || Spawn.GetLocation().GetAbsMax()>1000000) return false;
	if ((!Equipment.IsNone() && !Bag.Contains(Equipment)) || (bLit && Equipment!=TEXT("Firebrand"))) return false;
	TSet<FName> Unique;
	for (FName Item : Bag)
	{
		if (Item.IsNone() || Unique.Contains(Item)) return false;
		Unique.Add(Item);
	}
	if (Stage==7 && (Format<2 || !bLit || Equipment!=TEXT("Firebrand"))) return false;
	if (Stage==3 && (!Bag.Contains(TEXT("Rope")) || !Bag.Contains(TEXT("Gold")))) return false;
	if (Stage>=4 && Bag.Contains(TEXT("Rope"))) return false;
	if (Stage==4 && !Bag.Contains(TEXT("Gold"))) return false;
	if (Stage>=5 && Bag.Contains(TEXT("Gold"))) return false;
	return true;
}

void UWolfTrailSave::Resume(AWolfTrailGameMode& Mode)
{
	if (!Enabled(Mode)) return;
#if !UE_BUILD_SHIPPING
	const bool bProbe = FParse::Param(FCommandLine::Get(),TEXT("FirstLegSaveProbe"));
	if (bProbe && Mode.bCottageTest) { Clear(Mode); return; }
#endif
	if (!UGameplayStatics::DoesSaveGameExist(Slot(),0))
	{
#if !UE_BUILD_SHIPPING
		if (bProbe)
		{
			UE_LOG(LogTemp, Error,TEXT("DISK CHECKPOINT TEST FAILED: missing test slot"));
			FPlatformMisc::RequestExitWithStatus(false,1);
		}
#endif
		return;
	}
	const auto* Saved=Cast<UWolfTrailSave>(UGameplayStatics::LoadGameFromSlot(Slot(),0));
	if (!Saved || !Saved->IsUsable())
	{
		UE_LOG(LogTemp, Warning, TEXT("Unreadable Moonlit Trail checkpoint, starting at trailhead."));
		return;
	}
	Mode.Checkpoint=Saved->Stage;
	Mode.CheckpointPouchSite=Saved->PouchSite;
	Mode.CheckpointItems.Items=Saved->Bag;
	Mode.CheckpointItems.Equipped=Saved->Equipment;
	Mode.CheckpointItems.bBurning=Saved->bLit;
	Mode.CheckpointTransform=Saved->Spawn;
	// Format 1 stage 6 was the retired drainage outlet. Rejoin at the
	// preceding earned bend so returning players see the new strike.
	if (Saved->Format==1 && Saved->Stage==6)
	{
		Mode.Checkpoint=5;
		Mode.CheckpointTransform=FTransform(FRotator(0,14,0),FirstLegLayout::Escape);
		Mode.CheckpointItems.Extinguish();
		UE_LOG(LogTemp,Display,TEXT("STORMFALL: previous outlet save resumed at the escape bend"));
	}
	Mode.RetryCheckpoint();
#if !UE_BUILD_SHIPPING
	if (bProbe)
	{
		const APawn* Pawn = Mode.GetWorld()->GetFirstPlayerController()->GetPawn();
		const bool bValid = Mode.Checkpoint==2 && Mode.Inventory.Items.Num()==1 && Mode.Inventory.Has(TEXT("Shovel")) && Mode.Inventory.Equipped==Saved->Equipment &&
			Mode.bPitDug && Mode.bHermitTrapped && Mode.bSheltered && !Mode.bDoorOpen && !Mode.Inventory.bBurning &&
			Pawn && FVector::Dist(Pawn->GetActorLocation(),Saved->Spawn.GetLocation())<20;
		UE_LOG(LogTemp, Display,TEXT("DISK CHECKPOINT TEST %s: fresh process restored CP%d, %d items, equipped %s"),
			bValid?TEXT("PASSED"):TEXT("FAILED"),Mode.Checkpoint,Mode.Inventory.Items.Num(),*Mode.Inventory.Equipped.ToString());
		Clear(Mode);
		FPlatformMisc::RequestExitWithStatus(false,bValid ? 0 : 1);
	}
#endif
}

void UWolfTrailSave::Write(AWolfTrailGameMode& Mode)
{
	if (!Enabled(Mode)) return;
	auto* Saved=NewObject<UWolfTrailSave>();
	Saved->Format=2;
	Saved->Stage=Mode.Checkpoint;
	Saved->Bag=Mode.CheckpointItems.Items;
	Saved->Equipment=Mode.CheckpointItems.Equipped;
	Saved->bLit=Mode.CheckpointItems.bBurning;
	Saved->Spawn=Mode.CheckpointTransform;
	Saved->PouchSite=Mode.CheckpointPouchSite;
	if (!Saved->IsUsable() || !UGameplayStatics::SaveGameToSlot(Saved,Slot(),0))
	{
		UE_LOG(LogTemp,Warning,TEXT("Could not store Moonlit Trail checkpoint CP%d"),Mode.Checkpoint);
		Mode.Say(TEXT("Checkpoint available for this session. Disk save failed."),4.f);
	}
}

void UWolfTrailSave::Clear(AWolfTrailGameMode& Mode)
{
	if (Enabled(Mode) && UGameplayStatics::DoesSaveGameExist(Slot(),0))
		UGameplayStatics::DeleteGameInSlot(Slot(),0);
}

void AWolfTrailGameMode::NewTrail()
{
	UWolfTrailSave::Clear(*this);
	UGameplayStatics::OpenLevel(this,FName(*UGameplayStatics::GetCurrentLevelName(this,true)));
}
