#include "FirstLegCrossing.h"
#if !UE_BUILD_SHIPPING
#include "FirstLegCrossingLayout.h"
#include "FirstLegCablewayLayout.h"
#include "Night_of_the_WolvesPlayerController.h"
#include "WolfTrailGameMode.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "WolfMeshComponent.h"
#include "WolfPackComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Algo/Count.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "InputKeyEventArgs.h"
#include "TimerManager.h"
#include "UnrealClient.h"

using namespace FirstLegLayout;

void AFirstLegCrossing::DriveTest(AWolfTrailGameMode& Game,APawn& Pawn,float Delta)
{
	// Test markers survive a local checkpoint restore. No world state or supplies are injected.
	static bool bTestedMiss=false, bTestedDiversionDeath=false, bSawCP3Restore=false, bSawCP4Restore=false;
	static bool bTriedWrongThrow=false, bSawWrongLanding=false, bRecoveredWrongThrow=false;
	static bool bRushedBandit=false, bSawShotRestore=false, bStartedCancel=false, bCheckedCancel=false;
	static bool bCheckedBareHook=false, bSawSafeRelease=false, bHeldAtStop=false, bCheckedPlanks=false;
	static bool bCheckedGrip=false, bSawCastApex=false;
	static bool bPackReachedBankDuringRide=false, bSawBanditFace=false, bSawBanditHit=false, bSawHandoff=false;
	static float TimeAtBuffer=0.f;
	static FVector NearHousing;
	static bool bWatchedDownstream=false, bWatchedSwim=false, bCreekViewReturned=false;
	static bool bRetriedLookback=false;
	static float SecondsAtBreak=0.f;
	static FVector MaxAtCreekShot, PreviousWolf, FirstSwimPosition;
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
			if (!Game.Inventory.Has(TEXT("Rope")) || Cord->IsVisible() || CastBight->IsVisible()) {Fail(TEXT("interrupted cast consumed the rope"));return;}
			bCheckedCancel=true;
			UE_LOG(LogTemp,Display,TEXT("CABLEWAY TEST: stepped away during the cast, empty trolley and rope restored"));
		}
		if (bTestedMiss && !bSawCP3Restore)
		{
			if (Game.Checkpoint!=3 || !Game.Inventory.Has(TEXT("Gold")) || !Game.Inventory.Has(TEXT("Rope")) || Cord->IsVisible() || !Game.Inventory.Has(TEXT("Firebrand")) || RideFraction!=0.f)
			{Fail(TEXT("CP3 failed to restore both supplies and unattached world rope"));return;}
			bSawCP3Restore=true;
			UE_LOG(LogTemp,Display,TEXT("FIRST LEG TEST CP3: current death restored rope and gold, attachment cleared"));
		}
		if (FVector::Dist2D(Pawn.GetActorLocation(),Takeoff)<570) Photo(TEXT("FirstLeg-Creek"));
		if (!Move(FirstLegCableway::RiderPosition(0),32)) break;
		if (Game.Inventory.Equipped!=TEXT("Rope"))
		{
			if (!bCheckedBareHook)
			{
				Game.Interact();
				Game.UseEquipped();
				if (Phase!=EPhase::Ready || Cord->IsVisible() || !Game.Inventory.Has(TEXT("Rope"))) {Fail(TEXT("empty trolley accepted the wrong equipment"));return;}
				bCheckedBareHook=true;
			}
			PC->RequestTestEquip(TEXT("Rope"));break;
		}
		Game.UseEquipped();break;
	case EPhase::Casting:
		if (ActionTime > .53f*CastRopeSeconds && ActionTime < .62f*CastRopeSeconds)
		{
			const auto* Max = CastChecked<ACharacter>(&Pawn);
			const double Feet = Max->GetActorLocation().Z-Max->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const double HardwareHeight = FirstLegCableway::HookSaddle(0).Z-Feet;
			const double Jump = Max->GetCharacterMovement()->GetMaxJumpHeightWithJumpTime();
			// A generous standing overhead reach for Max, including his fingertips.
			if (HardwareHeight < Jump+235. || !CastBight->IsVisible() || !CastBight->GetStaticMesh()
				|| !HangingPose || CastBight->GetComponentLocation().Z < FirstLegCableway::HookSaddle(0).Z+15.
				|| HangingPose->GetSocketLocation(TEXT("hand_r")).Z > Feet+235.)
			{Fail(TEXT("high hook, detached cast or throwing hand outside its expected reach"));return;}
			Photo(TEXT("FirstLeg-RopeCast"));
			if (!bSawCastApex)
				UE_LOG(LogTemp,Display,TEXT("CABLEWAY TEST: saddle %.1fcm above Max's feet, jump rises %.1fcm; airborne loop clears the hook"),HardwareHeight,Jump);
			bSawCastApex=true;
		}
		if (!bClean && !bCheckedCancel) {bStartedCancel=true; Move(Takeoff-FirstLegCableway::Axis*440);}
		break;
	case EPhase::Rigged:
		if (!bSawCastApex || CastBight->IsVisible() || !Ironwork || !Ironwork->GetStaticMesh() || !TrolleyWheel || !TrolleyWheel->GetStaticMesh() || !Cord || !Cord->GetStaticMesh())
		{Fail(TEXT("missing carriage, sheave or sling mesh"));return;}
		NearHousing=Ironwork->GetComponentLocation();
		if (!Cord->IsVisible() || Game.Inventory.Has(TEXT("Rope")) || Game.Inventory.bBurning) {Fail(TEXT("completed sling and inventory disagree"));return;}
		// Leave with a quarter-second of the loading-stage window remaining.
		// The directing wolf can hold back while the pressure wolf closes.
		if (!bClean && bSawCP3Restore && BankTime < NearBankSeconds-.25f) break;
		Game.Interact();break;
	case EPhase::Riding:
		if (bSawCP3Restore && RideFraction>.5f && !bPackReachedBankDuringRide)
		{
			double ClosestStage=BIG_NUMBER, ClosestRider=BIG_NUMBER;
			for (int32 I=0; I<UWolfPackComponent::Count; ++I)
			{
				const FVector Feet=Pack->GetMember(I)->GetComponentLocation();
				if (Feet.X>9910.+StreamBend(Feet.Y) || Feet.Z < -40.)
				{Fail(TEXT("pack crossed the flooded gap before Max landed"));return;}
				ClosestStage=FMath::Min(ClosestStage,FVector::Dist2D(Feet,Takeoff));
				ClosestRider=FMath::Min(ClosestRider,FVector::Dist2D(Feet,Pawn.GetActorLocation()));
			}
			if (ClosestStage<470. && ClosestRider>480.)
			{
				bPackReachedBankDuringRide=true;
				UE_LOG(LogTemp,Display,TEXT("CABLEWAY TEST: late launch, all five on shore; nearest wolf %.0fcm to stage, %.0fcm to rider"),ClosestStage,ClosestRider);
			}
		}
		if (Game.Checkpoint!=3 || !IsTraversalLocked()) {Fail(TEXT("cable ride committed checkpoint or unlocked early"));return;}
		if (!bCheckedGrip && RideFraction>.25f)
		{
			if (!IsValid(HangingPose)) {Fail(TEXT("missing player grip pose"));return;}
			const double Left=FVector::Dist(HangingPose->GetSocketLocation(TEXT("hand_l")),FirstLegCableway::GripPosition(RideFraction,true));
			const double Right=FVector::Dist(HangingPose->GetSocketLocation(TEXT("hand_r")),FirstLegCableway::GripPosition(RideFraction,false));
			if (FMath::Max(Left,Right)>17.f) {Fail(FString::Printf(TEXT("hands missed the moving sling by %.1f/%.1fcm"),Left,Right));return;}
			bCheckedGrip=true;
			UE_LOG(LogTemp,Display,TEXT("CABLEWAY TEST: wrist-to-grip clearance %.1f/%.1fcm"),Left,Right);
		}
		if (!bClean && !bTestedMiss && RideFraction>.33f) {bTestedMiss=true;Game.Interact();break;}
		if (RideFraction>.4f && RideFraction<.7f) Photo(TEXT("FirstLeg-Trolley"));
		if (bClean && IsAboveCatchDeck() && RideFraction<1.f)
		{
			bSawSafeRelease=true;
			Game.Interact();
		}
		else if (!bClean && RideFraction>=1.f)
		{
			if (FVector::Dist(Pawn.GetActorLocation(),FirstLegCableway::RiderPosition(1.f))>8.f)
			{Fail(TEXT("parked trolley and rider did not stop together"));return;}
			TimeAtBuffer+=Delta;
			if (TimeAtBuffer>1.2f)
			{
				bHeldAtStop=true;
				UE_LOG(LogTemp,Display,TEXT("CABLEWAY TEST: held above catch deck for %.2fs; manual dismount"),TimeAtBuffer);
				Game.Interact();
			}
		}
		break;
	case EPhase::CreekLookback:
	{
		if (bRetriedLookback || bCreekViewReturned || Game.Checkpoint!=4 || !IsWatchingCreekPack() ||
			!PC->IsMoveInputIgnored() || !PC->IsLookInputIgnored() || !GetLeadWolf())
		{Fail(TEXT("creek shot replayed or did not hold Max and the live pack"));return;}
		if (MaxAtCreekShot.IsZero()) MaxAtCreekShot=Pawn.GetActorLocation();
		if (FVector::Dist2D(MaxAtCreekShot,Pawn.GetActorLocation())>5.f)
		{Fail(TEXT("Max drifted while watching the wolves"));return;}
		const FVector Wolf=GetLeadWolf()->GetComponentLocation();
		const FVector BrokenSpan(9780,-1510,50);
		if (FVector::Dist2D(Wolf,BrokenSpan)<215 && FVector::Dist2D(PreviousWolf,Wolf)<Delta*20.f)
			SecondsAtBreak+=Delta;
		if (SecondsAtBreak>.55f && Wolf.Y<-1775) bWatchedDownstream=true;
		if (bWatchedDownstream && Wolf.Z<-165.f && Wolf.X>9800)
		{
			if (FirstSwimPosition.IsZero()) FirstSwimPosition=Wolf;
			if (Wolf.X-FirstSwimPosition.X>65.f) bWatchedSwim=true;
		}
		if (CreekShotTime>1.f && CreekShotTime<4.f && PC->GetViewTarget()!=CreekCamera)
		{Fail(TEXT("camera left the bridge before the wolves turned downstream"));return;}
		if (SecondsAtBreak>.55f && FParse::Param(FCommandLine::Get(),TEXT("CreekRetryTest")))
		{
			bRetriedLookback=true;
			Game.RetryCheckpoint();
			return;
		}
		if (SecondsAtBreak>.55f && !bWatchedDownstream) Photo(TEXT("Creek-Pack-Hesitation"));
		if (bWatchedSwim) Photo(TEXT("Creek-Pack-Swim"));
		PreviousWolf=Wolf;
		break;
	}
	case EPhase::FarBank:
		if (bRetriedLookback)
		{
			if (IsWatchingCreekPack() || IsTraversalLocked() || PC->GetViewTarget()!=&Pawn ||
				PC->IsMoveInputIgnored() || PC->IsLookInputIgnored() || Game.Checkpoint!=4 || Game.Inventory.Has(TEXT("Rope")))
				{Fail(TEXT("retry during creek shot did not restore the far bank and controls"));return;}
			UE_LOG(LogTemp,Display,TEXT("CREEK RETRY PASSED: interrupted the bridge shot; CP4 restored Max, his controls and the spent rope"));
			FPlatformMisc::RequestExit(false);
			return;
		}
		if (!bCreekViewReturned)
		{
			if (!bWatchedSwim || PC->GetViewTarget()!=&Pawn || PC->IsMoveInputIgnored() || PC->IsLookInputIgnored())
			{Fail(FString::Printf(TEXT("creek handoff incomplete, pause %.2fs downstream %d swim %d"),SecondsAtBreak,bWatchedDownstream,bWatchedSwim));return;}
			bCreekViewReturned=true;
			UE_LOG(LogTemp,Display,TEXT("CREEK LOOKBACK PASSED: live leader held at bridge %.2fs, went downstream and swam; view and input returned to Max"),SecondsAtBreak);
		}
		if (!bCheckedPlanks)
		{
			const double Along=FVector::DotProduct(Pawn.GetActorLocation()-FirstLegCableway::FarStop,FirstLegCableway::Axis);
			const double PlankZ=FirstLegCableway::CatchDeckHeight(Along);
			const double Feet=Pawn.GetActorLocation().Z-CastChecked<ACharacter>(&Pawn)->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			if (FMath::Abs(Feet-PlankZ)>7.f) {Fail(FString::Printf(TEXT("landed at feet Z %.1f over plank top %.1f"),Feet,PlankZ));return;}
			bCheckedPlanks=true;
			UE_LOG(LogTemp,Display,TEXT("CABLEWAY TEST: physical landing %.1fcm above plank surface"),Feet-PlankZ);
		}
		if (RideFraction>=1.f && !(Ironwork->GetComponentLocation()-NearHousing).Equals(FirstLegCableway::FarStop-FirstLegCableway::NearStop,10.f))
		{Fail(FString::Printf(TEXT("carriage did not reach the far stop, near %s now %s"),*NearHousing.ToString(),*Ironwork->GetComponentLocation().ToString()));return;}
		if (!bClean && !bPackReachedBankDuringRide) {Fail(TEXT("late ride did not separate Max from the wolf at takeoff"));return;}
		if (IsTraversalLocked() || (bClean ? !bSawSafeRelease : !bHeldAtStop) || Game.Inventory.Has(TEXT("Rope")) || !bCheckedBareHook)
		{Fail(TEXT("far landing, equipment control or release branch incorrect"));return;}
		if (bTriedWrongThrow && !bRecoveredWrongThrow)
		{
			if (!bSawWrongLanding || Game.Checkpoint!=4 || !Game.Inventory.Has(TEXT("Gold")) || Bag->IsVisible() || bWolfIntercept || bBanditStruck)
			{Fail(TEXT("wrong-throw loss did not restore CP4 and the single sack"));return;}
			bRecoveredWrongThrow=true;
			UE_LOG(LogTemp,Display,TEXT("BANDIT TEST wrong throw reset to CP4 with the sack restored"));
		}
		if (bRushedBandit && !bSawShotRestore)
		{
			if (Game.Checkpoint!=4 || !Game.Inventory.Has(TEXT("Gold")) || !Cord->IsVisible() || RideFraction!=1.f)
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
		if (Game.Inventory.Equipped!=TEXT("Gold")) {PC->RequestTestEquip(TEXT("Gold"));break;}
		if (Game.InteractionPrompt != TEXT("F  Throw gold"))
		{
			Fail(TEXT("Equipped gold must show the same throw action at every aim angle"));
			return;
		}
		if (!bClean && !bTriedWrongThrow)
		{
			// Uphill, away from the swimming pack. The bag must actually leave
			// his hand here and cost him his only chance on this attempt.
			PC->SetControlRotation(FRotator(8,35,0));
			bTriedWrongThrow=true;
		}
		else
		{
			FRotator Aim = (GoldPatch-Pawn.GetActorLocation()).Rotation();
			// The successful repeat uses the opposite edge of the approach. No
			// single fixed gold marker can satisfy all three throws in this run.
			if (bSawCP4Restore) { Aim.Yaw+=20.; Aim.Pitch=5.; }
			PC->SetControlRotation(Aim);
			Photo(TEXT("FirstLeg-Gold"));
		}
		Game.UseEquipped();
		if (Phase!=EPhase::Diversion || Game.Inventory.Has(TEXT("Gold")))
		{ Fail(TEXT("the aimed throw did not release the only sack")); return; }
		break;
	case EPhase::Diversion:
		if (bTriedWrongThrow && !bRecoveredWrongThrow)
		{
			if (bPouchLanded)
			{
				if (bWolfIntercept || !Bag->IsVisible() || GoldLanding.X<TossOrigin.X+125. || AttackViewer.IsValid() || bBanditStruck)
				{ Fail(TEXT("wrong throw snapped to the wolf approach or diverted wolves")); return; }
				if (!bSawWrongLanding) UE_LOG(LogTemp,Display,TEXT("BANDIT TEST wrong throw spent the sack and landed uphill; pack still hunts Max"));
				bSawWrongLanding=true;
				Photo(TEXT("FirstLeg-Wrong-Throw"));
			}
			break;
		}
		if (AttackViewer.IsValid())
		{
			if (PC->GetViewTarget() != AttackCamera || !PC->IsMoveInputIgnored())
			{ Fail(TEXT("bandit cutaway failed to take the camera")); return; }
			if (AttackTime > .55f && AttackTime < RealizationSeconds)
			{
				bSawBanditFace = true;
				Photo(TEXT("FirstLeg-Bandit-Alarm"));
			}
			if (bBanditStruck && AttackTime > BanditImpactTime() + .20f && AttackTime < BanditImpactTime() + .5f)
			{
				if (!bSawBanditFace || !GetLeadWolf())
				{ Fail(TEXT("bandit strike before a visible alarm or without the live pack")); return; }
				bSawBanditHit = true;
				Photo(TEXT("FirstLeg-Bandit-Takedown"));
			}
			break;
		}
		if (bBanditStruck && PC->GetViewTarget() == &Pawn && !PC->IsMoveInputIgnored()) bSawHandoff = true;
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
		else if (!bBanditStruck || !bSawHandoff)
		{
			// It is still his road until Max sees the pack take him down.
			if (bFilm)
			{
				const FRotator Look=(BanditRoot->GetComponentLocation()+FVector(0,0,75)-Pawn.GetActorLocation()).Rotation();
				PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(),Look,Delta,3));
			}
		}
		else if (TestWaypoint<7 && Move(FarRoad[TestWaypoint+1],80)) ++TestWaypoint;
		else if (TestWaypoint>=7) Move(Escape+FVector(155,30,0),30);
		break;
	case EPhase::Escaped:
		if (!bSawBanditFace || !bSawBanditHit || !bSawHandoff || !bBanditStruck ||
			Game.Checkpoint!=5 || Game.Inventory.Items.Num()!=2 || !Game.Inventory.Has(TEXT("Shovel")) ||
			!Game.Inventory.Has(TEXT("Firebrand")) || Game.Inventory.bBurning || !Game.Inventory.Equipped.IsNone() || (!bClean && (!bSawCP3Restore || !bSawCP4Restore || !bRecoveredWrongThrow)))
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
			UE_LOG(LogTemp,Display,TEXT("FIRST LEG TEST PASSED: CP0 to CP5; early drop in current, parked trolley dismount, gold diversion death, retries and final inventory verified"));
		}
		if (FParse::Param(FCommandLine::Get(),TEXT("IntoStorm")))
		{
			PC->ConsoleCommand(TEXT("Wolf.TestStormfall"),true);
			break;
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

namespace
{
enum class ERobberyAttempt { UphillSprint, ShoulderSprint, StandStill, EarlySprint, WaitForAttack };
const TCHAR* AttemptName(ERobberyAttempt Attempt)
{
	switch (Attempt)
	{
	case ERobberyAttempt::UphillSprint: return TEXT("uphill toss and road sprint");
	case ERobberyAttempt::ShoulderSprint: return TEXT("cross-trail toss and shoulder sprint");
	case ERobberyAttempt::StandStill: return TEXT("short toss and wait");
	case ERobberyAttempt::EarlySprint: return TEXT("good toss and early sprint");
	default: return TEXT("good toss and wait for takedown");
	}
}
}

void AFirstLegCrossing::StartRobberyTest()
{
	// Start with an earned CP4 save in a temporary user directory. Only the
	// game creates the robber, pack, spent sack, kills, new pawns and next save.
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer,[this, Attempt=ERobberyAttempt::UphillSprint,
		Waypoint=1, ThrowAt=-1.f, Start=GetWorld()->GetTimeSeconds(), bSawDeath=false,
		bCheckedLanding=false, bSawShot=false, bSprint=false, bReachedShoulder=false,
		DistanceRun=0., PeakSpeed=0., WalkSpeed=0.f, Release=FVector::ZeroVector, PreviousMax=TWeakObjectPtr<ACharacter>()]() mutable
	{
		auto* Game=GetWorld()->GetAuthGameMode<AWolfTrailGameMode>();
		auto* PC=Cast<ANight_of_the_WolvesPlayerController>(GetWorld()->GetFirstPlayerController());
		auto* Max=PC ? PC->GetCharacter() : nullptr;
		if (!Game || !Max) return;
		const float Now=GetWorld()->GetTimeSeconds();
		const FVector Here=Max->GetActorLocation();
		const bool bSuccess=Attempt==ERobberyAttempt::WaitForAttack;
		const bool bGoodToss=bSuccess || Attempt==ERobberyAttempt::EarlySprint;
		const bool bOffRoad=Attempt==ERobberyAttempt::ShoulderSprint;
		const bool bRunImmediately=!bSuccess && Attempt!=ERobberyAttempt::StandStill;
		auto Fail=[&](const TCHAR* Why)
		{
			UE_LOG(LogTemp,Error,TEXT("ROBBERY TEST FAILED: %s; %s, CP%d phase %d at %s, %.2fs since throw, travelled %.0f cm"),
				Why,AttemptName(Attempt),Game->Checkpoint,int32(Phase),*Here.ToCompactString(),Now-ThrowAt,DistanceRun);
			FPlatformMisc::RequestExitWithStatus(false,1);
		};
		auto Sprint=[&](bool bEnable)
		{
			if (bEnable==bSprint) return;
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift,bEnable ? IE_Pressed : IE_Released,bEnable ? 1.f : 0.f));
			bSprint=bEnable;
		};
		auto Move=[&](const FVector& Goal)
		{
			const FVector Dir=(Goal-Here).GetSafeNormal2D();
			PC->SetControlRotation(FRotator(-8,Dir.Rotation().Yaw,0));
			Max->AddMovementInput(Dir);
		};
		if (Now-Start>185.f) {Fail(TEXT("scenario exceeded time budget"));return;}
		if (Game->Checkpoint==5)
		{
			if (!bSuccess || !bSawShot || !bBanditStruck || !bCheckedLanding || Game->Inventory.Has(TEXT("Gold")) || Phase!=EPhase::Escaped)
			{Fail(TEXT("escaped without witnessing the successful trick"));return;}
			UE_LOG(LogTemp,Display,TEXT("ROBBERY TEST PASSED: four fatal choices retried at CP4; actual wolf strike, handoff and sprint earned CP5"));
			FPlatformMisc::RequestExit(false);
			return;
		}
		if (Game->Checkpoint!=4) {Fail(TEXT("requires isolated far-bank checkpoint"));return;}
		if (Game->IsDead())
		{
			if (bSawDeath) return;
			if (bSuccess || !bCheckedLanding || bBanditStruck || Game->Inventory.Has(TEXT("Gold")) ||
				(Game->DeathCause!=TEXT("Bandit") && Game->DeathCause!=TEXT("Wolves")))
			{Fail(TEXT("unexpected death or incorrect pouch/robber state"));return;}
			if ((bRunImmediately && (DistanceRun<125. || PeakSpeed<WalkSpeed*1.10f || !bSprint)) || (bOffRoad && !bReachedShoulder))
			{Fail(TEXT("failed attempt did not exercise real sprint / shoulder movement"));return;}
			UE_LOG(LogTemp,Display,TEXT("ROBBERY TEST death: %s by %s, %.2fs from release, moved %.0f cm at up to %.0f cm/s, release %s to %s"),
				AttemptName(Attempt),*Game->DeathCause,Now-ThrowAt,DistanceRun,PeakSpeed,*Release.ToCompactString(),*Here.ToCompactString());
			bSawDeath=true;
			PreviousMax=Max;
			Sprint(false);
			return;
		}
		if (bSawDeath)
		{
			if (PreviousMax.Get()==Max || Phase!=EPhase::FarBank || Bag->IsVisible() || bBanditStruck || bWolfIntercept ||
				Algo::Count(Game->Inventory.Items,FName(TEXT("Gold")))!=1 || !Cord->IsVisible() || Game->Inventory.Has(TEXT("Rope")) ||
				PC->IsMoveInputIgnored() || PC->GetViewTarget()!=Max || FVector::Dist2D(Here,Landing)>100.)
			{Fail(TEXT("automatic retry failed to restore the far bank, control and one sack"));return;}
			UE_LOG(LogTemp,Display,TEXT("ROBBERY TEST retry: %s restored one sack at CP4"),AttemptName(Attempt));
			Attempt=static_cast<ERobberyAttempt>(int32(Attempt)+1);
			bSawDeath=bCheckedLanding=bReachedShoulder=bSawShot=false;
			ThrowAt=-1.f; DistanceRun=PeakSpeed=0.; Waypoint=1;
			return;
		}
		if (PC->bTestSelecting) return;
		if (Phase==EPhase::FarBank)
		{
			if (Waypoint<4 && FVector::Dist2D(Here,FarRoad[Waypoint])<75.) ++Waypoint;
			Move(FarRoad[Waypoint]);
			return;
		}
		if (Phase==EPhase::Demand)
		{
			if (bOffRoad && Here.Y>-1095.) {Move(FVector(13650.,-1180.,Here.Z));return;}
			if (Game->Inventory.Equipped!=TEXT("Gold")) {PC->RequestTestEquip(TEXT("Gold"));return;}
			if (Game->InteractionPrompt!=TEXT("F  Throw gold")) {Fail(TEXT("missing neutral throw action"));return;}
			FRotator Aim=(GoldPatch-Here).Rotation();
			if (!bGoodToss) Aim=bOffRoad ? FRotator(6,-90,0) :
				Attempt==ERobberyAttempt::StandStill ? FRotator(-70,35,0) : FRotator(8,35,0);
			PC->SetControlRotation(Aim);
			Game->UseEquipped();
			if (Phase!=EPhase::Diversion || Game->Inventory.Has(TEXT("Gold"))) {Fail(TEXT("aimed throw failed to spend single sack"));return;}
			if (Game->CommitCheckpoint(5,Escape,FRotator::ZeroRotator)) {Fail(TEXT("CP5 accepted while live bandit still controlled road"));return;}
			ThrowAt=Now; Release=Here; Waypoint=4; WalkSpeed=Max->GetCharacterMovement()->MaxWalkSpeed;
		}
		if (Phase!=EPhase::Diversion) {Fail(TEXT("unexpected crossing phase"));return;}
		if (bPouchLanded && !bCheckedLanding)
		{
			if (bWolfIntercept!=bGoodToss) {Fail(TEXT("throw landed on wrong side of wolf approach"));return;}
			bCheckedLanding=true;
		}
		if (AttackViewer.IsValid())
		{
			if (!bSuccess) {Fail(TEXT("unearned cutaway while escaping the live robber"));return;}
			bSawShot=PC->GetViewTarget()==AttackCamera && PC->IsMoveInputIgnored();
			return;
		}
		if (bRunImmediately || (bSuccess && bSawShot && bBanditStruck && PC->GetViewTarget()==Max))
		{
			Sprint(true);
			DistanceRun=FMath::Max(DistanceRun,FVector::Dist2D(Release,Here));
			PeakSpeed=FMath::Max(PeakSpeed,Max->GetVelocity().Size2D());
			const FVector RoadFoot=FMath::ClosestPointOnSegment(Here,FarRoad[3],FarRoad[5]);
			bReachedShoulder|=FVector::Dist2D(Here,RoadFoot)>340.;
			if (bOffRoad) Move(FVector(Escape.X,-1050.,Here.Z));
			else
			{
				if (Waypoint<8 && FVector::Dist2D(Here,FarRoad[Waypoint])<80.) ++Waypoint;
				Move(Waypoint<8 ? FarRoad[Waypoint] : Escape+FVector(130,0,0));
			}
		}
		if (Now-ThrowAt>36.f || (!bSuccess && Here.X>=Escape.X-180.))
			Fail(TEXT("Max outlived failed throw or reached escape bend with living bandit"));
	},.012f,FTimerManagerTimerParameters{.bLoop=true,.bMaxOncePerFrame=true});
}

static FAutoConsoleCommandWithWorld RobberyProbe(TEXT("Wolf.TestRobbery"),TEXT("Physically race the bandit after wrong and correct throws from an isolated CP4 save"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (!World || World->WorldType!=EWorldType::Game) return;
		TActorIterator<AFirstLegCrossing> Crossing(World);
		if (Crossing) {Crossing->StartRobberyTest();return;}
		UE_LOG(LogTemp,Error,TEXT("ROBBERY TEST FAILED: missing crossing"));
		FPlatformMisc::RequestExitWithStatus(false,1);
	}));
#endif
