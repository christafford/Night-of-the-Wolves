#include "WolfTrailGameMode.h"
#include "WolfTrailHUD.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"

AWolfTrailGameMode::AWolfTrailGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FClassFinder<APawn> Character(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	static ConstructorHelpers::FClassFinder<APlayerController> Controller(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"));
	if (Character.Succeeded()) DefaultPawnClass = Character.Class;
	if (Controller.Succeeded()) PlayerControllerClass = Controller.Class;
	HUDClass = AWolfTrailHUD::StaticClass();
	static ConstructorHelpers::FObjectFinder<USoundBase> Howls(TEXT("/Game/NightOfTheWolves/Presentation/wolf_howls.wolf_howls"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Wind(TEXT("/Game/NightOfTheWolves/Presentation/forest_wind.forest_wind"));
	if (Howls.Succeeded()) WolfHowls = Howls.Object;
	if (Wind.Succeeded()) ForestWind = Wind.Object;
}

void AWolfTrailGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (WolfHowls) UGameplayStatics::SpawnSound2D(this, WolfHowls, WolfHowlVolume);
	if (ForestWind) UGameplayStatics::SpawnSound2D(this, ForestWind, ForestWindVolume);
#if !UE_BUILD_SHIPPING
	bSelfTest = FParse::Param(FCommandLine::Get(), TEXT("M3SelfTest"));
	bShovelTimeTest = FParse::Param(FCommandLine::Get(), TEXT("M3ShovelTimeTest"));
#endif
}

void AWolfTrailGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bWolfCaptured || bShotByHermit || bSheltered) return;
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn) return;
	if (!bKnockBound)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (PC->InputComponent)
			{
				PC->InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AWolfTrailGameMode::Knock);
				PC->InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AWolfTrailGameMode::UseShovel);
				bKnockBound = true;
			}
		}
	}
	if (!bPursuitInitialized)
	{
		StartPosition = Pawn->GetActorLocation();
		TrailDirection = Pawn->GetActorForwardVector().GetSafeNormal2D();
		WolfPosition = -WolfSpeed * IdleCaptureSeconds;
		bPursuitInitialized = true;
		UE_LOG(LogTemp, Display, TEXT("M1: attempt started; idle capture in %.2f seconds"), IdleCaptureSeconds);
		return;
	}
	const float Progress = FVector::DotProduct(Pawn->GetActorLocation() - StartPosition, TrailDirection);
	FurthestProgress = FMath::Max(FurthestProgress, Progress);
	bReachedTrailEnd = FurthestProgress >= TrailLength;
	const FVector Location = Pawn->GetActorLocation();
	const bool bInside = Location.X > CabinDoorLocation.X + 65.f && FMath::Abs(Location.Y - CabinDoorLocation.Y) < 350.f;
	const bool bNearDoor = FVector::Dist2D(Location, CabinDoorLocation) <= KnockDistance;
	bCanKnock = !bHermitReacting && !bHermitRunning && !bHermitTrapped && !bInside && bNearDoor;
	InteractionPrompt.Empty();
	if (bHermitTrapped && bInside && bNearDoor)
		InteractionPrompt = TEXT("Press E to close the door");
	else if (bCanKnock)
		InteractionPrompt = TEXT("Press E to knock on door");
	if (!bHasShovel && FVector::Dist2D(Location, ShovelLocation) <= ShovelDistance)
		InteractionPrompt = TEXT("Press F to take the shovel");
	else if (bHasShovel && !bPitDug && FVector::Dist2D(Location, PitLocation) <= DigDistance && Location.X < CabinDoorLocation.X)
		InteractionPrompt = TEXT("Press F to dig the pit");
	if (bHermitRunning && RunningHermit.IsValid())
	{
		HermitRunElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(HermitRunElapsed / HermitRunSeconds, 0.f, 1.f);
		const FVector From(2570.f, 0.f, 100.f);
		const FVector To(PitLocation.X, PitLocation.Y, -140.f);
		FVector Position = FMath::Lerp(From, FVector(PitLocation.X, PitLocation.Y, 100.f), Alpha);
		Position.Z = FMath::Lerp(100.f, To.Z, FMath::Clamp((Alpha - 0.65f) / 0.35f, 0.f, 1.f));
		RunningHermit->SetActorLocation(Position);
		if (Alpha >= 1.f) { bHermitRunning = false; bHermitTrapped = true; }
	}
	WolfPosition += WolfSpeed * DeltaSeconds;
	// The one-time shovel bonus expires in real game time, even while Max moves.
	ShovelBonusRemaining = FMath::Max(0.f, ShovelBonusRemaining - DeltaSeconds);
	WolfPosition = FMath::Max(WolfPosition, Progress - WolfSpeed * (IdleCaptureSeconds + ShovelBonusRemaining));
	// Use Max's current position for capture: retreating cannot preserve a previous lead.
	if (WolfPosition >= Progress) PresentWolfCapture();
