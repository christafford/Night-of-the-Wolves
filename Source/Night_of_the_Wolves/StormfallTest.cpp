#if !UE_BUILD_SHIPPING
#include "StormfallLayout.h"
#include "Night_of_the_WolvesPlayerController.h"
#include "WolfTrailGameMode.h"
#include "WolfPackComponent.h"
#include "WolfMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "InputKeyEventArgs.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
enum class EStormCheck { Approach, Strike, Shelter, FirstVisit, ProbeRoots, Return, Equip, Light, Stow, Reequip, Relight, LitVisit, Burning, Passage };

struct FFireRetreatCheck
{
	TWeakObjectPtr<UWolfMeshComponent> Wolf[UWolfPackComponent::Count];
	FVector Start[UWolfPackComponent::Count]{};
	FVector Previous[UWolfPackComponent::Count]{};
	FVector Cover[UWolfPackComponent::Count]{};

	void Begin(UWolfPackComponent& Pack)
	{
		for (int32 I=0; I<Pack.Count; ++I)
		{
			Wolf[I]=Pack.GetMember(I);
			Start[I]=Previous[I]=Wolf[I]->GetComponentLocation();
		}
	}

	bool Continuous(UWolfPackComponent& Pack,float Delta)
	{
		for (int32 I=0; I<Pack.Count; ++I)
		{
			if (!Wolf[I].IsValid() || Wolf[I].Get()!=Pack.GetMember(I)) return false;
			const FVector Here=Wolf[I]->GetComponentLocation();
			if (FVector::Dist2D(Previous[I],Here)>FMath::Max(80.f,Delta*1350.f)) return false;
			Previous[I]=Here;
		}
		return true;
	}

	bool InCover(bool bRecord)
	{
		using StormfallLayout::Relight;
		bool bEveryWolfInCover = true;
		for (int32 I=0; I<UWolfPackComponent::Count; ++I)
		{
			const FVector Here=Previous[I];
			if (bRecord)
			{
				const double Gained=FVector::Dist2D(Here,Relight)-FVector::Dist2D(Start[I],Relight);
				UE_LOG(LogTemp,Display,TEXT("STORMFALL RETREAT wolf %d gained %.0f cm from fire, start %s cover %s"),I,Gained,*Start[I].ToCompactString(),*Here.ToCompactString());
				bEveryWolfInCover &= Gained>=220.;
				Cover[I]=Here;
			}
			else bEveryWolfInCover &= FVector::Dist2D(Cover[I],Here)<=150.;
		}
		return bEveryWolfInCover;
	}
};

