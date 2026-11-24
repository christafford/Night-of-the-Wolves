#include "MaxFootsteps.h"
#include "WolfTrailGameMode.h"
#include "WolfPackComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

UMaxFootsteps::UMaxFootsteps()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	for (int32 I = 1; I <= 5; ++I)
	{
		for (bool bWood : {false, true})
		{
			const FString Path = FString::Printf(TEXT("/Game/NightOfTheWolves/Max/Footsteps/%s%d"), bWood ? TEXT("Timber") : TEXT("Trail"), I);
			ConstructorHelpers::FObjectFinder<USoundBase> Step(*Path);
			if (Step.Succeeded()) (bWood ? TimberSteps : TrailSteps).Add(Step.Object);
		}
	}
}

void UMaxFootsteps::BeginPlay()
{
	Super::BeginPlay();
	PreviousLocation = GetOwner()->GetActorLocation();
}

void UMaxFootsteps::TickComponent(float Dt, ELevelTick Type, FActorComponentTickFunction* Function)
{
	Super::TickComponent(Dt, Type, Function);
	const auto* Max = Cast<ACharacter>(GetOwner());
	const auto* Game = GetWorld()->GetAuthGameMode<AWolfTrailGameMode>();
	if (!Max || !Game) return;
	const auto* Movement = Max->GetCharacterMovement();
	const FVector Current = Max->GetActorLocation();
	const float Travel = FVector::Dist2D(Current, PreviousLocation);
	PreviousLocation = Current;
	const float Speed = Movement->Velocity.Size2D();
	const auto* PC = Cast<APlayerController>(Max->GetController());
	// A stationary jump in the drainage ditch has one audible landing even
	// though neither foot accumulates horizontal stride distance.
	const bool bGrounded=Movement->IsMovingOnGround();
	const bool bLanding=bGrounded && JumpPeak-Current.Z>52.f;
	JumpPeak=(Movement->IsFalling()) ? FMath::Max(JumpPeak,float(Current.Z)) : -BIG_NUMBER;
	if (!bGrounded || (!bLanding && (Speed < 14.f || Travel < .01f || Travel > Speed*Dt + 24.f)) ||
		Game->IsDead() || Game->IsWolfPouncing() || Game->IsTraversalLocked() || !PC || PC->GetViewTarget() != Max)
	{
		DistanceSincePlant = SincePlant = 0.f;
		LastFoot = -1;
		return;
	}

	SincePlant += Dt;
	DistanceSincePlant += Travel;
	const FVector Left = Max->GetMesh()->GetSocketLocation(TEXT("foot_l"));
	const FVector Right = Max->GetMesh()->GetSocketLocation(TEXT("foot_r"));
	const float HeightDifference = Left.Z - Right.Z;
	const int32 LowerFoot = HeightDifference > 0 ? 1 : 0;
	// As weight transfers, the swing ankle descends past the trailing ankle.
	// Distance also covers reduced/suspended skeleton updates on distant LODs.
	const bool bContact = FMath::Abs(HeightDifference) > 1.5f && LowerFoot != LastFoot && SincePlant >= .14f && DistanceSincePlant >= 8.f;
	const float Stride = FMath::GetMappedRangeValueClamped(FVector2D(45, 700), FVector2D(25, 190), Speed);
	if (!bLanding && !bContact && DistanceSincePlant < Stride * 1.22f) return;
	LastFoot = bContact ? LowerFoot : (LastFoot == 0 ? 1 : 0);
	SincePlant = DistanceSincePlant = 0.f;
	const FHitResult& Floor = Movement->CurrentFloor.HitResult;
	const bool bWood = (Floor.GetActor() && Floor.GetActor()->ActorHasTag(TEXT("M3PitCover"))) ||
		(Floor.GetComponent() && Floor.GetComponent()->ComponentHasTag(TEXT("TimberFootsteps"))) || Game->IsOnCabinFloor(Current);
	const auto& Takes = bWood ? TimberSteps : TrailSteps;
	if (Takes.IsEmpty()) return;
	const int32 Offset = Takes.Num() > 1 ? FMath::RandRange(1, Takes.Num()-1) : 1;
	LastTake = (LastTake + Offset) % Takes.Num();
	const float Force = bLanding ? .96f : FMath::GetMappedRangeValueClamped(FVector2D(60, 675), FVector2D(.43, 1.), Speed);
	const FVector Contact=bLanding ? Floor.ImpactPoint : (LastFoot==0 ? Left : Right);
	Game->GetHuntingPack()->HearFootfall(Contact, Force);
	UGameplayStatics::PlaySoundAtLocation(this, Takes[LastTake], Contact,
		Volume * Force, FMath::FRandRange(.93f, 1.06f) + .055f*Force);
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("MaxFoleyTest")))
		UE_LOG(LogTemp, Display, TEXT("MAX STEP %s %s speed %.0f %s"), LastFoot ? TEXT("R") : TEXT("L"),
			*GetNameSafe(Takes[LastTake]), Speed, bContact ? TEXT("contact") : TEXT("distance"));
#endif
}