#if !UE_BUILD_SHIPPING
	if (bSelfTest) TickM3SelfTest(DeltaSeconds);
	if (bShovelTimeTest) TickShovelTimeTest(DeltaSeconds);
#endif
}

#if !UE_BUILD_SHIPPING
void AWolfTrailGameMode::TickShovelTimeTest(float DeltaSeconds)
{
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn || bShotByHermit) { UE_LOG(LogTemp, Error, TEXT("SHOVEL TIME TEST FAILED: no pawn or shotgun death")); FPlatformMisc::RequestExit(false); return; }
	if (!bHasShovel)
	{
		FVector Direction = FVector(2300.f, 280.f, Pawn->GetActorLocation().Z) - Pawn->GetActorLocation();
		if (Direction.Size2D() >= 25.f) { Pawn->AddMovementInput(Direction.GetSafeNormal2D(), 1.f); return; }
		const float Progress = FVector::DotProduct(Pawn->GetActorLocation() - StartPosition, TrailDirection);
		const float LeadBefore = (Progress - WolfPosition) / WolfSpeed;
		if (ACharacter* Character = Cast<ACharacter>(Pawn)) Character->GetCharacterMovement()->StopMovementImmediately();
		UseShovel();
		const float LeadAfter = (Progress - WolfPosition) / WolfSpeed;
		UseShovel(); // Repeated input must not grant the bonus again.
		const float LeadAfterRepeat = (Progress - WolfPosition) / WolfSpeed;
		if (!bHasShovel || !FMath::IsNearlyEqual(LeadAfter - LeadBefore, ShovelBonusSeconds, 0.01f) || !FMath::IsNearlyEqual(LeadAfter, LeadAfterRepeat, 0.01f))
		{
			UE_LOG(LogTemp, Error, TEXT("SHOVEL TIME TEST FAILED: bonus or repeated pickup"));
			FPlatformMisc::RequestExit(false);
			return;
		}
		ShovelTestExpected = LeadAfter;
		UE_LOG(LogTemp, Display, TEXT("SHOVEL TIME TEST: lead %.2f -> %.2f seconds"), LeadBefore, LeadAfter);
		return;
	}
	ShovelTestElapsed += DeltaSeconds;
	if (bWolfCaptured)
	{
		const bool bPassed = FMath::Abs(ShovelTestElapsed - ShovelTestExpected) < 0.25f;
		UE_LOG(LogTemp, Display, TEXT("SHOVEL TIME TEST %s: capture after %.2f seconds, expected %.2f"), bPassed ? TEXT("PASSED") : TEXT("FAILED"), ShovelTestElapsed, ShovelTestExpected);
		FPlatformMisc::RequestExit(false);
	}
}

