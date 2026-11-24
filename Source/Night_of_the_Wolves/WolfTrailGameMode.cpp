#include "WolfTrailGameMode.h"
#include "WolfPackComponent.h"
#include "MaxDigStroke.h"
#include "WolfCaptureCamera.h"
#include "WolfTrailOpening.h"
#include "WolfMeshComponent.h"
#include "WolfCottageSet.h"
#include "CottageHermit.h"
#include "DialogueArticulation.h"
#include "FirstLegCrossing.h"
#include "StormfallSet.h"
#include "HighWaterChamber.h"
#include "StormfallLayout.h"
#include "RiverDrowningShot.h"
#include "Night_of_the_WolvesCharacter.h"
#include "WolfTrailHUD.h"
#include "WolfTrailSave.h"
#include "WolfTrailRoute.h"
#include "Components/AudioComponent.h"
#include "Components/SplineComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const FVector CottageApproach(1400,-900,105);
const FVector IndoorRetry(2650,0,100);
const FVector HearthPosition(3010,215,0);
bool Near(const FVector& A,const FVector& B,float Radius)
{
	return FVector::DistSquared2D(A,B) <= FMath::Square(Radius);
}
}

AWolfTrailGameMode::AWolfTrailGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TrailScene")));
	HuntingPack = CreateDefaultSubobject<UWolfPackComponent>(TEXT("HuntingPack"));
	static ConstructorHelpers::FClassFinder<APawn> Character(TEXT("/Game/NightOfTheWolves/Max/BP_Max"));
	static ConstructorHelpers::FClassFinder<APlayerController> Controller(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"));
	if (Character.Succeeded()) DefaultPawnClass = Character.Class;
	if (Controller.Succeeded()) PlayerControllerClass = Controller.Class;
	HUDClass = AWolfTrailHUD::StaticClass();
	OpeningCameraClass = AWolfTrailOpening::StaticClass();
	WolfCardSnarl = CreateDefaultSubobject<UAudioComponent>(TEXT("WolfCardSnarl"));
	WolfCardSnarl->SetupAttachment(GetRootComponent());
	WolfCardSnarl->bAutoActivate = false;
	WolfCardSnarl->bAllowSpatialization = false;
	static ConstructorHelpers::FObjectFinder<USoundBase> CardGrowl(TEXT("/Game/NightOfTheWolves/Presentation/WolfCardSnarl"));
	WolfCardSnarl->SetSound(CardGrowl.Object);

	static ConstructorHelpers::FObjectFinder<USoundBase> Howls(TEXT("/Game/NightOfTheWolves/Presentation/wolf_howls"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Wind(TEXT("/Game/NightOfTheWolves/Presentation/forest_wind"));
	WolfHowls = Howls.Object;
	ForestWind = Wind.Object;
	static ConstructorHelpers::FObjectFinder<USoundBase> Music(TEXT("/Game/NightOfTheWolves/Music/SW_MoonlitPursuit"));
	PursuitMusic = Music.Object;
	Score = CreateDefaultSubobject<UAudioComponent>(TEXT("PursuitScore"));
	Score->SetupAttachment(GetRootComponent());
	Score->bAutoActivate = false;
	Score->bAllowSpatialization = false;
}

void AWolfTrailGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (WolfHowls) HowlLoop = UGameplayStatics::SpawnSound2D(this,WolfHowls,WolfHowlVolume);
	if (ForestWind) WindLoop = UGameplayStatics::SpawnSound2D(this,ForestWind,ForestWindVolume);
	if (PursuitMusic) UGameplayStatics::PrimeSound(PursuitMusic);
	TActorIterator<AWolfTrailRoute> Routes(GetWorld());
	if (Routes) Route = *Routes;
	TArray<AActor*> Lights;
	UGameplayStatics::GetAllActorsWithTag(this,TEXT("M5FireLight"),Lights);
	if (!Lights.IsEmpty()) FireplaceLight = Cast<APointLight>(Lights[0]);
	UGameplayStatics::GetAllActorsWithTag(this,TEXT("M5WindowLight"),Lights);
	if (!Lights.IsEmpty()) WindowLight = Cast<APointLight>(Lights[0]);
	HuntingPack->SpawnMembers();
	Cottage = GetWorld()->SpawnActor<AWolfCottageSet>();
	Hermit = GetWorld()->SpawnActor<ACottageHermit>();
	Crossing = GetWorld()->SpawnActor<AFirstLegCrossing>();
	Storm = GetWorld()->SpawnActor<AStormfallSet>(StormfallLayout::Mouth, StormfallLayout::Facing);
	SurveyChamber = GetWorld()->SpawnActor<AHighWaterChamber>(StormfallLayout::Mouth, StormfallLayout::Facing);
#if !UE_BUILD_SHIPPING
	bFirstLegTest = FParse::Param(FCommandLine::Get(),TEXT("FirstLegTest"));
	bCottageTest = FParse::Param(FCommandLine::Get(),TEXT("M3SelfTest"));
	bRecoveryTest = FParse::Param(FCommandLine::Get(),TEXT("CottageRecoveryTest"));
#endif
}

float AWolfTrailGameMode::RouteDistance(const FVector& Position) const
{
	if (!Route.IsValid()) return Position.X;
	const USplineComponent* Spline = Route->Spline;
	return Spline->GetDistanceAlongSplineAtSplineInputKey(Spline->FindInputKeyClosestToWorldLocation(Position));
}
bool AWolfTrailGameMode::IsInside(const FVector& P) const
{
	return P.X > CabinDoorLocation.X + 50 && P.X < 3230 && FMath::Abs(P.Y) < 355 && P.Z > -25 && P.Z < 300;
}
float AWolfTrailGameMode::GetOpeningTitleOpacity() const
{
	if (!bReady) return 1.f;
	return OpeningCamera ? OpeningCamera->TitleOpacity() : 0.f;
}

float AWolfTrailGameMode::GetOpeningElapsedSeconds() const
{
	return OpeningCamera ? OpeningCamera->ElapsedSeconds() : 0.f;
}

bool AWolfTrailGameMode::AllowPausing(APlayerController* PC)
{
	return !IsOpeningCinematicPlaying() && Super::AllowPausing(PC);
}

void AWolfTrailGameMode::BeginOpening(APawn& Pawn)
{
#if !UE_BUILD_SHIPPING
	if (bFirstLegTest || bCottageTest || bRecoveryTest) return;
#endif
	auto* PC = Cast<APlayerController>(Pawn.GetController());
	if (!PC || !Route.IsValid() || !Route->Spline || !OpeningCameraClass) return;

	OpeningCamera = GetWorld()->SpawnActor<AWolfTrailOpening>(OpeningCameraClass);
	if (!OpeningCamera) return;
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	PC->FlushPressedKeys();
	Pawn.ConsumeMovementInputVector();
	if (ACharacter* Max = Cast<ACharacter>(&Pawn))
	{
		Max->StopJumping();
		Max->GetCharacterMovement()->StopMovementImmediately();
	}
	OpeningCamera->Start(*Route->Spline,Pawn,*HuntingPack,StartDistance);
	PC->SetViewTarget(OpeningCamera);
}

void AWolfTrailGameMode::FinishOpening()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetViewTarget(PC->GetPawn());
		PC->FlushPressedKeys();
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
	OpeningCamera->Destroy();
	OpeningCamera = nullptr;
	StartPursuitMusic();
	UE_LOG(LogTemp,Display,TEXT("TRAIL OPENING complete: third-person controls restored, %.2fs pursuit lead"),IdleCaptureSeconds);
}