void CheckStormfall(UWorld* World)
{
	if (!World || World->WorldType!=EWorldType::Game) return;
	using namespace StormfallLayout;
	const bool bResume=FParse::Param(FCommandLine::Get(),TEXT("StormResume"));
	const bool bPhotos=FParse::Param(FCommandLine::Get(),TEXT("StormPhotos"));
	FTimerHandle Timer;
	World->GetTimerManager().SetTimer(Timer,[World,bResume,bPhotos,State=EStormCheck::Approach,Start=World->GetTimeSeconds(),
		Since=0.f,Waypoint=0,Held=FKey(),Photographs=TSet<FString>(),Retreat=FFireRetreatCheck(),bRetried=false]() mutable
	{
		auto* Game=World->GetAuthGameMode<AWolfTrailGameMode>();
		auto* PC=Cast<ANight_of_the_WolvesPlayerController>(World->GetFirstPlayerController());
		auto* Max=PC ? PC->GetCharacter() : nullptr;
		if (!Max || !Game) return;
		const float Now=World->GetTimeSeconds();
		const float Age=Now-Since;
		auto* Movement=Max->GetCharacterMovement();
		const FVector Feet=Movement->GetActorFeetLocation(), Local=LocalPoint(Feet);
		auto Photo=[&](const TCHAR* Name)
		{
			if (!bPhotos || Photographs.Contains(Name) || FScreenshotRequest::IsScreenshotRequested()) return;
			Photographs.Add(Name);
			const FString Dir=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("Saved/StormfallReview"));
			IFileManager::Get().MakeDirectory(*Dir,true);
			FScreenshotRequest::RequestScreenshot(Dir/(FString(Name)+TEXT(".png")),true,false);
		};
		auto Change=[&](EStormCheck Next)
		{
			State=Next; Since=Now; Waypoint=0;
		};
		auto Tap=[&](const FKey& Key)
		{
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Pressed,1.f));
			Held=Key;
		};
		if (Held.IsValid())
		{
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(Held,IE_Released,0.f));
			Held=FKey();
		}
		auto Fail=[&](const TCHAR* Why)
		{
			UE_LOG(LogTemp,Error,TEXT("STORMFALL TEST FAILED: %s; phase %d wp %d CP%d %.2fs, feet %s local %s prompt %s"),
				Why,int32(State),Waypoint,Game->Checkpoint,Now,*Feet.ToCompactString(),*Local.ToCompactString(),*Game->InteractionPrompt);
			FPlatformMisc::RequestExitWithStatus(false,1);
		};
		auto Blocked=[&](FVector A,FVector B)
		{
			return World->LineTraceTestByChannel(WorldPoint(A),WorldPoint(B),ECC_Visibility,
				FCollisionQueryParams(SCENE_QUERY_STAT(StormfallTest),false,Max));
		};
		auto RootsBlocked=[&] {return Blocked({496,142,104},{496,307,104});};
		auto LogBlocked=[&] {return Blocked({-430,-101,98},{-430,-360,98});};
		auto Look=[&](const FVector& At,float Pitch)
		{
			PC->SetControlRotation(FRotator(Pitch,(At-Feet).Rotation().Yaw,0));
		};
		auto Move=[&](const FVector& At)
		{
			if (FVector::DistSquared2D(Feet,At)<FMath::Square(11.))
			{
				Movement->StopMovementImmediately();
				return true;
			}
			Max->AddMovementInput((At-Feet).GetSafeNormal2D());
			Look(At,-4.f);
			return false;
		};
		if (bResume)
		{
			if (Game->Checkpoint==6 && Game->HasStormStruck() && Game->AreCaveRootsClear() &&
				!RootsBlocked() && LogBlocked() && Game->HasTreeFlame() && Movement->IsMovingOnGround() && FVector::Dist2D(Feet,Outlet)<150.)
			{
				UE_LOG(LogTemp,Display,TEXT("STORMFALL RESUME PASSED: cold CP6, grounded in inner chamber, lit brand, charred roots open and trunk still blocking trail"));
				FPlatformMisc::RequestExit(false);
			}
			else if (Now-Start>3.) Fail(TEXT("cold CP6 restore"));
			return;
		}
		if (Now-Start>110.f || Game->IsDead()) {Fail(TEXT("timeout or death")); return;}
		switch (State)
		{
		case EStormCheck::Approach:
			if (Game->Checkpoint!=5) {Fail(TEXT("requires CP5")); return;}
			Move(StrikeTrigger+FVector(52,12,0));
			Look(TreeFoot,16.f);
			if (Game->HasStormStruck())
			{
				if (LogBlocked() || !RootsBlocked() || Game->HasTreeFlame()) {Fail(TEXT("invalid initial strike state"));return;}
				Retreat.Begin(*Game->GetHuntingPack());
				Movement->StopMovementImmediately(); Photo(TEXT("01-Lightning")); Change(EStormCheck::Strike);
			}
			break;
		case EStormCheck::Strike:
			if (Age<.23f && Age>.18f) Photo(TEXT("02-Return-stroke"));
			if (Age>1.63f) {Look(TreeFoot+FVector(0,530,0),11.f);Photo(TEXT("03-Falling-crown"));}
			if (!Retreat.Continuous(*Game->GetHuntingPack(),World->GetDeltaSeconds())) {Fail(TEXT("wolf identity or continuous retreat movement"));return;}
			if (Age>3.15f) Photo(TEXT("03b-Wolves-break-away"));
			if (Age>4.15f) Photo(TEXT("03c-Wolves-flee"));
			if (Age>3.3f && !bRetried && FParse::Param(FCommandLine::Get(),TEXT("StormRetry")))
			{
				bRetried=true;
				Game->RetryCheckpoint();
				if (Game->HasStormStruck() || Game->IsDead() || LogBlocked() || !RootsBlocked() ||
					Game->Checkpoint!=5 || PC->GetViewTarget()!=PC->GetPawn())
				{Fail(TEXT("retry during retreat failed to reset fire, camera or physical barriers"));return;}
				UE_LOG(LogTemp,Display,TEXT("STORMFALL RETRY PASSED: retry during fire reaction restored CP5 camera and unstruck fir"));
				Change(EStormCheck::Approach);
				break;
			}
			if (Age>9.f)
			{
				if (!LogBlocked() || !RootsBlocked() || !Retreat.InCover(true)) {Fail(TEXT("fallen fir or five-wolf downhill retreat"));return;}
				Change(EStormCheck::Shelter);
			}
			break;
		case EStormCheck::Shelter:
			if (!Retreat.Continuous(*Game->GetHuntingPack(),World->GetDeltaSeconds()) || !Retreat.InCover(false))
			{Fail(TEXT("pack resumed the hunt in the fire clearing"));return;}
			if (Age>(FParse::Param(FCommandLine::Get(),TEXT("StormLinger")) ? 55.f : 2.f))
			{
				if (PC->GetViewTarget()!=Max || Game->HasTreeFlame()) {Fail(TEXT("camera or firebrand while waiting for the pack"));return;}
				UE_LOG(LogTemp,Display,TEXT("STORMFALL FIRE PASSED: all five fled, remained away; unarmed Max waited %.1fs at bend"),Age+9.f);
				Change(EStormCheck::FirstVisit);
			}
			break;
		case EStormCheck::FirstVisit:
		case EStormCheck::LitVisit:
		{
			const FVector Route[]={Approach,CaveEntry,Chamber,RootStance};
			if (Waypoint==0 && Feet.X>17720 && State==EStormCheck::FirstVisit) Photo(TEXT("04-Fire-and-cut"));
			if (Move(Route[Waypoint]) && ++Waypoint==UE_ARRAY_COUNT(Route))
			{
				Look(Through,-4.f);
				Max->SetActorRotation((Through-Feet).GetSafeNormal2D().Rotation());
				if (State==EStormCheck::LitVisit)
				{
					if (!Game->HasTreeFlame() || !RootsBlocked()) {Fail(TEXT("brand or root barrier at second visit"));return;}
					Change(EStormCheck::Burning);
				}
				else Change(EStormCheck::ProbeRoots);
			}
			break;
		}
		case EStormCheck::ProbeRoots:
			if (Game->Inventory.Has(TEXT("Firebrand")) && Game->Inventory.Equipped!=TEXT("Firebrand"))
			{
				PC->RequestTestEquip(TEXT("Firebrand")); Since=Now; break;
			}
			if (!Game->HasTreeFlame() && Age<.05f && !Held.IsValid()) Tap(EKeys::F);
			Move(Through);
			if (!RootsBlocked() || Game->AreCaveRootsClear() || Local.Y>203.) {Fail(TEXT("unlit Max passed or ignited roots"));return;}
			if (Age>1.3f)
			{
				if (Game->CommitCheckpoint(6,Outlet,OutletFacing)) {Fail(TEXT("CP6 awarded early"));return;}
				UE_LOG(LogTemp,Display,TEXT("STORMFALL CHECK: unlit item use and walking into tangled roots failed, CP6 refused"));
				Change(EStormCheck::Return);
			}
			break;
		case EStormCheck::Return:
		{
			const FVector Route[]={RootStance,Chamber,CaveEntry,Approach,Relight};
			if (Move(Route[Waypoint]) && ++Waypoint==UE_ARRAY_COUNT(Route)) Change(EStormCheck::Equip);
			break;
		}
		case EStormCheck::Equip:
		case EStormCheck::Reequip:
			if (!Game->Inventory.Has(TEXT("Firebrand"))) {Tap(EKeys::E); Change(EStormCheck::Light);}
			else if (Game->Inventory.Equipped!=TEXT("Firebrand")) PC->RequestTestEquip(TEXT("Firebrand"));
			else {Tap(EKeys::F); Change(State==EStormCheck::Equip ? EStormCheck::Light : EStormCheck::Relight);}
			break;
		case EStormCheck::Light:
		case EStormCheck::Relight:
			Look(TreeFoot+FVector(0,330,0),-7.f);
			if (Game->HasTreeFlame() && Age>.4f)
			{
				Photo(TEXT("05-Brand-at-fallen-fir"));
				Change(State==EStormCheck::Light ? EStormCheck::Stow : EStormCheck::LitVisit);
			}
			else if (Age>2.f) {Fail(TEXT("relight from the burning branch"));return;}
			break;
		case EStormCheck::Stow:
			if (Game->Inventory.Equipped!=NAME_None) PC->RequestTestEquip(NAME_None);
			else
			{
				if (Game->Inventory.bBurning || Game->HasTreeFlame()) {Fail(TEXT("flame survived stowing"));return;}
				UE_LOG(LogTemp,Display,TEXT("STORMFALL CHECK: burning tree lit brand, stowing extinguished it"));
				Change(EStormCheck::Reequip);
			}
			break;
		case EStormCheck::Burning:
			if (Age<.35f) Photo(TEXT("06-Roots-in-firelight"));
			else if (Age<.40f && !Held.IsValid()) Tap(EKeys::F);
			if (Age>1.15f && Age<1.8f)
			{
				if (!RootsBlocked()) {Fail(TEXT("burning curtain opened before fibres cleared"));return;}
				Photo(TEXT("07-Roots-catching"));
			}
			if (Game->AreCaveRootsClear())
			{
				if (RootsBlocked()) {Fail(TEXT("charred roots still obstruct cleft"));return;}
				UE_LOG(LogTemp,Display,TEXT("STORMFALL CHECK: second relight, held firebrand burned roots; physical side cleft now open"));
				Change(EStormCheck::Passage);
			}
			break;
		case EStormCheck::Passage:
			if (Waypoint==0 && Move(Through)) ++Waypoint;
			else if (Waypoint==1) Move(Outlet);
			if (Game->Checkpoint==6)
			{
				if (!Game->HasTreeFlame() || !Movement->IsMovingOnGround() || !LogBlocked())
				{Fail(TEXT("incomplete passage or lost flame"));return;}
				Photo(TEXT("08-Inner-cleft"));
				UE_LOG(LogTemp,Display,TEXT("STORMFALL TEST PASSED: storm at bend, fallen tree, dark cave and blocked roots, two relights, burn and physical passage to CP6 in %.2fs"),Now-Start);
				FPlatformMisc::RequestExit(false);
			}
			break;
		}
	},.012f,FTimerManagerTimerParameters{.bLoop=true,.bMaxOncePerFrame=true});
}
FAutoConsoleCommandWithWorld StormfallProbe(TEXT("Wolf.TestStormfall"),TEXT("Play the stormfall and root cleft from an isolated bend checkpoint"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&CheckStormfall));
}
#endif