void AWolfTrailGameMode::TickM3SelfTest(float DeltaSeconds)
{
	SelfTestElapsed += DeltaSeconds;
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn || bWolfCaptured || bShotByHermit || SelfTestElapsed > 25.f)
	{
		UE_LOG(LogTemp, Error, TEXT("M3 SELFTEST FAILED step=%d elapsed=%.2f wolf=%d shot=%d"), SelfTestStep, SelfTestElapsed, bWolfCaptured, bShotByHermit);
		FPlatformMisc::RequestExit(false);
		return;
	}
	auto MoveTo = [Pawn](const FVector& Target) {
		FVector Direction = Target - Pawn->GetActorLocation();
		Direction.Z = 0.f;
		if (Direction.Size2D() < 25.f) return true;
		Pawn->AddMovementInput(Direction.GetSafeNormal(), 1.f);
		return false;
	};
	switch (SelfTestStep)
	{
	case 0: if (MoveTo(FVector(2300.f, 280.f, 0.f))) { UseShovel(); if (bHasShovel) ++SelfTestStep; } break;
	case 1: if (MoveTo(FVector(2260.f, 180.f, 0.f))) { UseShovel(); if (bPitDug) ++SelfTestStep; } break;
	case 2: if (MoveTo(FVector(2320.f, 145.f, 0.f)) && bCanKnock) { Knock(); if (bHermitReacting) ++SelfTestStep; } break;
	case 3: if (MoveTo(FVector(2375.f, 155.f, 0.f))) ++SelfTestStep; break;
	case 4: if (MoveTo(FVector(2375.f, 0.f, 0.f))) ++SelfTestStep; break;
	case 5: if (MoveTo(FVector(2530.f, 0.f, 0.f))) ++SelfTestStep; break;
	case 6:
		if (bHermitTrapped)
		{
			Knock();
			if (bSheltered)
			{
				UE_LOG(LogTemp, Display, TEXT("M3 SELFTEST PASSED: physical route, shovel, pit, hermit and shelter in %.2f seconds"), SelfTestElapsed);
				FPlatformMisc::RequestExit(false);
			}
		}
		break;
	}
}
#endif

void AWolfTrailGameMode::PresentWolfCapture()
{
	if (bWolfCaptured || bShotByHermit) return;
	bWolfCaptured = true;
	WolfCaptureTime = GetWorld()->GetTimeSeconds();
	bCanKnock = false;
	GetWorldTimerManager().ClearTimer(HermitTimer);
	UE_LOG(LogTemp, Display, TEXT("M1: wolf capture; presenting placeholder death"));
	FreezePlayer();
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AWolfTrailGameMode::RestartAttempt, DeathDisplaySeconds, false);
}

void AWolfTrailGameMode::Knock()
{
	if (bHermitTrapped && !bWolfCaptured && !bShotByHermit)
	{
		const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (Pawn && Pawn->GetActorLocation().X > CabinDoorLocation.X + 65.f &&
			FMath::Abs(Pawn->GetActorLocation().Y - CabinDoorLocation.Y) < 350.f &&
			FVector::Dist2D(Pawn->GetActorLocation(), CabinDoorLocation) <= KnockDistance)
		{
			TArray<AActor*> Doors;
			UGameplayStatics::GetAllActorsWithTag(this, TEXT("M2Door"), Doors);
			for (AActor* Door : Doors) { Door->SetActorRotation(FRotator::ZeroRotator); Door->SetActorLocation(FVector(2450.f, 0.f, 125.f)); }
			bSheltered = true;
			InteractionPrompt.Empty();
			UE_LOG(LogTemp, Display, TEXT("M3: cabin shelter reached"));
		}
		return;
	}
	if (!bCanKnock || bWolfCaptured || bShotByHermit || bHermitReacting) return;
	bCanKnock = false;
	bHermitReacting = true;
	TArray<AActor*> Doors;
	UGameplayStatics::GetAllActorsWithTag(this, TEXT("M2Door"), Doors);
	for (AActor* Door : Doors) { Door->SetActorLocation(FVector(2450.f, -85.f, 125.f)); Door->SetActorRotation(FRotator(0.f, 90.f, 0.f)); }
	TArray<AActor*> Hermits;
	UGameplayStatics::GetAllActorsWithTag(this, TEXT("M2Hermit"), Hermits);
	for (AActor* Hermit : Hermits) Hermit->SetActorHiddenInGame(false);
	UE_LOG(LogTemp, Display, TEXT("M2: knock; hermit reacts"));
	GetWorldTimerManager().SetTimer(HermitTimer, this, &AWolfTrailGameMode::CompleteHermitReaction, HermitReactionSeconds, false);
}