void AWolfTrailGameMode::StartPursuitMusic()
{
	if (!PursuitMusic || Checkpoint >= 6 || Score->IsPlaying()) return;
	Score->SetSound(PursuitMusic);
	Score->SetVolumeMultiplier(MusicVolume);
	Score->FadeIn(MusicFadeInSeconds, Checkpoint == 5 ? .72f : 1.f);
}

bool AWolfTrailGameMode::IsTraversalLocked() const
{
	return IsOpeningCinematicPlaying() || IsKnocking() || bHermitReacting || DigElapsed >= 0 || (Crossing && Crossing->IsTraversalLocked())
		|| (SurveyChamber && (SurveyChamber->IsShoving() || SurveyChamber->IsClimbing() || SurveyChamber->IsSwimmingOut() || SurveyChamber->IsFramingRefill()));
}
bool AWolfTrailGameMode::IsWatchingCreekPack() const
{
	return Crossing && Crossing->IsWatchingCreekPack();
}
void AWolfTrailGameMode::StartPlay()
{
	Super::StartPlay();
	// Standalone has spawned its local player here and every actor has begun play.
	// Resolve the checkpoint and set the first view target before the initial draw.
	InitializeTrail();
}

void AWolfTrailGameMode::InitializeTrail()
{
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this,0);
	if (!Pawn || !Pawn->IsPlayerControlled() || !Cottage || !Hermit || !Crossing || !Storm || !SurveyChamber) return;
	CheckpointTransform = Pawn->GetActorTransform();
	StartDistance = RouteDistance(Pawn->GetActorLocation());
	RestoreWorld(0);
	bReady = true;
	UWolfTrailSave::Resume(*this);
	if (Checkpoint == 0) BeginOpening(*Pawn);
	if (!OpeningCamera)
	{
		StartPursuitMusic();
		UE_LOG(LogTemp,Display,TEXT("FIRST LEG start: controls ready, %.1fs opening lead"),IdleCaptureSeconds);
	}
}

void AWolfTrailGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!bReady)
	{
		// Some PIE modes possess the local pawn after StartPlay.
		InitializeTrail();
		return;
	}
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this,0);
	if (!Pawn || !Pawn->IsPlayerControlled() || IsDead()) return;
	if (OpeningCamera)
	{
		if (OpeningCamera->Advance(Dt)) FinishOpening();
		return;
	}
	NoticeRemaining -= Dt;
	InteractionPrompt.Reset();
	ThreatCue.Reset();
	const float T = GetWorld()->GetTimeSeconds();
	const float Flicker = .9f + .06f*FMath::Sin(T*9) + .04f*FMath::Sin(T*15);
	if (FireplaceLight.IsValid()) FireplaceLight->PointLightComponent->SetIntensity(38.f*Flicker);
	if (WindowLight.IsValid()) WindowLight->PointLightComponent->SetIntensity(16.f*Flicker);
	if (Checkpoint >= 5)
	{
		UpdateStormfall(Dt, *Pawn);
		if (Checkpoint >= 6 && !IsDead()) SurveyChamber->Update(*this, *Pawn, Dt);
		if (NoticeRemaining > 0 && !IsDead()) ThreatCue = Notice;
		return;
	}
	UpdateCottage(Dt,*Pawn);
	if (IsDead()) return;
	UpdatePrompt(*Pawn);
	if (!Crossing->TickEncounter(*this,*Pawn,Dt))
		UpdatePressure(Dt,*Pawn);
	if (NoticeRemaining > 0 && !IsDead()) ThreatCue = Notice;
#if !UE_BUILD_SHIPPING
	if ((bFirstLegTest || bCottageTest || bRecoveryTest) && !IsDead()) DriveTest(Dt,*Pawn);
#endif
}

void AWolfTrailGameMode::UpdateCottage(float Dt,APawn& Pawn)
{
	const FVector P = Pawn.GetActorLocation();
	if (bPitDug && P.Z < Cottage->GetPitBounds().Max.Z+26 && Cottage->GetPitBounds().IsInsideXY(P))
	{
		Die(TEXT("The hollow gave way beneath you"));
		return;
	}
	if (IsKnocking()) UpdateKnock(*CastChecked<ANight_of_the_WolvesCharacter>(&Pawn), Dt);
	if (DigElapsed >= 0)
	{
		const float PreviousCycle = MaxDigStroke::Cycle(DigElapsed / DigSeconds);
		DigElapsed += Dt;
		const float Cycle = MaxDigStroke::Cycle(DigElapsed / DigSeconds);
		if (FMath::FloorToInt(Cycle-MaxDigStroke::Plant) > FMath::FloorToInt(PreviousCycle-MaxDigStroke::Plant))
			Cottage->Impact(DigStanceEnd + DigFacing.Vector()*54.f,.41f + .04f*FMath::FloorToInt(Cycle),.88f);
		const float Settle = FMath::SmoothStep(0.f,.085f,DigElapsed/DigSeconds);
		if (DigElapsed-Dt < .085f*DigSeconds)
		{
			Pawn.SetActorLocation(FMath::Lerp(DigStanceStart,DigStanceEnd,double(Settle)),true);
			Pawn.SetActorRotation(FQuat::Slerp(DigStartFacing.Quaternion(), DigFacing.Quaternion(), Settle));
		}
		if (!Near(P,Cottage->PitSurface(),235) || Inventory.Equipped != TEXT("Shovel"))
			DigElapsed = -1;
		else if (DigElapsed >= DigSeconds)
		{
			DigElapsed = -1;
			bPitDug = true;
			Cottage->SetPit(true);
			Cottage->Impact(Cottage->PitSurface(),.65f,.65f);
			Say(TEXT("The rotten cover drops into a deep hollow."));
		}
	}
	if (bHermitReacting)
	{
		HermitElapsed += Dt;
		const float DoorSwing = (HermitElapsed / HermitReactionSeconds - .42f) / .52f;
		if (DoorSwing > 0.f && !bDoorOpen) SetDoorOpen(true);
		if (bDoorOpen && HermitElapsed - Dt < HermitReactionSeconds)
			Cottage->SwingDoor(FMath::Clamp(DoorSwing,0.f,1.f));
		const float Run = FMath::Clamp((HermitElapsed-HermitReactionSeconds)/HermitRunSeconds,0.f,1.f);
		const float Planted = FMath::Max(0.f,HermitElapsed-HermitReactionSeconds-HermitRunSeconds);
		// Let him find his footing and face Max for the bark before the rotten cover gives way.
		constexpr float ShoutAt = .40f;
		const float FallAt = ShoutAt + GetDialogueDuration(EDialogueLine::HermitShout) + .18f;
		// He briefly catches at the rim, enough time to hear his plea before he slips.
		const float PleaAt = FallAt + .44f;
		const float SlipAt = PleaAt + GetDialogueDuration(EDialogueLine::HermitPit) + .24f;
		const float ImpactAt = SlipAt + 1.25f;
		const float FireAt = FMath::Max(HermitAimSeconds, FallAt);
		const float Drop = bPitDug ? .56f * FMath::SmoothStep(FallAt, PleaAt, Planted)
			+ .44f * FMath::SmoothStep(SlipAt + .12f, ImpactAt, Planted) : 0.f;
		const float Turn = FMath::SmoothStep(0.f,.68f,Planted);
		Hermit->Stage(Run,Turn,Drop);
		if (!bHermitShouted && Planted >= ShoutAt)
		{
			bHermitShouted = true;
			Hermit->ShoutAtMax();
			Say(TEXT("“I warned you!”"), GetDialogueDuration(EDialogueLine::HermitShout));
		}
		const float PreviousRun = FMath::Clamp((HermitElapsed-Dt-HermitReactionSeconds)/HermitRunSeconds,0.f,1.f);
		if (FMath::FloorToInt(Run*4.f) > FMath::FloorToInt(PreviousRun*4.f))
			Cottage->Impact(Hermit->GetActorLocation(),.20f,Run == 1.f ? .72f : .94f);
		if (!bPitDug && Planted >= FireAt)
		{
			if (!bHermitFired)
			{
				Hermit->Fire();
				bHermitFired = true;
			}
			if (Planted >= FireAt+.18f)
			{
				Die(TEXT("Hermit"));
				return;
			}
		}
		else if (bPitDug && Planted >= FallAt)
		{
			const float Previous = Planted - Dt;
			if (Previous < FallAt) Cottage->Impact(Cottage->PitSurface(), .62f, .72f);
			if (Previous < PleaAt && Planted >= PleaAt)
			{
				Hermit->CryFromPit();
				Say(TEXT("“Get me out of here!”"), GetDialogueDuration(EDialogueLine::HermitPit));
			}
			if (Previous < SlipAt && Planted >= SlipAt) Hermit->LoseFooting();
			if (Planted >= ImpactAt && !bHermitFired)
			{
				bHermitFired = true;
				Hermit->Fire();
				Say(TEXT("His shotgun goes off in the fall. The hermit lies still."), 3.f);
				UE_LOG(LogTemp, Display, TEXT("COTTAGE HERMIT accidental shot in the hollow"));
			}
			if (Planted >= ImpactAt + .52f)
			{
				Hermit->RestInPit();
				Hermit->EndReveal(true);
				bHermitReacting = false;
				bHermitTrapped = true;
			}
		}
	}
	Cottage->Animate(GetWorld()->GetTimeSeconds(),DigElapsed/DigSeconds,DoorRemaining/DoorEnduranceSeconds,bSheltered && !bDoorOpen);
}

