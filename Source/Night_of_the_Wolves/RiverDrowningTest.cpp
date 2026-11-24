#if !UE_BUILD_SHIPPING
#include "RiverDrowningShot.h"
#include "FirstLegCrossingLayout.h"
#include "WolfTrailGameMode.h"
#include "Components/PoseableMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
void TestRiver(UWorld* World)
{
	if (!World || World->WorldType!=EWorldType::Game) return;
	FTimerHandle Timer;
	World->GetTimerManager().SetTimer(Timer,[World,Previous=TWeakObjectPtr<ACharacter>(),Elapsed=0.f,ShotAt=0.f,
		Attempt=0,SawCard=false,WristMin=BIG_NUMBER,WristMax=-BIG_NUMBER,HeadHigh=-BIG_NUMBER,HeadLow=BIG_NUMBER,
		SawView=false,InitialY=0.,Drift=0.,CardAt=0.f]() mutable
	{
		const float Time=World->GetTimeSeconds();
		auto* Game=World->GetAuthGameMode<AWolfTrailGameMode>();
		auto* PC=World->GetFirstPlayerController();
		auto* Max=PC?PC->GetCharacter():nullptr;
		if (!Game || !Max) return;
		auto Fail=[&](const TCHAR* Why){
			UE_LOG(LogTemp,Error,TEXT("RIVER TEST FAILED: %s (CP%d attempt %d at %s)"),Why,Game->Checkpoint,Attempt,*Max->GetActorLocation().ToString());
			FPlatformMisc::RequestExitWithStatus(false,1);
		};
		if (Elapsed==0.f) Elapsed=Time;
		if (Time>Elapsed+55 || Game->Checkpoint!=3) {Fail(TEXT("missing isolated near-bank checkpoint or timeout"));return;}
		if (Game->IsDead())
		{
			if (Game->DeathCause!=TEXT("Current")) {Fail(TEXT("incorrect cause"));return;}
			if (Game->IsRiverDrowning())
			{
				if (ShotAt==0.f)
				{
					Previous=Max; ShotAt=Time;
					InitialY=Max->GetActorLocation().Y;
				}
				if (Time>ShotAt+.3f)
				{
					SawView|=PC->GetViewTarget()->IsA<ARiverDrowningShot>();
					const auto* Body=Max->FindComponentByClass<UPoseableMeshComponent>();
					if (!Body || !PC->IsMoveInputIgnored()) {Fail(TEXT("missing struggle pose or input lock"));return;}
					const float Arm=float((Body->GetSocketLocation(TEXT("hand_l"))-Body->GetSocketLocation(TEXT("upperarm_l"))).Z);
					const float Head=float(Body->GetSocketLocation(TEXT("head")).Z);
					WristMin=FMath::Min(WristMin,Arm); WristMax=FMath::Max(WristMax,Arm);
					HeadHigh=FMath::Max(HeadHigh,Head); HeadLow=FMath::Min(HeadLow,Head);
					Drift=InitialY-Max->GetActorLocation().Y;
					if (Attempt==1 && Time-ShotAt>.6f)
					{
						Game->RetryCheckpoint();
						++Attempt;
					}
				}
			}
			else if (!SawCard)
			{
				if (!SawView || Time-ShotAt<3.f || WristMax-WristMin<36 || HeadHigh-HeadLow<64 || Drift<145)
				{Fail(TEXT("card before visible reach, drift and immersion"));return;}
				SawCard=true; CardAt=Time;
				UE_LOG(LogTemp,Display,TEXT("RIVER CHECK: card after %.2fs, wrist range %.0fcm, head descent %.0fcm, downstream %.0fcm"),
					Time-ShotAt,WristMax-WristMin,HeadHigh-HeadLow,Drift);
			}
			return;
		}
		if (Previous.IsValid() && Previous.Get()==Max) {Fail(TEXT("victim not replaced"));return;}
		if (ShotAt>0)
		{
			if (!Game->Inventory.Has(TEXT("Rope")) || !Game->Inventory.Has(TEXT("Gold")) || PC->GetViewTarget()!=Max || PC->IsMoveInputIgnored() || Game->IsRiverDrowning())
			{Fail(TEXT("retry failed to restore the near bank"));return;}
			if (Attempt==0 && (!SawCard || Time-CardAt<Game->DeathDisplaySeconds-.1f)) {Fail(TEXT("death card skipped"));return;}
			if (Attempt==2)
			{
				UE_LOG(LogTemp,Display,TEXT("RIVER TEST PASSED: walked off broken bridge twice, timed drowning/card/retry, interrupted attempt restored camera and equipment"));
				FPlatformMisc::RequestExit(false);
				return;
			}
			Attempt=1; ShotAt=0.f;
		}
		// Ordinary forward movement off the broken end. The rope remains in the bag.
		const FVector Goal=FVector(10500,-1240,0);
		Max->AddMovementInput((Goal-Max->GetActorLocation()).GetSafeNormal2D());
	},.012f,FTimerManagerTimerParameters{.bLoop=true,.bMaxOncePerFrame=true});
}
FAutoConsoleCommandWithWorld RiverTest(TEXT("Wolf.TestRiver"),TEXT("Walk into the current from an isolated CP3 save"),FConsoleCommandWithWorldDelegate::CreateStatic(&TestRiver));
}
#endif
