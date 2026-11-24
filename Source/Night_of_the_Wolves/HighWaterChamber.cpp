#include "HighWaterChamber.h"
#include "HighWaterLayout.h"
#include "Night_of_the_WolvesCharacter.h"
#include "WolfTrailGameMode.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "UObject/ConstructorHelpers.h"


namespace
{
const FName ChamberBrand(TEXT("Firebrand"));
FVector ChamberFeet(const APawn& Pawn)
{
	return StormfallLayout::LocalPoint(Pawn.GetActorLocation() - FVector::UpVector * Pawn.GetSimpleCollisionHalfHeight());
}
}

AHighWaterChamber::AHighWaterChamber()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Survey chamber")));
	RefillCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Float reveal"));
	RefillCamera->SetupAttachment(RootComponent);
	RefillCamera->SetRelativeLocation({585,2335,445});
	RefillCamera->FieldOfView = 65.f;
	static ConstructorHelpers::FObjectFinder<USoundWave> Splash(TEXT("/Game/NightOfTheWolves/HighWater/PoolSplash"));
	PoolSplash = Splash.Object;
	BuildScenery();
}

void AHighWaterChamber::RestoreSurvey(int32 Checkpoint)
{
	using namespace HighWaterLayout;
	EndRefillView(0.f);
	RestoreCleftCamera();
	EndTraversal();
	const bool bFinished = Checkpoint >= 7;
	TorchMount = EHighWaterTorch::Carried;
	bDiverted = false;
	bUnpinned = bGuided = bWallLit = bAtLanding = bTookRisenTorch = bFinished;
	Water = 1.f;
	TroughAngle = 0.f;
	TimberRoll = bFinished ? 1.f : 0.f;
	FloatBed = FloatDestination = bFinished ? Guides : LooseFloat;
	FloatLocal = FloatBed;
	if (bFinished) FloatLocal.Z = HighWater + 12.f;
	UpdateScenery(0.f);
}

AHighWaterChamber::ETarget AHighWaterChamber::FindTarget(const FVector& Feet) const
{
	using namespace HighWaterLayout;
	ETarget Result = ETarget::None;
	double Best = 1.;
	auto Candidate = [&](ETarget Target, const FVector& Position, double Radius, double MaxHeight)
	{
		if (FMath::Abs(Feet.Z - Position.Z) > MaxHeight) return;
		const double Distance = FVector::DistSquared2D(Feet, Position) / FMath::Square(Radius);
		if (Distance < Best) { Result = Target; Best = Distance; }
	};
	Candidate(ETarget::Wall, LowerFork, 133., 61.);
	Candidate(ETarget::Trough, Trough, 135., 61.);
	if (Water < .11f)
	{
		if (!bUnpinned) Candidate(ETarget::Timber, Pin, 145., 70.);
		else if (!bGuided && TimberRoll >= .95f) Candidate(ETarget::Float, FloatLocal + FVector(0,-77,0), 153., 78.);
		if (bGuided) Candidate(ETarget::LadderFork, UpperForkLow, 125., 74.);
	}
	if (Water > .96f && bGuided && Feet.X >= Guides.X - 116. && FMath::Abs(Feet.Y - Guides.Y) < 90.)
		Candidate(ETarget::Ladder, LowerClimb, 143., 43.);
	if (bAtLanding && bGuided) Candidate(ETarget::Landing, Landing, 140., 62.);
	return Result;
}