void AWolfTrailGameMode::UpdatePressure(float Dt,APawn& Pawn)
{
	const FVector P = Pawn.GetActorLocation();
	const float Distance = RouteDistance(P);
	if (Checkpoint == 0)
	{
		OpeningSeconds += Dt;
		WolfDistance = FMath::Max(WolfDistance + WolfSpeed * Dt, Distance - WolfSpeed*IdleCaptureSeconds);
		const float Seconds = (Distance-WolfDistance)/WolfSpeed;
		Cottage->TrailPack(Route.Get(),FMath::Clamp(Distance-WolfDistance,0.f,700.f));
		ThreatCue = Seconds < 1.4f ? TEXT("Claws scrape the trail behind you.") : Seconds < 3.f ? TEXT("The brush snaps. Closer.") : TEXT("");
		if (Seconds <= 0) { PresentWolfCapture(); return; }
		if (Distance >= RouteDistance(CottageApproach) - 360 && (Near(P,CottageApproach,780) || Near(P,CabinDoorLocation,850)))
		{
			if (CommitCheckpoint(1,CottageApproach,FRotator(0,32,0)))
			{
				Say(TEXT("The pack scrambles around the fallen timber."),3);
				Cottage->Impact(CottageApproach-FVector(450,0,0),.6f,.8f);
			}
		}
		return;
	}
	// The pack takes up the threshold when Max gets inside. Jumping the hollow
	// changes his height, so judge departures by the door plane and yard distance.
	const bool bBurning = Inventory.bBurning && Inventory.Equipped == TEXT("Firebrand");
	if (bHermitTrapped && IsInside(P)) bDoorwayGuarded = true;
	if (bDoorwayGuarded && bDoorOpen && P.X <= CabinDoorLocation.X)
	{
		if (!bBurning)
		{
			Cottage->DoorPack(CabinDoorLocation,false,true,bDoorBreached);
			UE_LOG(LogTemp,Display,TEXT("Doorway intercepted at %s, equipped %s, flame out"),*P.ToCompactString(),*Inventory.Equipped.ToString());
			PresentWolfCapture();
			return;
		}
		// The retreat clears the takeoff, hollow and landing. Farther down the
		// trail Max can put away the brand and spend his remaining lead.
		if (FVector::DistSquared2D(P,CabinDoorLocation) > FMath::Square(385.f))
			bDoorwayGuarded = false;
	}
	if (!bSheltered)
	{
		if (!bHermitReacting && !IsKnocking()) ExteriorRemaining -= Dt;
		if (bDoorwayGuarded) Cottage->DoorPack(CabinDoorLocation,false);
		else Cottage->YardPack(Route.Get(),ExteriorRemaining/ExteriorSeconds,false);
		if (ExteriorRemaining < 6) ThreatCue = TEXT("A wolf breaks cover and runs at you.");
		else if (ExteriorRemaining < 16) ThreatCue = TEXT("Growls sound just beyond the yard.");
		else if (Near(P,FVector(2390,220,0),330) && !bHermitReacting && !IsKnocking() && !bHermitTrapped)
			{
			Hermit->WarnAtWindow();
			ThreatCue = TEXT("Behind the glass: a gun barrel. “Stay away from my door.”");
		}
		if (bHermitReacting) ThreatCue = HermitElapsed < HermitReactionSeconds ? TEXT("Heavy boots behind the door.") : TEXT("");
		if (ExteriorRemaining <= 0 && !IsKnocking() && !bHermitReacting) PresentWolfCapture();
		return;
	}
	if (!bDoorOpen)
	{
		DoorRemaining -= Dt;
		DoorImpactIn -= Dt;
		const float Health = DoorRemaining / DoorEnduranceSeconds;
		ThreatCue = Health < .20f ? TEXT("The bar splits. The door buckles in its frame.") :
			Health < .50f ? TEXT("A heavy body slams against the door.") :
			Health < .84f ? TEXT("Claws scratch the other side of the door.") : TEXT("Fire crackles. The howling outside sounds distant.");
		if (DoorImpactIn <= 0 && Health < .84f)
		{
			Cottage->Impact(CabinDoorLocation,.4f + .55f*(1-Health),.8f+Health*.3f);
			DoorImpactIn = FMath::Lerp(.7f,4.f,Health);
		}
		Cottage->DoorPack(CabinDoorLocation,false);
		if (DoorRemaining <= 0)
		{
			bDoorBreached = true;
			SetDoorOpen(true);
			FleeRemaining = FMath::Min(FleeRemaining,2.f);
			Say(TEXT("The broken bar hits the floor. Wolves pour inside."));
		}
		return;
	}
	const float Extension = FMath::Max(0.f,Distance-BestDepartureDistance);
	BestDepartureDistance = FMath::Max(Distance,BestDepartureDistance);
	if (bBurning)
		FireRemaining -= Dt;
	else
		FleeRemaining = FMath::Min(14.f,FleeRemaining - Dt + Extension/430.f);
	if (bDoorBreached && IsInside(P)) FleeRemaining = FMath::Min(FleeRemaining, 2.f) - (bBurning ? Dt : 0.f);
	const float Seconds = bBurning ? FireRemaining : FleeRemaining;
	if (bDoorwayGuarded) Cottage->DoorPack(CabinDoorLocation,bBurning,false,bDoorBreached);
	else Cottage->YardPack(Route.Get(),FMath::Clamp(Seconds/(bBurning ? FireDeterrenceSeconds : 14.f),0.f,1.f),bBurning);
	if (bBurning)
		ThreatCue = Seconds < 5 ? TEXT("The lead wolf lunges toward the embers.") :
			Seconds < 14 ? TEXT("A wolf feints at the burning end.") : TEXT("The wolves recoil from the flame.");
	else if (bDoorwayGuarded && !bDoorBreached)
		ThreatCue = TEXT("The pack is waiting at the open door.");
	else if (Seconds < 7)
		ThreatCue = TEXT("Footfalls close in behind you.");
	if (Seconds <= 0 || (bDoorBreached && IsInside(P) && FleeRemaining<=0)) PresentWolfCapture();
}

