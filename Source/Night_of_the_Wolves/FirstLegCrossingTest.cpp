#include "FirstLegCrossing.h"
#if !UE_BUILD_SHIPPING
#include "FirstLegCrossingLayout.h"
#include "Night_of_the_WolvesPlayerController.h"
#include "WolfTrailGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

using namespace FirstLegLayout;

void AFirstLegCrossing::DriveTest(AWolfTrailGameMode& Game,APawn& Pawn,float Delta)
{
	// Test markers survive a local checkpoint restore. No world state or supplies are injected.
	static bool bTestedMiss=false, bTestedDiversionDeath=false, bSawCP3Restore=false, bSawCP4Restore=false;
	static bool bRushedBandit=false, bSawShotRestore=false, bStartedCancel=false, bCheckedCancel=false;
	static bool bPackReachedBankDuringSwing=false;
	const bool bClean=FParse::Param(FCommandLine::Get(),TEXT("FirstLegClean"));
	const bool bFilm=bClean && FParse::Param(FCommandLine::Get(),TEXT("FirstLegPhotos"));
	TestTime+=Delta;
	auto Fail=[&](const FString& Reason){
		UE_LOG(LogTemp,Error,TEXT("FIRST LEG TEST FAILED: %s, phase=%d CP=%d at %s"),*Reason,int32(Phase),Game.Checkpoint,*Pawn.GetActorLocation().ToString());
		FPlatformMisc::RequestExitWithStatus(false,1);
	};
	if (TestTime>105) { Fail(TEXT("crossing or escape stalled")); return; }
	auto* PC=Cast<ANight_of_the_WolvesPlayerController>(Pawn.GetController());
	if (!PC) { Fail(TEXT("missing controller"));return; }
	if (PC->bTestSelecting) return;
	auto Photo=[&](const TCHAR* Name) {
		static TSet<FString> Taken;
		if (!FParse::Param(FCommandLine::Get(),TEXT("FirstLegPhotos")) || Taken.Contains(Name)) return;
		Taken.Add(Name);
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ScreenShotDir()/(FString(Name)+TEXT(".png"))),true,false);
	};
	auto Move=[&](const FVector& Goal,float Radius=40.f){
		const FVector Direction=Goal-Pawn.GetActorLocation();
		if (Direction.Size2D()<Radius) return true;
		PC->SetControlRotation(FRotator(-8,Direction.Rotation().Yaw,0));
		Pawn.AddMovementInput(Direction.GetSafeNormal2D(),1.f);return false;
	};
	switch (Phase)
	{
	case EPhase::Approach: Move(CreekApproach); break;
	case EPhase::Ready:
		if (bStartedCancel && !bCheckedCancel)
		{
			if (!Game.Inventory.Has(TEXT("Rope")) || Cord->IsVisible()) {Fail(TEXT("interrupted cast consumed a rope"));return;}
			bCheckedCancel=true;
			UE_LOG(LogTemp,Display,TEXT("FIRST LEG TEST: stepped away during rope cast, rope retained"));
		}
		if (bTestedMiss && !bSawCP3Restore)
		{
			if (Game.Checkpoint!=3 || !Game.Inventory.Has(TEXT("Gold")) || !Game.Inventory.Has(TEXT("Rope")) || Cord->IsVisible() || !Game.Inventory.Has(TEXT("Firebrand")))
			{Fail(TEXT("CP3 failed to restore both supplies and unattached world rope"));return;}
			bSawCP3Restore=true;
			UE_LOG(LogTemp,Display,TEXT("FIRST LEG TEST CP3: current death restored rope and gold, attachment cleared"));
		}
		if (FVector::Dist2D(Pawn.GetActorLocation(),Takeoff)<570) Photo(TEXT("FirstLeg-Creek"));
		if (!Move(Takeoff,115)) break;
		if (Game.Inventory.Equipped!=TEXT("Rope")) { PC->RequestTestEquip(TEXT("Rope"));break; }
		Game.UseEquipped();break;
	case EPhase::Casting:
		if (!bClean && !bCheckedCancel) {bStartedCancel=true; Move(Takeoff-FVector(440,0,0));}
		break;
	case EPhase::Attached:
		// Also try leaving just before the lead wolf can reach the takeoff.
		if (!bClean && bSawCP3Restore && FVector::Dist2D(LastLead,Takeoff)>243) break;
		Game.Interact();break;
	case EPhase::Swinging:
		if (bSawCP3Restore && FVector::Dist2D(LastLead,Takeoff)<110 && FVector::Dist2D(LastLead,Pawn.GetActorLocation())>250)
			bPackReachedBankDuringSwing=true;
		if (!bClean && !bTestedMiss && ActionTime>.3f) {bTestedMiss=true;Game.Interact();}
		else if ((bClean || bSawCP3Restore) && ActionTime>OutwardSwingSeconds*.5f && ActionTime<OutwardSwingSeconds*.75f) Photo(TEXT("FirstLeg-Swing"));
		else if (ActionTime>OutwardSwingSeconds*.83f) Game.Interact();
		break;
	case EPhase::FarBank:
		if (!bClean && !bPackReachedBankDuringSwing) {Fail(TEXT("late swing did not separate Max from the wolf at takeoff"));return;}
		if (bRushedBandit && !bSawShotRestore)
		{
			if (Game.Checkpoint!=4 || !Game.Inventory.Has(TEXT("Gold")) || !Cord->IsVisible())
			{Fail(TEXT("bandit shot did not restore the far-bank opportunity"));return;}
			bSawShotRestore=true;
			UE_LOG(LogTemp,Display,TEXT("FIRST LEG TEST CP4: crossed bandit's aim boundary; fatal shot and retry"));
		}
		if (bTestedDiversionDeath && !bSawCP4Restore)
		{
			if (Game.Checkpoint!=4 || !Game.Inventory.Has(TEXT("Gold")) || Game.Inventory.Has(TEXT("Rope")) || Bag->IsVisible() || !Cord->IsVisible())
			{Fail(TEXT("CP4 failed to restore spent pouch, attached rope and undisturbed bandit"));return;}
			bSawCP4Restore=true;
			UE_LOG(LogTemp,Display,TEXT("FIRST LEG TEST CP4: diversion death restored one gold sack and the world rope"));
		}
		if (TestWaypoint<3 && Move(FarRoad[TestWaypoint+1],90)) ++TestWaypoint;
		else if (TestWaypoint>=3) Move(FarRoad[4]);
		break;
	case EPhase::Demand:
		if (!bClean && !bSawShotRestore)
		{
			bRushedBandit=true;
			Move(BanditMark);
			if (FVector::Dist2D(Pawn.GetActorLocation(),BanditMark)<510) Photo(TEXT("FirstLeg-Bandit"));
			break;
		}
		if (bClean && !IsAimingAtGold(Pawn)) Photo(TEXT("FirstLeg-Bandit"));
		if (Game.Inventory.Equipped!=TEXT("Gold")) {PC->RequestTestEquip(TEXT("Gold"));break;}
		if (!IsAimingAtGold(Pawn))
		{
			if (!bClean) Game.UseEquipped();
			if (!Game.Inventory.Has(TEXT("Gold"))) {Fail(TEXT("off-target throw consumed gold"));return;}
			PC->SetControlRotation((GoldLanding-Pawn.GetActorLocation()).Rotation());break;
		}
		if (bClean) Photo(TEXT("FirstLeg-Gold"));
		Game.UseEquipped();break;
	case EPhase::Diversion:
		if (!bClean && !bSawCP4Restore)
		{
			bTestedDiversionDeath=true;
			Move(GoldLanding,65);
			if (AttackTime>0 && AttackTime<2)
			{
				PC->SetControlRotation(FRotator(-8,(GoldLanding-Pawn.GetActorLocation()).Rotation().Yaw,0));
				Photo(TEXT("FirstLeg-Diversion"));
			}
		}
		else if (bFilm && TestWaypoint==5 && AttackTime<1.2f)
		{
			const FRotator Look=(BanditRoot->GetComponentLocation()+FVector(0,0,75)-Pawn.GetActorLocation()).Rotation();
			PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(),Look,Delta,3));
		}
		else if (TestWaypoint<7 && Move(FarRoad[TestWaypoint+1],80)) ++TestWaypoint;
		else if (TestWaypoint>=7) Move(Escape+FVector(155,30,0),30);
		break;
	case EPhase::Escaped:
		if (Game.Checkpoint!=5 || Game.Inventory.Items.Num()!=2 || !Game.Inventory.Has(TEXT("Shovel")) ||
			!Game.Inventory.Has(TEXT("Firebrand")) || Game.Inventory.bBurning || !Game.Inventory.Equipped.IsNone() || (!bClean && (!bSawCP3Restore || !bSawCP4Restore)))
		{Fail(TEXT("unexpected final equipment or skipped recovery branch"));return;}
		if (bClean && (FParse::Param(FCommandLine::Get(),TEXT("DoorwayTest")) || FParse::Param(FCommandLine::Get(),TEXT("OmitFirebrand"))))
		{
			UE_LOG(LogTemp,Display,TEXT("FIRST LEG DOORWAY PASSED: two guarded-door retries followed by a burning-brand exit and CP5; final inventory verified"));
		}
		else if (bClean)
		{
			UE_LOG(LogTemp,Display,TEXT("FIRST LEG CLEAN PASSED: CP0 to CP5 without retry in %.2f game seconds; final inventory verified"), GetWorld()->GetTimeSeconds());
		}
		else
		{
			UE_LOG(LogTemp,Display,TEXT("FIRST LEG TEST PASSED: CP0 to CP5; swing miss, gold diversion death, retries and final inventory verified"));
		}
		if (bFilm)
		{
			FTimerHandle EndFrame;
			GetWorldTimerManager().SetTimer(EndFrame,[]{FPlatformMisc::RequestExit(false);},2.5f,false);
		}
		else FPlatformMisc::RequestExit(false);
		break;
	default: break;
	}
}
#endif
