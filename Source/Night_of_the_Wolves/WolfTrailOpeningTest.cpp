#if !UE_BUILD_SHIPPING

#include "WolfTrailGameMode.h"
#include "WolfPackComponent.h"
#include "WolfMeshComponent.h"
#include "UnrealClient.h"
#include "WolfTrailRoute.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Sound/SoundBase.h"
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
	TWeakObjectPtr<UWolfMeshComponent> Wolves[UWolfPackComponent::Count];
	bool bFourRunning = false;
	bool bAlphaPassed = false;
	bool bFiveInShot = false;
	bool bAlphaStartedBehind = false;
	bool bSawCard = false;
	bool bHeardCard = false;
	int32 Photo = 0;
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
		auto* Score = Cast<UAudioComponent>(Mode->GetDefaultSubobjectByName(TEXT("PursuitScore")));
		if (!Score) { Fail(TEXT("missing pursuit score component")); return; }
		auto* CardSnarl = Cast<UAudioComponent>(Mode->GetDefaultSubobjectByName(TEXT("WolfCardSnarl")));
		if (!CardSnarl || !CardSnarl->GetSound() || CardSnarl->GetSound()->IsLooping())
			{ Fail(TEXT("missing one-shot card snarl")); return; }
		auto Photo=[&](int32 Number,const TCHAR* Label)
		{
			if (Seen.Photo>=Number || !FParse::Param(FCommandLine::Get(),TEXT("OpeningPackPhotos")) || FScreenshotRequest::IsScreenshotRequested()) return;
			const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("Saved/StormReview"));
			IFileManager::Get().MakeDirectory(*Folder,true);
			FScreenshotRequest::RequestScreenshot(Folder/FString(Label),true,false);
			Seen.Photo=Number;
		};
		if (Mode->IsOpeningCinematicPlaying())
		{
			if (Score->GetSound() || Score->IsPlaying()) { Fail(TEXT("score started during the title or wolf camera")); return; }
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
			const FTransform Camera(PC->PlayerCameraManager->GetCameraRotation(),Eye);
			const double HalfWidth=FMath::Tan(FMath::DegreesToRadians(PC->PlayerCameraManager->GetFOVAngle()*.5));
			int32 Visible=0, InFrame=0;
			float AlphaAlong=0, NextWolf=-BIG_NUMBER;
			for (int32 I=0; I<UWolfPackComponent::Count; ++I)
			{
				auto* Body=Mode->GetHuntingPack()->GetMember(I);
				if (!Body || Body->GetIdentity()!=I) { Fail(TEXT("lost a pack member during opening")); return; }
				if (!Seen.Wolves[I].IsValid()) Seen.Wolves[I]=Body;
				if (Body!=Seen.Wolves[I].Get()) { Fail(TEXT("replaced an opening wolf")); return; }
				const FVector Feet=Body->GetComponentLocation();
				const float BodyAlong=Spline->GetDistanceAlongSplineAtSplineInputKey(Spline->FindInputKeyClosestToWorldLocation(Feet));
				if (I==0) AlphaAlong=BodyAlong;
				else NextWolf=FMath::Max(NextWolf,BodyAlong);
				if (!Body->IsVisible()) continue;
				++Visible;
				const FVector InView=Camera.InverseTransformPositionNoScale(Feet+FVector(0,0,43));
				if (InView.X>0 && FMath::Abs(InView.Y)<InView.X*HalfWidth*.98 && FMath::Abs(InView.Z)<InView.X*HalfWidth*.54)
					++InFrame;
			}
			if (Visible<4) { Fail(TEXT("fewer than four physical wolves on the opening trail")); return; }
			if (Alpha<.01f && Visible==4 && InFrame==4 && !Seen.Wolves[0]->IsVisible())
			{
				Seen.bFourRunning=true; Photo(1,TEXT("01-Alpha-POV.png"));
			}
			if (Visible==5 && AlphaAlong+40.f<NextWolf) Seen.bAlphaStartedBehind=true;
			if (Visible==5 && AlphaAlong>NextWolf+105.f && Seen.bAlphaStartedBehind)
			{
				Seen.bAlphaPassed=true;
				if (InFrame==5) {Seen.bFiveInShot=true; Photo(2,TEXT("02-Alpha-Takes-Point.png"));}
			}
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
		if (!Score->GetSound() || Score->GetSound()!=Mode->PursuitMusic || !Score->GetSound()->IsLooping())
			{ Fail(TEXT("handoff did not select the looping gameplay score")); return; }
		if (Seen.ControlAt < 0)
		{
			Seen.ControlAt = Now;
			if (!Seen.bFourRunning || !Seen.bAlphaPassed || !Seen.bFiveInShot)
			{
				UE_LOG(LogTemp,Error,TEXT("ALPHA COVERAGE: four from POV %d, joined behind %d, passed %d, five in shot %d"),
					Seen.bFourRunning,Seen.bAlphaStartedBehind,Seen.bAlphaPassed,Seen.bFiveInShot);
				Fail(TEXT("missing four-to-five alpha reveal")); return;
			}
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
		if (CardSnarl->IsPlaying() && (!Mode->bWolfCaptured || Mode->IsWolfPouncing()))
			{ Fail(TEXT("card snarl started before the image or survived retry")); return; }
		if (Mode->bWolfCaptured && !Mode->IsWolfPouncing())
		{
			Seen.bSawCard=true;
			if (CardSnarl->IsPlaying()) Seen.bHeardCard=true;
			if (Now-Mode->WolfCaptureTime>.32f) Photo(3,TEXT("03-Wolf-Death-Card.png"));
		}
		if (Seen.LaunchAfter >= 0 && !Mode->IsDead() && Seen.FirstMax.Get() != Max)
		{
			if (PC->GetViewTarget() != Max || PC->IsMoveInputIgnored() || PC->IsLookInputIgnored())
				{ Fail(TEXT("CP0 retry did not restore third person and input")); return; }
			if (!Seen.bSawCard || (FParse::Param(FCommandLine::Get(),TEXT("OpeningPackAudioTest")) && !Seen.bHeardCard))
				{ Fail(TEXT("missing death card or requested mixer check")); return; }
			UE_LOG(LogTemp, Display, TEXT("ALPHA REVEAL: four in POV, same silver fifth passed and led four in frame. Card snarl %s; observed playing %d"),*CardSnarl->GetSound()->GetName(),Seen.bHeardCard);
			UE_LOG(LogTemp, Display, TEXT("OPENING TEST PASSED: %.2fs sequence, title held and dissolved, wolf eye travelled %.1fm with %d height reversals, nearest %.1fm, pounce %.2fs after handoff, retry returned directly to Max"),
				Seen.LastShotTime, (Seen.FurthestRailDistance-Seen.FirstRailDistance)/100.f, Seen.VerticalTurns, Seen.ClosestShot/100.f, Seen.LaunchAfter);
			FPlatformMisc::RequestExit(false);
		}
	}, .025f, true);
}
FAutoConsoleCommandWithWorld OpeningProbe(TEXT("Wolf.TestOpening"), TEXT("Observe the opening and first idle retry from a fresh trail"), FConsoleCommandWithWorldDelegate::CreateStatic(&TestOpening));
}
#endif