AWolfTrailGameMode::ETarget AWolfTrailGameMode::Target(const APawn& Pawn,bool bItem) const
{
	const FVector P = Pawn.GetActorLocation();
	if (IsKnocking() || bHermitReacting || DigElapsed >= 0 || IsDead()) return ETarget::None;
	if (bItem && bSheltered && IsInside(P) && Near(P,HearthPosition,158) && Inventory.Has(TEXT("Firebrand")) && !Inventory.bBurning) return ETarget::Brand;
	if (bItem)
	{
		const FVector ToPatch = (Cottage->PitSurface()-P).GetSafeNormal2D();
		const bool bFacing = FVector::DotProduct(Pawn.GetActorForwardVector(),ToPatch) > -.15f;
		return !bPitDug && P.X < CabinDoorLocation.X && Near(P,Cottage->PitSurface(),215) && bFacing ? ETarget::Earth : ETarget::None;
	}
	if (!Inventory.Has(TEXT("Shovel")) && Near(P,ShovelLocation,150) && !IsInside(P)) return ETarget::Shovel;
	if (IsInside(P) && bSheltered)
	{
		if (Checkpoint < 3 && !Inventory.Has(TEXT("Gold")) && Near(P,Cottage->GetGoldLocation(),142)) return ETarget::Gold;
		if (Checkpoint < 3 && !Inventory.Has(TEXT("Rope")) && Near(P,Cottage->GetRopeLocation(),142)) return ETarget::Rope;
		if ((!Inventory.Has(TEXT("Firebrand")) || !Inventory.bBurning) && Near(P,HearthPosition,158)) return ETarget::Brand;
	}
	// A knock can be made from solid ground beside either jamb after digging.
	if (!bHermitTrapped && P.X < CabinDoorLocation.X-15 && Near(P,CabinDoorLocation,245))
		return Inventory.Equipped.IsNone() ? ETarget::Knock : ETarget::HandsFull;
	if (bHermitTrapped && IsInside(P) && Near(P,CabinDoorLocation,200))
	{
		if (!bSheltered) return ETarget::Bar;
		if (!bDoorBreached) return bDoorOpen ? ETarget::Close : ETarget::Open;
	}
	return ETarget::None;
}

