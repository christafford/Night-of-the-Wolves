#if !UE_BUILD_SHIPPING

#include "WolfTrailGameMode.h"
#include "WolfPackComponent.h"
#include "WolfMeshComponent.h"
#include "HuntingWolf.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

namespace
{
// The fixture isolates repeatable sightlines from the art map. It drives Max's
// location and encounter permission; the five ordinary possessed pawns sense,
// choose destinations and physically navigate using their production systems.
struct FPackObservation : TSharedFromThis<FPackObservation>
{
	TWeakObjectPtr<AWolfTrailGameMode> Mode;
	TWeakObjectPtr<AActor> Geometry;
	TWeakObjectPtr<UBoxComponent> Screen, Island;
	TWeakObjectPtr<ACharacter> Max;
	TWeakObjectPtr<UWolfPackComponent> Pack;
	TWeakObjectPtr<UWolfMeshComponent> Roster[5];
	const FVector Origin{64000.,64000.,950.};
	static constexpr float Dt = .05f;
	int32 Stage = 0;
	float Time = 0, AltTravel = 0, FlankTravel = 0, ReversalAt = -1;
	float HiddenAge = 0, MinSpacing = BIG_NUMBER;
	int32 Simultaneous = 0, Reloads = 0;
	bool bScreenUp = false, bFlank = false, bSignal = false, bTurned = false, bTracker = false;
	FVector Prior[5]{}, LastBelief;
	FVector NarrowStart[5]{};
	bool bFailed = false;

	void Require(bool Good, const TCHAR* What)
	{
		if (Good || bFailed) return;
		bFailed = true;
		UE_LOG(LogTemp,Error,TEXT("WOLF PACK TEST FAILED stage %d at %.2fs: %s"),Stage,Time,What);
		FPlatformMisc::RequestExitWithStatus(false,1);
	}

	UBoxComponent* Box(const FVector& Center, const FVector& Extent)
	{
		auto* BoxShape = NewObject<UBoxComponent>(Geometry.Get());
		BoxShape->InitBoxExtent(Extent);
		BoxShape->SetCollisionProfileName(TEXT("BlockAll"));
		BoxShape->SetCollisionObjectType(ECC_WorldStatic);
		BoxShape->SetWorldLocation(Origin+Center);
		BoxShape->RegisterComponent();
		Geometry->AddInstanceComponent(BoxShape);
		return BoxShape;
	}

	void RosterCheck()
	{
		int32 Total = 0;
		for (TActorIterator<AHuntingWolf> It(Mode->GetWorld()); It; ++It) ++Total;
		Require(Total == 5,TEXT("unexpected number of hunting pawns"));
		for (int32 I=0; I<5; ++I)
		{
			auto* Body = Pack->GetMember(I);
			auto* Wolf = Body ? Cast<AHuntingWolf>(Body->GetOwner()) : nullptr;
			Require(Wolf && Wolf->GetController() && Body->GetIdentity()==I,TEXT("unpossessed or mismatched wolf"));
			if (!Body) return;
			if (!Roster[I].IsValid()) Roster[I] = Body;
			else Require(Roster[I]==Body,TEXT("reload replaced a roster member"));
		}
	}

	void MoveMax(float X, float Y=0)
	{
		Max->SetActorLocation(Origin+FVector(X,Y,Max->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2.f));
	}
	void Place(float X, float Y=0)
	{
		Pack->ResetAttempt();
		Pack->PlaceAtStart(Origin+FVector(X,Y,0),FVector::ForwardVector);
		for (int32 I=0; I<5; ++I) Prior[I] = Pack->GetMember(I)->GetComponentLocation();
	}

