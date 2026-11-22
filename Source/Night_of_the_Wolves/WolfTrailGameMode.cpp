#include "WolfTrailGameMode.h"
#include "WolfCaptureCamera.h"
#include "WolfTrailOpening.h"
#include "WolfMeshComponent.h"
#include "WolfCottageSet.h"
#include "CottageHermit.h"
#include "FirstLegCrossing.h"
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
const FVector TablePosition(2800,-230,0);
const FVector HearthPosition(3010,215,0);
bool Near(const FVector& A,const FVector& B,float Radius)
{
	return FVector::DistSquared2D(A,B) <= FMath::Square(Radius);
}
}

AWolfTrailGameMode::AWolfTrailGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FClassFinder<APawn> Character(TEXT("/Game/NightOfTheWolves/Max/BP_Max"));
	static ConstructorHelpers::FClassFinder<APlayerController> Controller(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"));
	if (Character.Succeeded()) DefaultPawnClass = Character.Class;
	if (Controller.Succeeded()) PlayerControllerClass = Controller.Class;
	HUDClass = AWolfTrailHUD::StaticClass();
	OpeningCameraClass = AWolfTrailOpening::StaticClass();
	static ConstructorHelpers::FObjectFinder<USoundBase> Howls(TEXT("/Game/NightOfTheWolves/Presentation/wolf_howls"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Wind(TEXT("/Game/NightOfTheWolves/Presentation/forest_wind"));
	WolfHowls = Howls.Object;
	ForestWind = Wind.Object;
}

void AWolfTrailGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (WolfHowls) HowlLoop = UGameplayStatics::SpawnSound2D(this,WolfHowls,WolfHowlVolume);
	if (ForestWind) WindLoop = UGameplayStatics::SpawnSound2D(this,ForestWind,ForestWindVolume);
	TActorIterator<AWolfTrailRoute> Routes(GetWorld());
	if (Routes) Route = *Routes;
	TArray<AActor*> Lights;
	UGameplayStatics::GetAllActorsWithTag(this,TEXT("M5FireLight"),Lights);
	if (!Lights.IsEmpty()) FireplaceLight = Cast<APointLight>(Lights[0]);
	UGameplayStatics::GetAllActorsWithTag(this,TEXT("M5WindowLight"),Lights);
	if (!Lights.IsEmpty()) WindowLight = Cast<APointLight>(Lights[0]);
	Cottage = GetWorld()->SpawnActor<AWolfCottageSet>();
	Hermit = GetWorld()->SpawnActor<ACottageHermit>();
	Crossing = GetWorld()->SpawnActor<AFirstLegCrossing>();
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
	OpeningCamera->Start(*Route->Spline,Pawn,StartDistance);
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
	UE_LOG(LogTemp,Display,TEXT("TRAIL OPENING complete: third-person controls restored, %.2fs pursuit lead"),IdleCaptureSeconds);
}

bool AWolfTrailGameMode::IsTraversalLocked() const
{
	return IsOpeningCinematicPlaying() || bHermitReacting || DigElapsed >= 0 || (Crossing && Crossing->IsTraversalLocked());
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
	if (!Pawn || !Pawn->IsPlayerControlled() || !Cottage || !Hermit || !Crossing) return;
	CheckpointTransform = Pawn->GetActorTransform();
	StartDistance = RouteDistance(Pawn->GetActorLocation());
	RestoreWorld(0);
	bReady = true;
	UWolfTrailSave::Resume(*this);
	if (Checkpoint == 0) BeginOpening(*Pawn);
	if (!OpeningCamera)
		UE_LOG(LogTemp,Display,TEXT("FIRST LEG start: controls ready, %.1fs opening lead"),IdleCaptureSeconds);
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
	if (bLegComplete)
	{
		if (Pawn->GetActorLocation().Z < -320.f) RetryCheckpoint();
		return;
	}
	UpdateCottage(Dt,*Pawn);
	if (IsDead()) return;
	UpdatePrompt(*Pawn);
	if (Crossing->TickEncounter(*this,*Pawn,Dt))
		Cottage->HidePack();
	else
		UpdatePressure(Dt,*Pawn);
	if (NoticeRemaining > 0 && !IsDead()) ThreatCue = Notice;
#if !UE_BUILD_SHIPPING
	if ((bFirstLegTest || bCottageTest || bRecoveryTest) && !IsDead()) DriveTest(Dt,*Pawn);
#endif
}

void AWolfTrailGameMode::UpdateCottage(float Dt,APawn& Pawn)
{
	const FVector P = Pawn.GetActorLocation();
	if (bPitDug && P.Z < 26 && Near(P,PitLocation,92))
	{
		Die(TEXT("The hollow gave way beneath you"));
		return;
	}
	if (DigElapsed >= 0)
	{
		if (!Near(P,PitLocation,235) || Inventory.Equipped != TEXT("Shovel"))
			DigElapsed = -1;
		else if ((DigElapsed += Dt) >= DigSeconds)
		{
			DigElapsed = -1;
			bPitDug = true;
			Cottage->SetPit(true);
			Cottage->Impact(PitLocation,.65f,.65f);
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
		const float Drop = bPitDug ? FMath::Clamp(Planted/.64f,0.f,1.f) : 0.f;
		const float Turn = bPitDug ? 0.f : FMath::SmoothStep(0.f,.68f,Planted);
		Hermit->Stage(Run,Turn,Drop);
		const float PreviousRun = FMath::Clamp((HermitElapsed-Dt-HermitReactionSeconds)/HermitRunSeconds,0.f,1.f);
		if (FMath::FloorToInt(Run*4.f) > FMath::FloorToInt(PreviousRun*4.f))
			Cottage->Impact(Hermit->GetActorLocation(),.20f,Run == 1.f ? .72f : .94f);
		if (!bPitDug && Planted >= HermitAimSeconds)
		{
			if (!bHermitFired)
			{
				Hermit->Fire();
				bHermitFired = true;
			}
			if (Planted >= HermitAimSeconds+.18f)
			{
				Die(TEXT("Hermit"));
				return;
			}
		}
		else if (Drop >= 1.f)
		{
			Hermit->EndReveal(true);
			bHermitReacting = false;
			bHermitTrapped = true;
			Cottage->Impact(PitLocation,1.f,.7f);
			Cottage->CurseFromPit();
			Say(TEXT("A splash. The hermit curses from below."));
		}
	}
	Cottage->Animate(GetWorld()->GetTimeSeconds(),DigElapsed,DoorRemaining/DoorEnduranceSeconds,bSheltered && !bDoorOpen);
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
		Cottage->TrailPack(Route.Get(),WolfDistance,P);
		ThreatCue = Seconds < 1.4f ? TEXT("Claws scrape the trail behind you.") : Seconds < 3.f ? TEXT("The brush snaps. Closer.") : TEXT("");
		if (Seconds <= 0) { PresentWolfCapture(); return; }
		if (Distance >= RouteDistance(CottageApproach) - 360 && (Near(P,CottageApproach,780) || Near(P,CabinDoorLocation,850)))
		{
			if (CommitCheckpoint(1,CottageApproach,FRotator(0,32,0)))
			{
				ExteriorRemaining = ExteriorSeconds;
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
			Cottage->DoorPack(CabinDoorLocation,false);
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
		if (!bHermitReacting) ExteriorRemaining -= Dt;
		if (bDoorwayGuarded) Cottage->DoorPack(CabinDoorLocation,false);
		else Cottage->YardPack(P,ExteriorRemaining/ExteriorSeconds,false);
		if (ExteriorRemaining < 6) ThreatCue = TEXT("A wolf breaks cover and runs at you.");
		else if (ExteriorRemaining < 16) ThreatCue = TEXT("Growls sound just beyond the yard.");
		else if (Near(P,FVector(2390,220,0),330) && !bHermitReacting && !bHermitTrapped)
			{
			Cottage->WarnAtWindow();
			ThreatCue = TEXT("Behind the glass: a gun barrel. “Stay away from my door.”");
		}
		if (bHermitReacting) ThreatCue = HermitElapsed < HermitReactionSeconds ? TEXT("Heavy boots behind the door.") : TEXT("");
		if (ExteriorRemaining <= 0) PresentWolfCapture();
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
	if (bDoorwayGuarded) Cottage->DoorPack(CabinDoorLocation,bBurning);
	else Cottage->YardPack(P,FMath::Clamp(Seconds/(bBurning ? FireDeterrenceSeconds : 14.f),0.f,1.f),bBurning);
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
	if (bHermitReacting || DigElapsed >= 0 || IsDead() || bLegComplete) return ETarget::None;
	if (bItem && bSheltered && IsInside(P) && Near(P,HearthPosition,158) && Inventory.Has(TEXT("Firebrand")) && !Inventory.bBurning) return ETarget::Brand;
	if (bItem)
	{
		const FVector ToPatch = (PitLocation-P).GetSafeNormal2D();
		const bool bFacing = FVector::DotProduct(Pawn.GetActorForwardVector(),ToPatch) > -.15f;
		return !bPitDug && P.X < CabinDoorLocation.X && Inventory.Has(TEXT("Shovel")) && Near(P,PitLocation,215) && bFacing ? ETarget::Earth : ETarget::None;
	}
	if (!Inventory.Has(TEXT("Shovel")) && Near(P,ShovelLocation,150) && !IsInside(P)) return ETarget::Shovel;
	if (IsInside(P) && bSheltered)
	{
		if (!Inventory.Has(TEXT("Gold")) && !Inventory.Has(TEXT("Rope")) && Checkpoint < 4 && Near(P,TablePosition,165)) return ETarget::Supplies;
		if ((!Inventory.Has(TEXT("Firebrand")) || !Inventory.bBurning) && Near(P,HearthPosition,158)) return ETarget::Brand;
	}
	if (Near(P,CabinDoorLocation,200))
	{
		if (!bHermitTrapped && P.X < CabinDoorLocation.X-15) return ETarget::Knock;
		if (bHermitTrapped && IsInside(P))
		{
			if (!bSheltered) return ETarget::Bar;
			if (!bDoorBreached) return bDoorOpen ? ETarget::Close : ETarget::Open;
		}
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
	case ETarget::Bar: Hints.Add(TEXT("E  Close and bar door")); break;
	case ETarget::Open: Hints.Add(TEXT("E  Lift bar and open")); break;
	case ETarget::Close: Hints.Add(TEXT("E  Close and bar door")); break;
	case ETarget::Supplies: Hints.Add(TEXT("E  Take supplies")); break;
	case ETarget::Brand: Hints.Add(Inventory.Has(TEXT("Firebrand")) ? TEXT("E  Relight and equip firebrand") : TEXT("E  Take and equip firebrand")); break;
	default: break;
	}
	if (Target(Pawn,true) == ETarget::Earth)
		Hints.Add(Inventory.Equipped==TEXT("Shovel") ? TEXT("F  Dig loose earth") : TEXT("Tab  Equip shovel"));
	InteractionPrompt = FString::Join(Hints,TEXT("     "));
	if (Checkpoint==0 && OpeningSeconds < 3) InteractionPrompt = TEXT("W A S D  Move      Shift  Sprint");
}

void AWolfTrailGameMode::Interact() { Act(false); }
void AWolfTrailGameMode::UseEquipped() { Act(true); }
void AWolfTrailGameMode::Act(bool bItem)
{
	if (IsOpeningCinematicPlaying() || IsDead() || bLegComplete || GetWorld()->IsPaused()) return;
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this,0);
	if (!Pawn || Crossing->TryAction(*this,*Pawn,bItem)) return;
	switch (Target(*Pawn,bItem))
	{
	case ETarget::Shovel:
		Inventory.Add(TEXT("Shovel"));
		Cottage->SetShovel(false);
		Say(TEXT("Shovel taken. Open your pack to equip it."));
		break;
	case ETarget::Earth:
		if (Inventory.Equipped != TEXT("Shovel")) { Say(TEXT("Equip the shovel to clear the loose earth.")); break; }
		DigElapsed = 0;
		Cottage->Impact(PitLocation,.55f,1.1f);
		break;
	case ETarget::Knock:
		Cottage->ShoutAtDoor();
		Cottage->Impact(CabinDoorLocation,.75f,1.2f);
		bHermitReacting = true;
		bHermitFired = false;
		HermitElapsed = 0;
		Hermit->AnswerKnock(*Pawn);
		Say(TEXT("“I warned you!”"),HermitReactionSeconds);
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
		if (!Inventory.Has(TEXT("Rope")) && !bWarnedTable)
		{
			bWarnedTable = true;
			Say(TEXT("The coil and the coin sack are still on the table."),4);
		}
		else NoticeRemaining = 0; // Let the guard or flame reaction replace an old pickup message.
		SetDoorOpen(true);
		break;
	case ETarget::Close: SetDoorOpen(false); break;
	case ETarget::Supplies:
		Inventory.Add(TEXT("Rope")); Inventory.Add(TEXT("Gold"));
		Cottage->SetSupplies(false);
		Say(TEXT("Rope taken. Sack of gold taken.")); break;
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
	if (IsOpeningCinematicPlaying() || IsDead() || bLegComplete) return;
	DeathCause = Cause;
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
			if (bWolfCaptured)
			{
				UWolfMeshComponent* Lead = Checkpoint >= 3 && Crossing ? Crossing->GetLeadWolf() : (Cottage ? Cottage->GetLeadWolf() : nullptr);
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
	bWolfPouncing = false;
	WolfCaptureTime = GetWorld()->GetTimeSeconds();
	if (bWolfCaptured) UE_LOG(LogTemp,Display,TEXT("WOLF POUNCE death card at CP%d"),Checkpoint);
	GetWorldTimerManager().SetTimer(RetryTimer,this,&AWolfTrailGameMode::RetryCheckpoint,DeathDisplaySeconds,false);
}
bool AWolfTrailGameMode::CommitCheckpoint(int32 Id,FVector Location,FRotator Facing)
{
	if (IsOpeningCinematicPlaying() || Id!=Checkpoint+1 || Id>5 || IsDead() || !GetWorld()->GetFirstPlayerController()) return false;
	if (Id==2 && (!bHermitTrapped || !bSheltered)) return false;
	if (Id==3 && (!Inventory.Has(TEXT("Gold")) || !Inventory.Has(TEXT("Rope")))) return false;
	if (Id==4 && (!Inventory.Has(TEXT("Gold")) || Inventory.Has(TEXT("Rope")))) return false;
	Checkpoint=Id;
	CheckpointTransform=FTransform(Facing,Location);
	CheckpointItems=Inventory;
	UWolfTrailSave::Write(*this);
	bLegComplete = Id == 5;
	UE_LOG(LogTemp,Display,TEXT("FIRST LEG CP%d stored at %s, items=%d"),Id,*Location.ToCompactString(),Inventory.Items.Num());
	return true;
}

void AWolfTrailGameMode::RestoreWorld(int32 Id)
{
	Inventory = CheckpointItems;
	bWolfPouncing = false;
	DeathCause.Reset(); InteractionPrompt.Reset(); ThreatCue.Reset(); NoticeRemaining=0;
	bWolfCaptured=false; bShotByHermit=false; bPitDug=Id>=2; bHermitTrapped=Id>=2; bSheltered=Id>=2;
	bLegComplete=Id==5; bHermitReacting=false; HermitElapsed=-1; DigElapsed=-1; bWarnedTable=false; bDoorBreached=false;
	WolfDistance=StartDistance-WolfSpeed*IdleCaptureSeconds; OpeningSeconds=0; ExteriorRemaining=ExteriorSeconds;
	DoorRemaining=DoorEnduranceSeconds; FireRemaining=FireDeterrenceSeconds; FleeRemaining=14; DoorImpactIn=0;
	BestDepartureDistance=RouteDistance(CheckpointTransform.GetLocation());
	bDoorwayGuarded=Id==2;
	Cottage->RestoreProps(bSheltered);
	Hermit->RestoreEncounter(bHermitTrapped,CabinDoorLocation,PitLocation);
	Cottage->SetShovel(!Inventory.Has(TEXT("Shovel"))); Cottage->SetPit(bPitDug);
	Cottage->SetSupplies(Id<3 && !Inventory.Has(TEXT("Rope"))); Cottage->SetBrand(!Inventory.Has(TEXT("Firebrand")));
	SetDoorOpen(Id>=3);
	Crossing->Restore(Id);
}
void AWolfTrailGameMode::RetryCheckpoint()
{
	APlayerController* PC=GetWorld()->GetFirstPlayerController();
	if (!PC || IsOpeningCinematicPlaying()) return;
	GetWorldTimerManager().ClearTimer(RetryTimer);
	Hermit->EndReveal(false);
	// Recreate the pawn so jump state, velocity and spring-arm lag start cleanly.
	if (APawn* Old=PC->GetPawn()) { PC->UnPossess(); Old->Destroy(); }
	PC->ResetIgnoreMoveInput(); PC->ResetIgnoreLookInput();
	RestartPlayerAtTransform(PC,CheckpointTransform);
	PC->SetControlRotation(CheckpointTransform.Rotator());
	if (CaptureCamera)
	{
		PC->SetViewTarget(PC->GetPawn());
		CaptureCamera->Destroy();
		CaptureCamera = nullptr;
	}
	RestoreWorld(Checkpoint);
	UE_LOG(LogTemp,Display,TEXT("FIRST LEG restored CP%d"),Checkpoint);
}
