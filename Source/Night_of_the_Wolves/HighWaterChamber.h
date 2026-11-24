#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HighWaterScenery.h"
#include "HighWaterChamber.generated.h"

class ANight_of_the_WolvesCharacter;
class AWolfTrailGameMode;
class UCameraComponent;
class UPoseableMeshComponent;
class USoundWave;
class USpringArmComponent;

enum class EHighWaterTorch : uint8 { Carried, Wall, Float };

UCLASS()
class NIGHT_OF_THE_WOLVES_API AHighWaterChamber : public AActor
{
	GENERATED_BODY()
public:
	AHighWaterChamber();
	void RestoreSurvey(int32 Checkpoint);
	void Update(AWolfTrailGameMode& Mode, APawn& Pawn, float Dt);
	bool Act(AWolfTrailGameMode& Mode, APawn& Pawn, bool bItem);
	bool IsClimbing() const { return ClimbTime >= 0.f; }
	bool IsSwimmingOut() const { return SwimTime >= 0.f; }
	bool IsShoving() const { return ShoveTime >= 0.f; }
	bool IsFramingRefill() const { return PreviousView.IsValid(); }
	bool IsComplete() const { return bAtLanding && bTookRisenTorch; }
	float WaterLevel() const { return Water; }
	bool IsDiverted() const { return bDiverted; }
	bool IsUnpinned() const { return bUnpinned; }
	bool IsGuided() const { return bGuided; }
	bool HasMountedTorch() const { return TorchMount != EHighWaterTorch::Carried; }
	bool IsTorchOnFloat() const { return TorchMount == EHighWaterTorch::Float; }
	FVector GetFloatLocal() const { return FloatLocal; }

private:
	enum class ETarget : uint8 { None, Wall, Trough, Timber, Float, LadderFork, Ladder, Landing };
	ETarget FindTarget(const FVector& Feet) const;
	void ShowPrompt(AWolfTrailGameMode& Mode, ETarget Target) const;
	void BuildScenery();
	void UpdateScenery(float Dt);
	FHighWaterScenery Scenery;
	void FrameRefill(APawn& Pawn);
	void EndRefillView(float Blend);
	void FrameCleft(APawn& Pawn, const FVector& Feet, float Dt);
	void RestoreCleftCamera();
	void StartClimb(ANight_of_the_WolvesCharacter& Max, bool bUp);
	void UpdateClimb(ANight_of_the_WolvesCharacter& Max, float Dt);
	void EndTraversal();
	void BeginTraversal(ANight_of_the_WolvesCharacter& Max);
	void SwimToShelf(ANight_of_the_WolvesCharacter& Max, bool bHoldingItem, float Dt);
	void PoseClimb(ANight_of_the_WolvesCharacter& Max, float Ascent, float Weight);
	void StartShove(ANight_of_the_WolvesCharacter& Max);
	void UpdateShove(ANight_of_the_WolvesCharacter& Max, float Dt);

	UPROPERTY(EditDefaultsOnly, Category="Survey chamber", meta=(ClampMin="1", Units="s"))
	float WaterTravelSeconds = 4.7f;
	UPROPERTY(EditDefaultsOnly, Category="Survey chamber", meta=(ClampMin="1", Units="s"))
	float ClimbSeconds = 3.1f;
	UPROPERTY() TObjectPtr<UCameraComponent> RefillCamera;
	UPROPERTY() TObjectPtr<USoundWave> PoolSplash;
	UPROPERTY(Transient) TObjectPtr<UPoseableMeshComponent> TraversalBody;
	TWeakObjectPtr<AActor> PreviousView;
	TWeakObjectPtr<APlayerController> RefillController;
	TWeakObjectPtr<APlayerController> ShoveController;
	TWeakObjectPtr<USpringArmComponent> CleftBoom;
	FVector NormalTargetOffset = FVector::ZeroVector;
	FVector NormalSocketOffset = FVector::ZeroVector;
	float NormalArmLength = 0.f;
	float ShoulderWeight = 0.f;
	TWeakObjectPtr<ANight_of_the_WolvesCharacter> TraversalMax;
	FVector StartFeet = FVector::ZeroVector;
	FVector SwimBank = FVector::ZeroVector;
	FVector ShoveFrom = FVector::ZeroVector;
	FVector ShoveDirection = FVector::ZeroVector;
	FVector FloatLocal = FVector::ZeroVector;
	FVector FloatBed = FVector::ZeroVector;
	FVector FloatDestination = FVector::ZeroVector;
	EHighWaterTorch TorchMount = EHighWaterTorch::Carried;
	float Water = 1.f;
	float TroughAngle = 0.f;
	float TimberRoll = 0.f;
	float ClimbTime = -1.f;
	float SwimTime = -1.f;
	float ShoveTime = -1.f;
	bool bBraced = false;
	bool bDiverted = false;
	bool bWallLit = false;
	bool bUnpinned = false;
	bool bGuided = false;
	bool bClimbingUp = false;
	bool bAtLanding = false;
	bool bTookRisenTorch = false;
};