void AHighWaterChamber::ShowPrompt(AWolfTrailGameMode& Mode, ETarget Target) const
{
	const bool bHands = Mode.Inventory.Equipped.IsNone();
	switch (Target)
	{
	case ETarget::Wall:
	case ETarget::LadderFork:
	{
		const auto Socket = Target == ETarget::Wall ? EHighWaterTorch::Wall : EHighWaterTorch::Float;
		if (TorchMount == Socket) Mode.InteractionPrompt = TEXT("E  Take firebrand");
		else if (Mode.HasTreeFlame()) Mode.InteractionPrompt = TEXT("F  Hang firebrand");
		else Mode.InteractionPrompt = TEXT("Soot blackens the fork.");
		break;
	}
	case ETarget::Trough:
		Mode.InteractionPrompt = bHands && bWallLit ? TEXT("E  Swing trough") : TEXT("Wet cedar trembles under the stream.");
		break;
	case ETarget::Timber:
		Mode.InteractionPrompt = bHands ? TEXT("E  Roll timber") : TEXT("The timber bears down on one corner.");
		break;
	case ETarget::Float:
		Mode.InteractionPrompt = bHands ? TEXT("E  Shove float") : TEXT("The logs shift underfoot.");
		break;
	case ETarget::Ladder:
		Mode.InteractionPrompt = bHands ? TEXT("E  Climb ladder") : TEXT("The ladder sways.");
		break;
	case ETarget::Landing:
		if (TorchMount == EHighWaterTorch::Float) Mode.InteractionPrompt = TEXT("E  Take firebrand");
		else if (bHands) Mode.InteractionPrompt = TEXT("E  Climb down");
		break;
	default: break;
	}
}

bool AHighWaterChamber::Act(AWolfTrailGameMode& Mode, APawn& Pawn, bool bItem)
{
	using namespace HighWaterLayout;
	const FVector Feet = ChamberFeet(Pawn);
	if (Mode.Checkpoint < 6 || Feet.Y < Entrance.Y || Feet.X < 80. || Feet.X > 1860.) return false;
	if (IsClimbing() || IsSwimmingOut() || IsShoving()) return true;
	const ETarget Target = FindTarget(Feet);
	const bool bHands = Mode.Inventory.Equipped.IsNone();
	if (Target == ETarget::Wall || Target == ETarget::LadderFork)
	{
		const auto Socket = Target == ETarget::Wall ? EHighWaterTorch::Wall : EHighWaterTorch::Float;
		if (bItem && Mode.HasTreeFlame() && !HasMountedTorch() && Mode.Inventory.Consume(ChamberBrand))
		{
			TorchMount = Socket;
			if (Target == ETarget::Wall) bWallLit = true;
		}
		else if (!bItem && TorchMount == Socket && bHands)
		{
			TorchMount = EHighWaterTorch::Carried;
			Mode.Inventory.Ignite();
		}
	}
	else if (!bItem && bHands)
	{
		switch (Target)
		{
		case ETarget::Trough:
			if (!bWallLit) { Mode.Say(TEXT("My fingers slip on wet wood.")); break; }
			bDiverted = !bDiverted;
			if (!bDiverted && bUnpinned && !bGuided) FloatDestination = WrongFloat;
			if (!bDiverted && bGuided && IsTorchOnFloat() && Water < .25f) FrameRefill(Pawn);
			else EndRefillView(.3f);
			break;
		case ETarget::Timber:
			bUnpinned = true;
			Mode.Say(TEXT("The trapped corner springs loose."));
			break;
		case ETarget::Float:
			if (auto* Max = Cast<ANight_of_the_WolvesCharacter>(&Pawn)) StartShove(*Max);
			break;
		case ETarget::Ladder:
			if (auto* Max = Cast<ANight_of_the_WolvesCharacter>(&Pawn)) StartClimb(*Max, true);
			break;
		case ETarget::Landing:
			if (IsTorchOnFloat())
			{
				TorchMount = EHighWaterTorch::Carried;
				Mode.Inventory.Ignite();
				bTookRisenTorch = true;
			}
			else if (auto* Max = Cast<ANight_of_the_WolvesCharacter>(&Pawn)) StartClimb(*Max, false);
			break;
		default: break;
		}
	}
	return true;
}