void AWolfTrailGameMode::UpdatePrompt(const APawn& Pawn)
{
	if (DigElapsed >= 0) { InteractionPrompt = TEXT("Clearing the rotten planks…"); return; }
	TArray<FString,TInlineAllocator<2>> Hints;
	switch (Target(Pawn,false))
	{
	case ETarget::Shovel: Hints.Add(TEXT("E  Take shovel")); break;
	case ETarget::Knock: Hints.Add(TEXT("E  Knock")); break;
	case ETarget::HandsFull: Hints.Add(TEXT("Your hands are full.")); break;
	case ETarget::Bar: Hints.Add(TEXT("E  Close and bar door")); break;
	case ETarget::Open: Hints.Add(TEXT("E  Lift bar and open")); break;
	case ETarget::Close: Hints.Add(TEXT("E  Close and bar door")); break;
	case ETarget::Rope: Hints.Add(TEXT("E  Take rope from the wall")); break;
	case ETarget::Gold: Hints.Add(TEXT("E  Take sack of gold from the table")); break;
	case ETarget::Brand: Hints.Add(Inventory.Has(TEXT("Firebrand")) ? TEXT("E  Relight and equip firebrand") : TEXT("E  Take and equip firebrand")); break;
	default: break;
	}
	if (Target(Pawn,true) == ETarget::Earth)
		Hints.Add(Inventory.Equipped==TEXT("Shovel") ? TEXT("F  Dig loose earth") : TEXT("Loose earth shows between the rotten planks."));
	InteractionPrompt = FString::Join(Hints,TEXT("     "));
	if (Checkpoint==0 && OpeningSeconds < 3) InteractionPrompt = TEXT("W A S D  Move      Shift  Sprint");
}

void AWolfTrailGameMode::Interact() { Act(false); }
void AWolfTrailGameMode::UseEquipped() { Act(true); }
void AWolfTrailGameMode::Act(bool bItem)
{
	if (IsOpeningCinematicPlaying() || IsDead() || GetWorld()->IsPaused()) return;
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this,0);
	if (!Pawn) return;
	if (Checkpoint >= 5)
	{
		if (Checkpoint < 6 || !SurveyChamber->Act(*this,*Pawn,bItem)) StormAction(*Pawn,bItem);
		return;
	}
	if (Crossing->TryAction(*this,*Pawn,bItem)) return;
	switch (Target(*Pawn,bItem))
	{
	case ETarget::Shovel:
		Inventory.Add(TEXT("Shovel"));
		Cottage->SetShovel(false);
		Say(TEXT("Shovel taken."));
		break;
	case ETarget::Earth:
		if (Inventory.Equipped != TEXT("Shovel")) break;
		{
			DigStanceStart = Pawn->GetActorLocation();
			// Take a short step up to the closest lip when the action was begun at full reach.
			const FVector Edge = Cottage->GetPitBounds().GetClosestPointTo(DigStanceStart);
			const FVector In = (Edge-DigStanceStart).GetSafeNormal2D();
			DigStanceEnd = DigStanceStart + In*FMath::Max(0.,FVector::Dist2D(DigStanceStart,Edge)-35.);
			DigStartFacing = Pawn->GetActorRotation();
			const FVector ToPit = (Cottage->PitSurface()-DigStanceEnd).GetSafeNormal2D();
			DigFacing = ToPit.IsNearlyZero() ? DigStartFacing : ToPit.Rotation();
			Pawn->ConsumeMovementInputVector();
			if (auto* Max = Cast<ACharacter>(Pawn))
			{
				Max->StopJumping();
				Max->GetCharacterMovement()->StopMovementImmediately();
			}
			DigElapsed = 0;
		}
		break;
	case ETarget::Knock:
		if (auto* Max = Cast<ANight_of_the_WolvesCharacter>(Pawn)) BeginKnock(*Max);
		break;
	case ETarget::Bar:
		bSheltered = true;
		SetDoorOpen(false);
		DoorRemaining = DoorEnduranceSeconds;
		DoorImpactIn = 0;
		FleeRemaining = 14;
		FireRemaining = FireDeterrenceSeconds;
		BestDepartureDistance = RouteDistance(Pawn->GetActorLocation());
		CommitCheckpoint(2,IndoorRetry,FRotator(0,0,0));
		Say(TEXT("The bar drops into its brackets."));
		break;
	case ETarget::Open:
		NoticeRemaining = 0; // Let the guard or flame reaction replace an old pickup message.
		SetDoorOpen(true);
		break;
	case ETarget::Close: SetDoorOpen(false); break;
	case ETarget::Rope:
		Inventory.Add(TEXT("Rope"));
		Cottage->SetRope(false);
		Say(TEXT("Rope taken."));
		break;
	case ETarget::Gold:
		Inventory.Add(TEXT("Gold"));
		Cottage->SetGold(false);
		Say(TEXT("Sack of gold taken."));
		break;
	case ETarget::Brand:
		Inventory.Ignite(); Cottage->SetBrand(false);
		Say(TEXT("The dry end fits your hand. The far end burns.")); break;
	default:
		if (bItem && !Inventory.Equipped.IsNone() && !IsTraversalLocked()) Say(TEXT("There is nowhere to use that here."),1.4f);
		break;
	}
}