void AWolfTrailGameMode::UseShovel()
{
	if (bWolfCaptured || bShotByHermit || bSheltered || bHermitReacting || bHermitRunning) return;
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn) return;
	const FVector Location = Pawn->GetActorLocation();
	if (!bHasShovel && FVector::Dist2D(Location, ShovelLocation) <= ShovelDistance)
	{
		bHasShovel = true;
		ShovelBonusRemaining = FMath::Max(0.f, ShovelBonusSeconds);
		WolfPosition -= WolfSpeed * ShovelBonusRemaining;
		TArray<AActor*> Parts;
		UGameplayStatics::GetAllActorsWithTag(this, TEXT("M3Shovel"), Parts);
		for (AActor* Part : Parts)
		{
			const FVector Offset = Part->ActorHasTag(TEXT("M3Blade")) ? FVector(25.f, 55.f, -25.f) : FVector(25.f, 55.f, 30.f);
			Part->SetActorEnableCollision(false);
			Part->AttachToActor(Pawn, FAttachmentTransformRules::KeepRelativeTransform);
			Part->SetActorRelativeLocation(Offset);
		}
		UE_LOG(LogTemp, Display, TEXT("M3: shovel equipped"));
	}
	else if (bHasShovel && !bPitDug && Location.X < CabinDoorLocation.X && FVector::Dist2D(Location, PitLocation) <= DigDistance)
	{
		bPitDug = true;
		TArray<AActor*> Covers;
		UGameplayStatics::GetAllActorsWithTag(this, TEXT("M3PitCover"), Covers);
		for (AActor* Cover : Covers) { Cover->SetActorHiddenInGame(true); Cover->SetActorEnableCollision(false); }
		UE_LOG(LogTemp, Display, TEXT("M3: pit dug"));
	}
}

void AWolfTrailGameMode::CompleteHermitReaction()
{
	if (bWolfCaptured || bShotByHermit || bSheltered) return;
	bHermitReacting = false;
	if (!bPitDug) { HermitShoots(); return; }
	TArray<AActor*> Hermits;
	UGameplayStatics::GetAllActorsWithTag(this, TEXT("M2Hermit"), Hermits);
	if (Hermits.Num() == 0) { HermitShoots(); return; }
	RunningHermit = Hermits[0];
	HermitRunElapsed = 0.f;
	bHermitRunning = true;
	UE_LOG(LogTemp, Display, TEXT("M3: hermit runs toward pit"));
}

void AWolfTrailGameMode::HermitShoots()
{
	if (bWolfCaptured || bShotByHermit) return;
	bHermitReacting = false;
	bShotByHermit = true;
	UE_LOG(LogTemp, Display, TEXT("M2: hermit shoots"));
	FreezePlayer();
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AWolfTrailGameMode::RestartAttempt, DeathDisplaySeconds, false);
}

void AWolfTrailGameMode::FreezePlayer()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		if (ACharacter* Character = Cast<ACharacter>(PC->GetPawn()))
		{
			Character->GetCharacterMovement()->StopMovementImmediately();
			Character->DisableInput(PC);
		}
	}
}

void AWolfTrailGameMode::RestartAttempt()
{
	UE_LOG(LogTemp, Display, TEXT("M1: restarting attempt"));
	// Reloading the level reconstructs the pawn, controller, camera, timers and HUD.
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}