	void Enter(int32 Next)
	{
		Stage = Next;
		Time = 0;
		if (Stage==0)
		{
			Place(-400.,-900.);
			for(int32 I=0; I<5; ++I) NarrowStart[I]=Prior[I];
		}
		else if (Stage==1) Place(-320.);
		else if (Stage==2)
		{
			Island->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Place(-680.);
		}
		else if (Stage==3)
		{
			Island->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Place(-620.);
		}
		else if (Stage==4) Place(-340.);
		UE_LOG(LogTemp,Display,TEXT("WOLF PACK TEST stage %d"),Stage);
	}

	void CheckRetry()
	{
		auto* PC=Mode->GetWorld()->GetFirstPlayerController();
		auto* PriorMax=Max.Get();
		Mode->RetryCheckpoint();
		Max=Cast<ACharacter>(PC->GetPawn());
		Require(Max.IsValid() && Max.Get()!=PriorMax,TEXT("retry did not restore Max"));
		RosterCheck();
		Require(Pack->GetAttacker()==nullptr,TEXT("stale capture after retry"));
		for(int32 I=0; I<5; ++I)
			Require(!Pack->GetTactics().CanAttack(I) && Pack->GetTactics().Wolves[I].Route.Points.IsEmpty(),
				TEXT("stale engagement or route after retry"));
	}
	void Tick()
	{
		if (bFailed || !Mode.IsValid() || !Pack.IsValid() || !Max.IsValid()) return;
		Time += Dt;
		float X=0,Y=0;
		switch(Stage)
		{
		case 0: X=420.f+Time*286.f; Y=-900; break;
		case 1: X=350.f+Time*175.f; Y=190.f*FMath::Sin(Time*.6f); break;
		case 2:
			X=ReversalAt < 0 ? 310.f+Time*170.f : 310.f+ReversalAt*170.f-(Time-ReversalAt)*260.f;
			break;
		case 3: X=690.f; Y=Time<.75f ? 0.f : FMath::Min(420.f,(Time-.75f)*165.f); break;
		case 4: X=0; break;
		}
		if (Stage==3 && Time>=.75f && !bScreenUp)
		{
			bScreenUp = true;
			LastBelief = Pack->GetTactics().LastKnown;
			Screen->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		if (Stage==3 && Time>=6.f) Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MoveMax(X,Y);
		const bool Lethal = Stage==4;
		const auto Terrain = Stage==0 ? EWolfGroundEncounter::Trail : EWolfGroundEncounter::Clearing;
		const FVector Feet = Origin + FVector(X,Y,0);
		Pack->Hunt(Dt, Feet - FVector(410,0,0), FVector::ForwardVector, Lethal ? 0.f : 230.f, Terrain, Lethal);
		const auto& Plan=Pack->GetTactics();
		int32 Engaging = 0;
		for (int32 I=0; I<5; ++I)
		{
			const FVector P=Pack->GetMember(I)->GetComponentLocation();
			const float Travel=FVector::Dist2D(P,Prior[I]);
			if (Stage!=0 || Time>1.f) Require(Travel < 120.f,TEXT("wolf jumped between route positions"));
			if (Plan.CanAttack(I)) ++Engaging;
			if (Stage==0 && Time>1.5f)
			{
				Require(Plan.Wolves[I].Assignment!=EWolfAssignment::Flank && Plan.Wolves[I].Assignment!=EWolfAssignment::Intercept,TEXT("committed to a specialty on the narrow trail"));
				for(int32 J=0; J<I; ++J) MinSpacing=FMath::Min(MinSpacing,float(FVector::Dist2D(P,Pack->GetMember(J)->GetComponentLocation())));
			}
			if (Stage==1 && Plan.Wolves[I].Assignment==EWolfAssignment::Flank && Plan.Wolves[Plan.PressureWolf].Assignment==EWolfAssignment::Pressure)
			{
				bFlank = true;
				FlankTravel += Travel;
			}
			if (Stage==2 && I==3 && Plan.Wolves[I].Assignment==EWolfAssignment::Intercept && Plan.Wolves[I].Route.bAlternate)
				AltTravel += Travel;
			if (Stage==3 && I==4 && Plan.Wolves[I].Assignment==EWolfAssignment::Track) bTracker=true;
			Prior[I]=P;
		}
		if (FParse::Param(FCommandLine::Get(),TEXT("PackTestTrace")) && FMath::FloorToInt(Time*2)!=FMath::FloorToInt((Time-Dt)*2))
		{
			UE_LOG(LogTemp,Display,TEXT("PACK FIXTURE %d %.2f %s seen %.2f flank %.0f scout %.0f"),Stage,Time,
				Plan.IntentionName(Plan.Intention),Plan.BeliefAge,FlankTravel,AltTravel);
			for(int32 I=0; I<5; ++I) UE_LOG(LogTemp,Display,TEXT("  %s %s pos %s dest %s: %s"),
				Plan.IdentityName(I),Plan.AssignmentName(Plan.Wolves[I].Assignment),*(Prior[I]-Origin).ToCompactString(),
				*(Plan.Wolves[I].Destination-Origin).ToCompactString(),*Plan.Wolves[I].Reason);
		}

		Simultaneous=FMath::Max(Simultaneous,Engaging);
		Require(Engaging <= Pack->Tuning.MaxEngagements && (!Engaging || Lethal),TEXT("engagement exceeded permission"));
		bSignal |= Plan.CueWolf >= 0 && Plan.CueTime > .05f;

		if (Stage==0 && Time>4.5f)
		{
			for(int32 I=0; I<5; ++I)
				Require(FVector::Dist2D(NarrowStart[I],Prior[I])>220.f,TEXT("wolf stalled on narrow approach"));
			UE_LOG(LogTemp,Display,TEXT("WOLF PACK TEST narrow physical gaps minimum %.1fcm"),MinSpacing);
			Enter(1);
		}
		else if (Stage==1 && Time>5.f)
		{
			Require(bFlank && FlankTravel>100.f && bSignal,TEXT("no concurrent physical support approach and signal"));
			UE_LOG(LogTemp,Display,TEXT("WOLF PACK TEST clearing physical flank %.0fcm; leader/deputy signal observed"),FlankTravel);
			CheckRetry(); // Reload while flank reservations are live.
			Enter(2);
		}
		else if (Stage==2)
		{
			if (AltTravel > 170.f && ReversalAt<0.f) ReversalAt=Time;
			if (ReversalAt>=0 && Time>ReversalAt+.1f && Plan.Wolves[3].Assignment!=EWolfAssignment::Intercept) bTurned=true;
			if ((bTurned && Time>ReversalAt+1.1f) || Time>10.f)
			{
				Require(AltTravel>170.f && bTurned,TEXT("scout did not physically take and leave an obsolete interception route"));
				UE_LOG(LogTemp,Display,TEXT("WOLF PACK TEST physical alternate %.0fcm; reversed at %.2fs; released intercept"),AltTravel,ReversalAt);
				Enter(3);
			}
		}
		else if (Stage==3)
		{
			if (Time>1.5f && Time<5.95f)
			{
				HiddenAge=FMath::Max(HiddenAge,Plan.BeliefAge);
				Require(FVector::Dist2D(LastBelief,Plan.LastKnown)<6.f,TEXT("pack updated the hidden quarry without sensory evidence"));
				Require(!Pack->ClaimCapture(*Max),TEXT("committed through sight barrier"));
			}
			if (Time>7.5f)
			{
				Require(HiddenAge>3.8f && bTracker && Plan.BeliefAge<.35f && Plan.Intention==EWolfIntention::Pursue,
					TEXT("lost-sight investigation or reacquisition was missing"));
				UE_LOG(LogTemp,Display,TEXT("WOLF PACK TEST hidden belief held %.2fs, tracker investigated, quarry reacquired"),HiddenAge);
				Enter(4);
			}
		}
		else if (Stage==4)
		{
			if (Engaging && Pack->ClaimCapture(*Max))
			{
				CheckRetry();
				if (++Reloads>=3 && !bFailed)
				{
					UE_LOG(LogTemp,Display,TEXT("WOLF PACK TEST PASSED: five possessed wolves, simultaneous commitments <= %d, %d physical capture/retry cycles"),Simultaneous,Reloads);
					FPlatformMisc::RequestExit(false);
					return;
				}
				Place(-340.);
				Time=0;
			}
			else Require(Time<8.f,TEXT("eligible physical capture failed to occur"));
		}
	}
};

void TestPack(UWorld* World)
{
	if (!World || World->WorldType!=EWorldType::Game) return;
	auto Test = MakeShared<FPackObservation>();
	Test->Mode=World->GetAuthGameMode<AWolfTrailGameMode>();
	auto* PC = World->GetFirstPlayerController();
	if (!Test->Mode.IsValid() || !PC) return;
	if (Test->Mode->IsOpeningCinematicPlaying())
	{
		FTimerHandle WaitForOpening;
		World->GetTimerManager().SetTimer(WaitForOpening,[World]() { TestPack(World); },.2f,false);
		return;
	}
	Test->Pack=Test->Mode->GetHuntingPack();
	Test->Max=Cast<ACharacter>(PC->GetPawn());
	if (!Test->Pack.IsValid() || !Test->Max.IsValid())
	{
		UE_LOG(LogTemp, Error,TEXT("WOLF PACK TEST FAILED: no live Max / pack"));
		FPlatformMisc::RequestExitWithStatus(false,1);
		return;
	}
	Test->Mode->SetActorTickEnabled(false);
	Test->Max->GetCharacterMovement()->DisableMovement();
	Test->Geometry=World->SpawnActor<AActor>();
	Test->Box(FVector(0,0,-35),FVector(6200,3600,35));
	Test->Box(FVector(850,-1130,115),FVector(2100,18,115));
	Test->Box(FVector(850,-670,115),FVector(2100,18,115));
	Test->Screen=Test->Box(FVector(100,0,240),FVector(30,3450,240));
	Test->Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Test->Island=Test->Box(FVector(0,0,32),FVector(88,162,32));
	Test->Island->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Test->RosterCheck();
	if (Test->bFailed) return;
	Test->Enter(0);
	FTimerHandle Timer;
	World->GetTimerManager().SetTimer(Timer, [Test]() { Test->Tick(); },FPackObservation::Dt,true);
}
// An observation camera for repeatable video of the real trail and clearing.
// The existing clean-route driver or player still moves Max and triggers the hunt.
void FilmPack(UWorld* World)
{
	if (!World || World->WorldType!=EWorldType::Game) return;
	auto* Mode=World->GetAuthGameMode<AWolfTrailGameMode>();
	auto* PC=World->GetFirstPlayerController();
	if (!Mode || !PC || !Mode->GetHuntingPack() || !Mode->GetHuntingPack()->GetMember(0)) return;
	auto* Camera=World->SpawnActor<ACameraActor>();
	Camera->GetCameraComponent()->FieldOfView=62.f;
	auto& Grade=Camera->GetCameraComponent()->PostProcessSettings;
	Grade.bOverride_AutoExposureBias=true;
	Grade.AutoExposureBias=1.15f;
	FTimerHandle Timer;
	World->GetTimerManager().SetTimer(Timer,[Mode,PC,Camera,World,Started=false]() mutable
	{
		if (!IsValid(Mode) || !IsValid(PC) || !PC->GetPawn() || !IsValid(Camera)) return;
		auto* Hunting=Mode->GetHuntingPack();
		if (!Hunting || Mode->IsOpeningCinematicPlaying() || Mode->IsDead() || Mode->IsTraversalLocked()) return;
		const bool bScout = FParse::Param(FCommandLine::Get(),TEXT("PackFollowScout"));
		auto* Focus=Hunting->GetMember(bScout ? 3 : Hunting->GetTactics().PressureWolf);
		if (!Focus || !Focus->IsVisible()) return;
		const FVector Hero = Focus->GetComponentLocation();
		FVector Nearest=Hero;
		float Distance=615.f;
		for (int32 I=0; I<5; ++I)
		{
			auto* Wolf=Hunting->GetMember(I);
			if (!Wolf || Wolf==Focus || !Wolf->IsVisible()) continue;
			const float Gap=FVector::Dist2D(Hero,Wolf->GetComponentLocation());
			if (Gap<Distance) { Distance=Gap; Nearest=Wolf->GetComponentLocation(); }
		}
		const FVector Feet=FMath::Lerp(Hero,Nearest,bScout ? .09 : .24);
		const FVector Look=Feet+FVector(0,0,58);
		const FVector Along=Focus->GetForwardVector().GetSafeNormal2D();
		const FVector Side(-Along.Y,Along.X,0);
		const float Range=FMath::Clamp(240.f+Distance*.48f,300.f,460.f);
		// Look upstream from one shoulder, keeping the faces and negative space
		// between the support wolves readable. Shorten the view at a tree trunk.
		const FVector Want=Look+Along*Range*.67 + Side*Range*.66 + FVector(0,0,Range*.47);
		FHitResult Hit;
		const FVector Eye=World->LineTraceSingleByChannel(Hit,Look,Want,ECC_Camera) ?
			Hit.ImpactPoint+Hit.ImpactNormal*14. : Want;
		const FVector At=Started ? FMath::VInterpTo(Camera->GetActorLocation(),Eye,.033f,4.5f) : Eye;
		Camera->SetActorLocationAndRotation(At,(Look-At).Rotation());
		if (PC->GetViewTarget()!=Camera) PC->SetViewTarget(Camera);
		Started=true;
	},.033f,true);
}
// This natural shoulder on the far road has a straight main track and a longer,
// supported route behind a thicket. Max walks it; tactical/terrain inputs and
// feet are observed exactly as in a live hunt. Only the test's start is staged.
void TestFarRoad(UWorld* World)
{
	if (!World || World->WorldType!=EWorldType::Game) return;
	auto* Mode=World->GetAuthGameMode<AWolfTrailGameMode>();
	auto* PC=World->GetFirstPlayerController();
	auto* Max=PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
	if (!Mode || !Max || !Mode->GetHuntingPack()) return;
	if (Mode->IsOpeningCinematicPlaying())
	{
		FTimerHandle Wait;
		World->GetTimerManager().SetTimer(Wait,[World](){ TestFarRoad(World); },.2f,false);
		return;
	}
	auto* Pack=Mode->GetHuntingPack();
	Mode->SetActorTickEnabled(false);
	const FVector Start(11818.63,-1069.27,48.);
	const FVector Heading=FVector(1.,-.20,0).GetSafeNormal();
	const FVector Side(-Heading.Y,Heading.X,0);
	const FVector Walker(12244.6,-979.1,48.);
	Max->SetActorLocationAndRotation(Walker+FVector(0,0,Max->GetSimpleCollisionHalfHeight()+2.),Heading.Rotation());
	Max->GetCharacterMovement()->StopMovementImmediately();
	Max->GetCharacterMovement()->MaxWalkSpeed=200.f;
	Pack->ResetAttempt();
	Pack->PlaceAtStart(Start+235.*Heading+63.*Side,Heading);
	if (FParse::Param(FCommandLine::Get(),TEXT("PackFollowScout"))) FilmPack(World);

	FTimerHandle Timer;
	FTimerManagerTimerParameters FrameTimer;
	FrameTimer.bLoop=true;
	FrameTimer.bMaxOncePerFrame=true;
	World->GetTimerManager().SetTimer(Timer,[World,Max,Pack,Heading,Side,Start,Time=0.f,ReverseAt=-1.f,Travel=0.f,
		Lateral=0.f,Previous=Start,EverAlternate=false,SawRelease=false]() mutable
	{
		const float Dt=World->GetDeltaSeconds();
		Time+=Dt;
		const FVector Direction=ReverseAt>=0.f ? -Heading : Heading;
		Max->AddMovementInput(Direction,1.f,true);
		const FVector Feet=Max->GetActorLocation()-FVector(0,0,Max->GetSimpleCollisionHalfHeight());
		Pack->Hunt(Dt,Feet-Heading*310.,Heading,225.f,EWolfGroundEncounter::FarBank,false);
		const auto& Decision=Pack->GetTactics().Wolves[3];
		const FVector Scout=Pack->GetMember(3)->GetComponentLocation();
		if (Decision.Assignment==EWolfAssignment::Intercept && Decision.Route.bAlternate)
		{
			if (!EverAlternate) UE_LOG(LogTemp,Display,TEXT("FAR ROAD scout committed at %s, destination %s, %.0fcm via %d points"),
				*Scout.ToCompactString(),*Decision.Destination.ToCompactString(),Decision.Route.Length,Decision.Route.Points.Num());
			EverAlternate=true;
			Travel+=FVector::Dist2D(Previous,Scout);
			Lateral=FMath::Max(Lateral,float(FMath::Abs(FVector::DotProduct(Scout-Start,Side))));
			if (Travel>340.f && Lateral>125.f && ReverseAt<0.f)
			{
				ReverseAt=Time;
				UE_LOG(LogTemp,Display,TEXT("FAR ROAD scout at %s while Max reverses at %s"),*Scout.ToCompactString(),*Feet.ToCompactString());
			}
		}
		else if (ReverseAt>=0.f) SawRelease=true;
		Previous=Scout;
		if (FParse::Param(FCommandLine::Get(),TEXT("PackTestTrace")) && FMath::FloorToInt(2*Time)!=FMath::FloorToInt(2*(Time-Dt)))
			UE_LOG(LogTemp,Display,TEXT("FAR ROAD %.1fs: %s %s on %s, Max %s, score %.0f/%.0f"),
				Time,*Scout.ToCompactString(),*Decision.Reason,*Decision.Destination.ToCompactString(),*Feet.ToCompactString(),Travel,Lateral);
		if (SawRelease && Time>ReverseAt+1.5f)
		{
			UE_LOG(LogTemp,Display,TEXT("FAR ROAD TEST PASSED: scout physically travelled %.0fcm on alternate, %.0fcm lateral separation, canceled after Max changed course"),Travel,Lateral);
			FPlatformMisc::RequestExit(false);
		}
		else if (Time>10.f)
		{
			UE_LOG(LogTemp,Error,TEXT("FAR ROAD TEST FAILED: scout committed %d; travel %.0fcm; lateral %.0fcm; released %d"),EverAlternate,Travel,Lateral,SawRelease);
			FPlatformMisc::RequestExitWithStatus(false,1);
		}
	},.001f,FrameTimer);
}
FAutoConsoleCommandWithWorld Road(TEXT("Wolf.TestRoadBranch"),TEXT("Observe an actual intercept and course reversal on the playable far-bank road"),FConsoleCommandWithWorldDelegate::CreateStatic(&TestFarRoad));
FAutoConsoleCommandWithWorld Film(TEXT("Wolf.FilmPack"),TEXT("Watch the live roster with an observation camera during ordinary pursuit"),FConsoleCommandWithWorldDelegate::CreateStatic(&FilmPack));
FAutoConsoleCommandWithWorld Probe(TEXT("Wolf.TestPack"),TEXT("Exercise five possessed wolves on controlled physical sightlines and alternate routes"),FConsoleCommandWithWorldDelegate::CreateStatic(&TestPack));
}
#endif