void AWolfTrailGameMode::SetDoorOpen(bool bOpen)
{
	bDoorOpen = bOpen;
	Cottage->Door(bOpen,bSheltered && !bOpen);
	if (HowlLoop) HowlLoop->SetVolumeMultiplier(WolfHowlVolume * (bSheltered && !bOpen ? .22f : 1.f));
	if (WindLoop) WindLoop->SetVolumeMultiplier(ForestWindVolume * (bSheltered && !bOpen ? .23f : 1.f));
}
void AWolfTrailGameMode::Say(const FString& Text,float Seconds)
{
	Notice = Text;
	NoticeRemaining = Seconds;
}
void AWolfTrailGameMode::PresentWolfCapture() { Die(TEXT("Wolves")); }
void AWolfTrailGameMode::Die(const FString& Cause)
{
	if (IsOpeningCinematicPlaying() || IsDead()) return;
	if (Cause == TEXT("Wolves"))
	{
		const APawn* Max=UGameplayStatics::GetPlayerPawn(this,0);
		if (!Max || !HuntingPack->ClaimCapture(*Max)) return;
	}
	DeathCause = Cause;
	EndKnock();
	if (Crossing) Crossing->InterruptEncounter();
	if (Hermit) Hermit->StopSpeech();
	bWolfCaptured = Cause == TEXT("Wolves");
	bShotByHermit = Cause == TEXT("Hermit");
	InteractionPrompt.Reset(); ThreatCue.Reset();
	UE_LOG(LogTemp,Display,TEXT("FIRST LEG death [%s] at CP%d"),*Cause,Checkpoint);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true);
		if (ACharacter* Character = PC->GetCharacter())
		{
			Character->GetCharacterMovement()->StopMovementImmediately();
			Character->GetCharacterMovement()->DisableMovement();
			if (Cause == TEXT("Current"))
			{
				RiverShot = GetWorld()->SpawnActor<ARiverDrowningShot>();
				if (RiverShot)
				{
					bRiverDrowning = true;
					RiverShot->Start(*this,*Character);
					return;
				}
			}
			if (bWolfCaptured)
			{
				UWolfMeshComponent* Lead = HuntingPack->GetAttacker();
				if (Lead)
				{
					CaptureCamera = GetWorld()->SpawnActor<AWolfCaptureCamera>();
					if (CaptureCamera)
					{
						bWolfPouncing = true;
						CaptureCamera->Start(*this,*Character,*Lead);
						return;
					}
				}
			}
		}
	}
	ShowDeathCard();
}
void AWolfTrailGameMode::ShowDeathCard()
{
	bRiverDrowning = false;
	bWolfPouncing = false;
	if (Score->IsPlaying()) Score->AdjustVolume(.45f,.45f);
	WolfCaptureTime = GetWorld()->GetTimeSeconds();
	if (bWolfCaptured)
	{
		WolfCardSnarl->Play();
		UE_LOG(LogTemp,Display,TEXT("WOLF POUNCE death card at CP%d; card snarl %s"),Checkpoint,*GetNameSafe(WolfCardSnarl->GetSound()));
	}
	GetWorldTimerManager().SetTimer(RetryTimer,this,&AWolfTrailGameMode::RetryCheckpoint,DeathDisplaySeconds,false);
}
bool AWolfTrailGameMode::CommitCheckpoint(int32 Id,FVector Location,FRotator Facing)
{
	if (IsOpeningCinematicPlaying() || Id!=Checkpoint+1 || Id>7 || IsDead() || !GetWorld()->GetFirstPlayerController()) return false;
	if (Id==2 && (!bHermitTrapped || !bSheltered)) return false;
	if (Id==3 && (!Inventory.Has(TEXT("Gold")) || !Inventory.Has(TEXT("Rope")))) return false;
	if (Id==4 && (!Inventory.Has(TEXT("Gold")) || Inventory.Has(TEXT("Rope")))) return false;
	if (Id==5 && (!Crossing || !Crossing->HasSurvivedRobbery() || Inventory.Has(TEXT("Gold")))) return false;
	if (Id==6 && (!bThroughRoots || !AreCaveRootsClear())) return false;
	if (Id==7 && (!SurveyChamber->IsComplete() || !HasTreeFlame())) return false;
	Checkpoint=Id;
	CheckpointTransform=FTransform(Facing,Location);
	CheckpointItems=Inventory;
	if (Id==5) CheckpointPouchSite=Crossing->GetGoldLanding();
	UWolfTrailSave::Write(*this);
	if (Id == 5 && Score->IsPlaying()) Score->AdjustVolume(2.4f,.72f);
	if (Id==6 && Score->IsPlaying()) Score->FadeOut(2.f,0.f);
	UE_LOG(LogTemp,Display,TEXT("FIRST LEG CP%d stored at %s, items=%d"),Id,*Location.ToCompactString(),Inventory.Items.Num());
	return true;
}