void AHighWaterChamber::FrameRefill(APawn& Pawn)
{
	auto* PC = Cast<APlayerController>(Pawn.GetController());
	if (!PC || PC->GetViewTarget() != &Pawn || PreviousView.IsValid()) return;
	PreviousView = &Pawn;
	RefillController = PC;
	PC->SetIgnoreMoveInput(true);
	Pawn.ConsumeMovementInputVector();
	if (auto* Max = Cast<ACharacter>(&Pawn))
	{
		Max->StopJumping();
		Max->GetCharacterMovement()->StopMovementImmediately();
	}
	const FVector Aim = FloatLocal + FVector(86,0,159);
	RefillCamera->SetRelativeRotation((Aim - RefillCamera->GetRelativeLocation()).Rotation());
	PC->SetViewTargetWithBlend(this, .62f, VTBlend_Cubic, 0.f, true);
}

void AHighWaterChamber::EndRefillView(float Blend)
{
	if (auto* PC = RefillController.Get())
	{
		PC->SetIgnoreMoveInput(false);
		AActor* Return = PreviousView.IsValid() ? PreviousView.Get() : PC->GetPawn();
		if (Return) PC->SetViewTargetWithBlend(Return, Blend, VTBlend_Cubic, 0.f, true);
	}
	PreviousView.Reset();
	RefillController.Reset();
}

void AHighWaterChamber::Update(AWolfTrailGameMode& Mode, APawn& Pawn, float Dt)
{
	using namespace HighWaterLayout;
	if (Mode.Checkpoint < 6) return;
	const FVector Feet = ChamberFeet(Pawn);
	FrameCleft(Pawn, Feet, Dt);
	const float PreviousWater = Water;
	Water = FMath::FInterpConstantTo(Water, bDiverted ? 0.f : 1.f, Dt, 1.f / FMath::Max(1.f, WaterTravelSeconds));
	TroughAngle = FMath::FInterpConstantTo(TroughAngle, bDiverted ? 1.f : 0.f, Dt, 1.2f);
	TimberRoll = FMath::FInterpConstantTo(TimberRoll, bUnpinned ? 1.f : 0.f, Dt, .91f);
	if (bUnpinned && !bGuided)
	{
		const bool bPushing = FloatDestination.Equals(Guides, 1.);
		// A free float coasts with the current after a premature refill. On the
		// dry bed Max can shove it from either resting place into the guide posts.
		if (bPushing || Water > .15f)
		{
			const float Coast = FMath::Clamp(FVector::Dist2D(FloatBed, Guides) / 195., .21, 1.);
			FloatBed = FMath::VInterpConstantTo(FloatBed, FloatDestination, Dt, bPushing ? 218.f * Coast : 171.f);
		}
		if (bPushing && FloatBed.Equals(Guides, 1.)) bGuided = true;
	}
	if (IsShoving())
	{
		if (auto* Max = Cast<ANight_of_the_WolvesCharacter>(&Pawn)) UpdateShove(*Max, Dt);
	}
	FloatLocal = FloatBed;
	if (bUnpinned) FloatLocal.Z = FMath::Max(FloatBed.Z, double(FMath::Lerp(LowWater, HighWater, Water) + 12.f));
	UpdateScenery(Dt);
	if (IsShoving()) return;
	if (IsFramingRefill())
	{
		const FVector Aim = FloatLocal + FVector(86,0,159);
		RefillCamera->SetRelativeRotation((Aim - RefillCamera->GetRelativeLocation()).Rotation());
		if (Water >= 1.f || bDiverted) EndRefillView(.65f);
		else return;
	}
	if (IsClimbing())
	{
		if (auto* Max = Cast<ANight_of_the_WolvesCharacter>(&Pawn)) UpdateClimb(*Max, Dt);
		return;
	}
	const bool bOverPool = Feet.X > 564. && Feet.X < 1650. && Feet.Y > 1847. && Feet.Y < 3330.;
	const float Surface = FMath::Lerp(LowWater, HighWater, Water);
	const bool bWadingTooDeep = bOverPool && Water > .26f && Feet.Z < Surface - 52.f;
	if (IsSwimmingOut() || bWadingTooDeep)
	{
		if (auto* Max = Cast<ANight_of_the_WolvesCharacter>(&Pawn))
		{
			if (!IsSwimmingOut())
			{
				UGameplayStatics::PlaySoundAtLocation(this, PoolSplash, HighWaterLayout::World({Feet.X, Feet.Y, Surface}));
				Mode.Say(TEXT("My boots lose the bottom."), 1.6f);
			}
			SwimToShelf(*Max, !Mode.Inventory.Equipped.IsNone(), Dt);
			return;
		}
	}
	if (bAtLanding && Feet.Z < LandingZ - 112.)
		bAtLanding = bTookRisenTorch = false;
	if (IsComplete() && Mode.Checkpoint == 6 && Mode.HasTreeFlame() && FVector::Dist(Feet, Exit) < 105.)
		Mode.CommitCheckpoint(7, HighWaterLayout::World(Exit) + FVector::UpVector * (Pawn.GetSimpleCollisionHalfHeight() + 3.),
			(HighWaterLayout::World(Exit) - HighWaterLayout::World(Landing)).GetSafeNormal2D().Rotation());
	if (PreviousWater < .97f && Water >= .97f && bUnpinned && !bGuided)
		Mode.Say(TEXT("Timber bumps against the far wall."));
	ShowPrompt(Mode, FindTarget(Feet));
}

