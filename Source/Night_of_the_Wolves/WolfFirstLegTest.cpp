#include "WolfTrailGameMode.h"

#if !UE_BUILD_SHIPPING
#include "FirstLegCrossing.h"
#include "CottageHermit.h"
#include "WolfCottageSet.h"
#include "Night_of_the_WolvesPlayerController.h"
#include "WolfTrailRoute.h"
#include "Components/SplineComponent.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

void AWolfTrailGameMode::DriveTest(float Dt,APawn& Pawn)
{
	TestElapsed += Dt;
	auto Fail=[this,&Pawn](const TCHAR* Why) {
		UE_LOG(LogTemp,Error,TEXT("FIRST LEG TEST FAILED %s step %d CP%d %.1fs at %s: %s"),Why,TestStep,Checkpoint,TestElapsed,*Pawn.GetActorLocation().ToCompactString(),*InteractionPrompt);
		FPlatformMisc::RequestExitWithStatus(false,1);
	};
	if (TestElapsed > 235) { Fail(TEXT("timeout")); return; }
	const FVector P=Pawn.GetActorLocation();
	if (bHermitReacting)
	{
		const float OnBoards = HermitElapsed - HermitReactionSeconds - HermitRunSeconds;
		if (OnBoards >= 0 && FVector::DistSquared2D(Hermit->GetActorLocation(),Cottage->PitSurface()) > 1.f)
		{
			Fail(TEXT("hermit missed the diggable timbers"));
			return;
		}
		if (OnBoards >= 0 && OnBoards < Dt)
		{
			UE_LOG(LogTemp,Display,TEXT("COTTAGE HERMIT reached %s hollow at %s after %.2fs; Max watched from %s"),
				bPitDug ? TEXT("open") : TEXT("covered"), *Hermit->GetActorLocation().ToCompactString(),
				HermitElapsed,*P.ToCompactString());
			if (P.X > Cottage->PitSurface().X-130.f || FMath::Abs(P.Y-Cottage->PitSurface().Y) < Cottage->GetPitBounds().GetExtent().Y+25.f)
			{
				Fail(TEXT("Max failed to clear the hermit's path on solid ground"));
				return;
			}
		}
		if (FParse::Param(FCommandLine::Get(),TEXT("CottagePhotos")))
		{
			FString Name;
			if (bPitDug && bHermitFired) Name = TEXT("Hermit-Fall");
			else if (OnBoards >= 1.15f && !bPitDug) Name = TEXT("Hermit-Aim");
			else if (HermitElapsed > HermitReactionSeconds + .54f * HermitRunSeconds && OnBoards < 0.f)
				Name = bPitDug ? TEXT("Hermit-Run-Open") : TEXT("Hermit-Run-Covered");
			static TSet<FString> HermitPhotos;
			if (!Name.IsEmpty() && !HermitPhotos.Contains(Name))
			{
				const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("FirstLegReview"));
				IFileManager::Get().MakeDirectory(*Folder,true);
				FScreenshotRequest::RequestScreenshot(Folder/(Name+TEXT(".png")),false,false);
				HermitPhotos.Add(Name);
			}
		}
	}
	// Optional visual review of the normal gameplay camera at the exterior and hearth.
	if (FParse::Param(FCommandLine::Get(),TEXT("CottagePhotos")))
	{
		static TSet<FString> Taken;
		const bool bYard=Checkpoint==1 && TestStep==3;
		const bool bRoom=Checkpoint==2 && TestStep==10;
		if (bYard || bRoom)
		{
			const FVector Focus=bYard?FVector(2710,80,260):FVector(3025,190,105);
			Pawn.GetController()->SetControlRotation((Focus-P).Rotation());
			const FString Name=bYard?TEXT("Cottage-Approach"):TEXT("Cottage-Fireplace");
			if (!Taken.Contains(Name) && (bYard ? P.X>1620 : P.X>2810))
			{
				const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("FirstLegReview"));
				IFileManager::Get().MakeDirectory(*Folder,true);
				FScreenshotRequest::RequestScreenshot(Folder/(Name+TEXT(".png")),false,false);
				Taken.Add(Name);
			}
		}
	}
	auto Move=[&](FVector Target,float Radius=18.f) {
		const FVector Dir=(Target-P).GetSafeNormal2D();
		if (FParse::Param(FCommandLine::Get(),TEXT("FirstLegPhotos")) && (TestStep==0 || TestStep>=14))
		{
			const FRotator Facing(-9.f,Dir.Rotation().Yaw,0);
			Pawn.GetController()->SetControlRotation(FMath::RInterpTo(Pawn.GetControlRotation(),Facing,Dt,5.f));
		}
		if (FVector::DistSquared2D(Target,P)<FMath::Square(Radius))
		{
			if (ACharacter* C=Cast<ACharacter>(&Pawn)) C->GetCharacterMovement()->StopMovementImmediately();
			return true;
		}
		Pawn.AddMovementInput(Dir);
		return false;
	};
	auto FollowSpline=[&](const FVector& Goal) {
		if(!Route.IsValid()) return Move(Goal);
		const float D=RouteDistance(P);
		if(D>=RouteDistance(Goal)-20) return true;
		Move(Route->Spline->GetLocationAtDistanceAlongSpline(D+270,ESplineCoordinateSpace::World),2);
		return false;
	};
	const int32 Previous=TestStep;
	const bool bDoorTest=bFirstLegTest && (FParse::Param(FCommandLine::Get(),TEXT("DoorwayTest")) ||
		FParse::Param(FCommandLine::Get(),TEXT("OmitFirebrand")));
	const bool bSkipBrand=bDoorTest && DoorChallengeAttempt==0;
	// Remember the observed pre-action budgets across input frames on the recovery run.
	static float DamagedDoorBudget = 0.f;
	static float SpentFireBudget = 0.f;
	switch(TestStep)
	{
	case 0:
		if(FollowSpline(FVector(1400,-900,0)) && Checkpoint==1) ++TestStep;
		break;
	case 1:
		// Exercise the lethal unprepared knock and the CP1 retry on the recovery run.
		if(bRecoveryTest)
		{
			if(Move(FVector(2310,-75,0))){Interact(); if(bHermitReacting) ++TestStep;}
		}
		else ++TestStep;
		break;
	case 2:
		if(bRecoveryTest)
		{
			if(bHermitReacting) break;
			if(Checkpoint!=1 || !Inventory.Items.IsEmpty() || bPitDug || bDoorOpen) { Fail(TEXT("CP1 retry"));return; }
		}
		++TestStep;
		break;
	case 3:
		if(Move(FVector(2300,280,0))) {Interact(); if(Inventory.Has(TEXT("Shovel"))) ++TestStep;}
		break;
	case 4:
		if(Inventory.Equipped==TEXT("Shovel")) ++TestStep;
		else if(auto* PC=Cast<ANight_of_the_WolvesPlayerController>(Pawn.GetController()))
			PC->RequestTestEquip(TEXT("Shovel"));
		break;
	case 5:
		if(Move(FVector(2350,197,0))) { UseEquipped(); if(DigElapsed>=0) ++TestStep; }
		break;
	case 6:
		if(!bPitDug || !Move(FVector(2350,202,0))) break;
		if (!Inventory.Equipped.IsNone())
		{
			Interact();
			if (bHermitReacting || bDoorOpen || Target(Pawn,false)!=ETarget::HandsFull)
			{ Fail(TEXT("a held tool allowed a knock")); return; }
			if (auto* PC=Cast<ANight_of_the_WolvesPlayerController>(Pawn.GetController())) PC->RequestTestEquip(NAME_None);
			break;
		}
		Interact();
		if(bHermitReacting) ++TestStep;
		break;
	case 7: if(bHermitTrapped && Move(FVector(2010,0,0))) ++TestStep; break;
	case 8:
		Move(FVector(2590,0,0));
		if(P.X >= Cottage->GetPitBounds().Min.X-65)
		{
			if(ACharacter* Character=Cast<ACharacter>(&Pawn)) Character->Jump();
			++TestStep;
		}
		break;
	case 9:
		if (Move(FVector(2570,0,0)))
		{
			Interact();
			if (Checkpoint==2) TestStep=bRecoveryTest ? 30 : 10;
		}
		break;
	case 10:
		if(bCottageTest){
			UE_LOG(LogTemp,Display,TEXT("M3 SELFTEST PASSED: moved, equipped shovel, cleared hollow, trapped hermit, barred door in %.2fs"),TestElapsed);
			FPlatformMisc::RequestExit(false);return;
		}
		if(bSkipBrand) { ++TestStep; break; }
		// Take the firebrand first so the supply pickup exercises equipment preservation.
		if(Move(FVector(2950,130,0))) { Interact(); if(Inventory.bBurning) ++TestStep; } break;
	case 11:
	case 12:
	{
		const bool bFirstPickup = TestStep == 11;
		const bool bTakeGold = bFirstPickup != bRecoveryTest;
		const FName Item = bTakeGold ? TEXT("Gold") : TEXT("Rope");
		const FName Other = bTakeGold ? TEXT("Rope") : TEXT("Gold");
		const FVector Stand = bTakeGold ? FVector(2800,-130,0) : FVector(3090,-84,0);
		if (!Move(Stand)) break;
		const FName Equipment = Inventory.Equipped;
		Interact();
		Interact(); // A second press at the same prop cannot grant its counterpart.
		if (!Inventory.Has(Item) || Inventory.Has(Other) == bFirstPickup ||
			Inventory.Equipped != Equipment || Inventory.bBurning == bSkipBrand)
		{ Fail(TEXT("independent cabin pickup or held equipment")); return; }
		if (bFirstPickup && CommitCheckpoint(3,P,Pawn.GetActorRotation()))
		{ Fail(TEXT("CP3 accepted one supply")); return; }
		UE_LOG(LogTemp,Display,TEXT("COTTAGE PICKUP: took %s, %s in pack=%d"),*Item.ToString(),*Other.ToString(),Inventory.Has(Other));
		if (bFirstPickup) TestStep = bRecoveryTest ? 26 : 12;
		else TestStep = bRecoveryTest ? 32 : (bDoorTest && DoorChallengeAttempt<2 ? 40 : 14);
		break;
	}
	case 40:
		if (!Move(FVector(2575,0,0))) break;
		Pawn.GetController()->SetControlRotation(FRotator(-8,180,0));
		if (DoorChallengeAttempt==0 && !Inventory.Equipped.IsNone())
		{
			if (auto* PC=Cast<ANight_of_the_WolvesPlayerController>(Pawn.GetController()))
				PC->RequestTestEquip(NAME_None);
			break;
		}
		Interact();
		if (!bDoorOpen) { Fail(TEXT("door did not open from inside")); return; }
		DoorOpenedAt=TestElapsed;
		++TestStep;
		break;
	case 41:
		// Stand just inside first so the guarded doorway and the fire recoil can be seen.
		if (!bDoorOpen) { Fail(TEXT("opening the door from indoors killed Max")); return; }
		if (TestElapsed-DoorOpenedAt<1.1f) break;
		if (FParse::Param(FCommandLine::Get(),TEXT("EncounterPhotos")))
		{
			const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("EncounterReview"));
			IFileManager::Get().MakeDirectory(*Folder,true);
			const TCHAR* Name=DoorChallengeAttempt==0 ? TEXT("Door-Waiting.png") : TEXT("Door-Flame.png");
			FScreenshotRequest::RequestScreenshot(Folder/Name,true,false);
		}
		++TestStep;
		break;
	case 42:
		if (DoorChallengeAttempt==1 && Inventory.bBurning)
		{
			if (auto* PC=Cast<ANight_of_the_WolvesPlayerController>(Pawn.GetController()))
				PC->RequestTestEquip(TEXT("Rope"));
			break;
		}
		++TestStep;
		break;
	case 43:
		if (DoorChallengeAttempt==1 && Inventory.Equipped!=TEXT("Firebrand"))
		{
			if (auto* PC=Cast<ANight_of_the_WolvesPlayerController>(Pawn.GetController()))
				PC->RequestTestEquip(TEXT("Firebrand"));
			break;
		}
		if (Inventory.bBurning) { Fail(TEXT("doorway probe should be unprotected")); return; }
		if (DoorChallengeAttempt==1)
			if (ACharacter* C=Cast<ACharacter>(&Pawn)) C->Jump();
		++TestStep;
		break;
	case 44:
		if (bDoorOpen)
		{
			if (P.X < CabinDoorLocation.X-20) { Fail(TEXT("unprotected player passed the threshold")); return; }
			Move(FVector(2325,0,0));
			break;
		}
		if (Checkpoint!=2 || Inventory.Items.Num()!=1 || !Inventory.Has(TEXT("Shovel")) || !Inventory.Equipped.IsNone() ||
			!IsInside(P) || !bDoorwayGuarded)
		{ Fail(TEXT("doorway capture did not restore the barred cabin")); return; }
		UE_LOG(LogTemp,Display,TEXT("DOORWAY TEST: %s departure intercepted; secured CP2 and starting equipment restored"),
			DoorChallengeAttempt==0 ? TEXT("empty-handed") : TEXT("extinguished brand"));
		++DoorChallengeAttempt;
		TestStep=10;
		break;
	case 30:
		if(!Move(FVector(2575,0,0)) || DoorRemaining > DoorEnduranceSeconds*.80f) break;
		DamagedDoorBudget=DoorRemaining;
		Interact();
		if (!bDoorOpen || NoticeRemaining>0 ||
			Inventory.Has(TEXT("Rope")) || Inventory.Has(TEXT("Gold")))
		{ Fail(TEXT("opening the door should leave both supplies behind without a reminder")); return; }
		if (CommitCheckpoint(3,P,Pawn.GetActorRotation()) || Checkpoint!=2)
		{ Fail(TEXT("CP3 accepted without supplies")); return; }
		++TestStep;
		break;
	case 31:
		Interact();
		if (bDoorOpen || !FMath::IsNearlyEqual(DoorRemaining,DamagedDoorBudget))
		{ Fail(TEXT("reclosing damaged door changed its endurance")); return; }
		UE_LOG(LogTemp,Display,TEXT("COTTAGE SAFEGUARDS: both supplies left for pickup, CP3 rejected, door reclosed with %.2f/%.2fs remaining"),DoorRemaining,DoorEnduranceSeconds);
		TestStep=10;
		break;
	case 26:
		if(!Move(FVector(2575,0,0))) break;
		Interact();
		if (!bDoorOpen || NoticeRemaining>0 || !Inventory.Has(TEXT("Rope")) || Inventory.Has(TEXT("Gold")))
		{ Fail(TEXT("opening the door should preserve the single pickup without a reminder")); return; }
		Interact();
		TestStep=12;
		break;
	case 32:
		if(!Move(FVector(2575,0,0))) break;
		Interact();
		if(!bDoorOpen || !Inventory.bBurning) { Fail(TEXT("opening with burning brand")); return; }
		++TestStep;
		break;
	case 33:
		if(FireRemaining>FireDeterrenceSeconds-3.f) break;
		SpentFireBudget=FireRemaining;
		Interact();
		if(bDoorOpen) { Fail(TEXT("closing door to return to hearth")); return; }
		++TestStep;
		break;
	case 34:
		if(auto* PC=Cast<ANight_of_the_WolvesPlayerController>(Pawn.GetController()))
		{
			if(Inventory.Equipped!=TEXT("Rope")) { PC->RequestTestEquip(TEXT("Rope")); break; }
			if(PC->bTestSelecting) break;
			if(Inventory.bBurning || !Inventory.Has(TEXT("Firebrand"))) { Fail(TEXT("failed to extinguish held firebrand")); return; }
			++TestStep;
		}
		break;
	case 35:
		if(Move(FVector(2950,130,0)))
		{
			const TArray<FName> Before=Inventory.Items;
			Interact();
			if (!Inventory.bBurning || Inventory.Equipped!=TEXT("Firebrand") || Inventory.Items!=Before ||
				!FMath::IsNearlyEqual(FireRemaining,SpentFireBudget))
			{ Fail(TEXT("relighting at hearth must reuse the brand and its remaining deterrence")); return; }
			UE_LOG(LogTemp,Display,TEXT("COTTAGE SAFEGUARDS: extinguished via wheel, relit at hearth without duplicating brand; %.2f/%.2fs deterrence retained"),FireRemaining,FireDeterrenceSeconds);
			RetryCheckpoint();
			if(Checkpoint!=2 || Inventory.Items.Num()!=1 || !Inventory.Has(TEXT("Shovel")) || !Inventory.Equipped.IsNone() || !bPitDug || !bHermitTrapped || bDoorOpen || DoorRemaining!=DoorEnduranceSeconds)
			{ Fail(TEXT("CP2 retry"));return; }
			TestStep=13;
		}
		break;
	case 13:
		if(bDoorBreached && bDoorOpen) TestStep=25;
		break;
	case 25:
		if(!bDoorBreached && Checkpoint==2 && bSheltered && !bDoorOpen && Inventory.Items.Num()==1)
		{
			UE_LOG(LogTemp,Display,TEXT("COTTAGE RECOVERY TEST PASSED: unprepared knock, CP1 retry, bare-hands knock, separate pickups, equipment, visible door breach and second secured CP2 restore"));
			FPlatformMisc::RequestExit(false);
		}
		break;
	case 14: if(Move(FVector(2575,0,0))) {Interact(); if(bDoorOpen) ++TestStep;} break;
	case 15:
		Move(FVector(2080,0,0));
		if(P.X <= Cottage->GetPitBounds().Max.X+52)
		{
			if(ACharacter* C=Cast<ACharacter>(&Pawn)) C->Jump();
			TestStep=17;
		}
		break;
	case 17:
		if(Move(FVector(2080,0,0))) {if(ACharacter* C=Cast<ACharacter>(&Pawn)) C->StopJumping(); ++TestStep;}
		break;
	case 18: if(Move(FVector(2060,-900,0))) ++TestStep; break;
	case 19: if(FollowSpline(FVector(8200,-1170,0))) ++TestStep; break;
	default: Crossing->DriveTest(*this,Pawn,Dt); break;
	}
	if(TestStep!=Previous)
		UE_LOG(LogTemp,Display,TEXT("FIRST LEG TEST step %d, CP%d, %.2fs, %s"),TestStep,Checkpoint,TestElapsed,*P.ToCompactString());
}
#endif