void AWolfTrailGameMode::RestoreWorld(int32 Id)
{
	WolfCardSnarl->Stop();
	Inventory = CheckpointItems;
	bWolfPouncing = false;
	DeathCause.Reset(); InteractionPrompt.Reset(); ThreatCue.Reset(); NoticeRemaining=0;
	bWolfCaptured=false; bShotByHermit=false; bPitDug=Id>=2; bHermitTrapped=Id>=2; bSheltered=Id>=2;
	bRiverDrowning=false; bHermitReacting=false; HermitElapsed=-1; DigElapsed=-1; bDoorBreached=false;
	WolfDistance=StartDistance-WolfSpeed*IdleCaptureSeconds; OpeningSeconds=0; ExteriorRemaining=ExteriorSeconds;
	DoorRemaining=DoorEnduranceSeconds; FireRemaining=FireDeterrenceSeconds; FleeRemaining=14; DoorImpactIn=0;
	BestDepartureDistance=RouteDistance(CheckpointTransform.GetLocation());
	bDoorwayGuarded=Id==2;
	HuntingPack->ResetAttempt();
	if (Id==0 && Route.IsValid())
	{
		const float Distance=FMath::Max(0.f,WolfDistance-178.f);
		HuntingPack->PlaceAtStart(Route->Spline->GetLocationAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World),
			Route->Spline->GetDirectionAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World));
	}
	else if (Id==1) HuntingPack->PlaceAtStart(CottageApproach-FVector(590,95,105),FVector::ForwardVector);
	else if (Id==2) HuntingPack->PlaceAtStart(CabinDoorLocation-FVector(239,0,100),FVector::ForwardVector);
	else if (Id==3) HuntingPack->PlaceAtStart(FVector(8460,-1190,50),FVector::ForwardVector);
	else if (Id==5) HuntingPack->PlaceAtStart(FVector(16390,700,82),FVector(1,.25,0));
	else if (Id>=6) HuntingPack->PlaceAtStart(FVector(15450,570,55),FVector(1,.25,0));
	Cottage->RestoreProps(bSheltered);
	Hermit->RestoreEncounter(bHermitTrapped,CabinDoorLocation,Cottage->PitSurface(),Cottage->GetPitBounds().GetExtent().Y);
	Cottage->SetShovel(!Inventory.Has(TEXT("Shovel"))); Cottage->SetPit(bPitDug);
	Cottage->SetGold(Id<3 && !Inventory.Has(TEXT("Gold")));
	Cottage->SetRope(Id<3 && !Inventory.Has(TEXT("Rope")));
	Cottage->SetBrand(!Inventory.Has(TEXT("Firebrand")));
	SetDoorOpen(Id>=3);
	Crossing->Restore(Id,CheckpointPouchSite);
	ResetStormfall(Id);
	SurveyChamber->RestoreSurvey(Id);
}
void AWolfTrailGameMode::RetryCheckpoint()
{
	APlayerController* PC=GetWorld()->GetFirstPlayerController();
	if (!PC || IsOpeningCinematicPlaying()) return;
	GetWorldTimerManager().ClearTimer(RetryTimer);
	EndKnock();
	Hermit->EndReveal(false);
	// Recreate the pawn so jump state, velocity and spring-arm lag start cleanly.
	if (APawn* Old=PC->GetPawn()) { PC->UnPossess(); Old->Destroy(); }
	PC->ResetIgnoreMoveInput(); PC->ResetIgnoreLookInput();
	RestartPlayerAtTransform(PC,CheckpointTransform);
	PC->SetControlRotation(CheckpointTransform.Rotator());
	if (RiverShot)
	{
		PC->SetViewTarget(PC->GetPawn());
		RiverShot->Destroy();
		RiverShot = nullptr;
	}
	if (CaptureCamera)
	{
		PC->SetViewTarget(PC->GetPawn());
		CaptureCamera->Destroy();
		CaptureCamera = nullptr;
	}
	RestoreWorld(Checkpoint);
	if (Checkpoint<6 && Score->IsPlaying()) Score->AdjustVolume(.8f,Checkpoint == 5 ? .72f : 1.f);
	UE_LOG(LogTemp,Display,TEXT("FIRST LEG restored CP%d"),Checkpoint);
}