void AHighWaterChamber::FrameCleft(APawn& Pawn, const FVector& Feet, float Dt)
{
	// The regular trail boom is longer than the width of the arched approach.
	// Ease in before the turn and let the arm open back out across the pool.
	const float NearRamp = FMath::SmoothStep(734., 1005., Feet.Y);
	const float OnShelf = 1.f - FMath::SmoothStep(1845., 2210., Feet.Y);
	const float Inside = 1.f - FMath::SmoothStep(275., 417., FMath::Abs(Feet.X - 535.));
	const float UpperGallery = FMath::SmoothStep(2760., 3075., Feet.Y) * FMath::SmoothStep(418., 491., Feet.Z)
		* (1.f - FMath::SmoothStep(184., 260., FMath::Abs(Feet.X - 1515.)))
		* (1.f - FMath::SmoothStep(3675., 3850., Feet.Y));
	const float Desired = FMath::Max(NearRamp * OnShelf * Inside, UpperGallery);
	if (!CleftBoom.IsValid())
	{
		if (Desired <= 0.f) return;
		auto* Max = Cast<ANight_of_the_WolvesCharacter>(&Pawn);
		if (!Max) return;
		CleftBoom = Max->GetCameraBoom();
		NormalArmLength = CleftBoom->TargetArmLength;
		NormalTargetOffset = CleftBoom->TargetOffset;
		NormalSocketOffset = CleftBoom->SocketOffset;
		ShoulderWeight = Desired; // A cold CP7 spawn already starts under the low vault.
	}
	ShoulderWeight = FMath::FInterpTo(ShoulderWeight, Desired, Dt, 8.f);
	if (Desired <= 0.f && ShoulderWeight < .002f) { RestoreCleftCamera(); return; }
	CleftBoom->TargetArmLength = FMath::Lerp(NormalArmLength, FMath::Min(NormalArmLength, 154.f), ShoulderWeight);
	// Offset the trace origin too. If the spring arm retracts completely at
	// the rock soffit, the eye remains beside Max's shoulder, outside his coat.
	const FVector Right = FRotationMatrix(FRotator(0,Pawn.GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y);
	CleftBoom->TargetOffset = NormalTargetOffset + (FVector(0,0,48.) + Right*48.) * ShoulderWeight;
	CleftBoom->SocketOffset = NormalSocketOffset + FVector(0,2.,3.) * ShoulderWeight;
}

void AHighWaterChamber::RestoreCleftCamera()
{
	if (CleftBoom.IsValid())
	{
		CleftBoom->TargetArmLength = NormalArmLength;
		CleftBoom->TargetOffset = NormalTargetOffset;
		CleftBoom->SocketOffset = NormalSocketOffset;
	}
	CleftBoom.Reset();
	ShoulderWeight = 0.f;
}
