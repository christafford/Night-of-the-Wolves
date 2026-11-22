#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "WolfMeshComponent.generated.h"

// A skinned quadruped. The foot cycle is driven by distance traveled; a stationary wolf breathes.
UCLASS()
class UWolfMeshComponent : public UPoseableMeshComponent
{
	GENERATED_BODY()

public:
	UWolfMeshComponent();
	void ResetGait(float StartingPhase);
	void AdvanceGait(float DeltaSeconds, float TravelCm, bool bSwimming = false, float Threat = 0.f);
	void SetPouncePose(float Progress);

private:
	struct FLeg { int32 Upper, Lower, Pastern, Paw; bool bFore, bSecondBeat; };
	TArray<FLeg> Legs;
	TArray<FTransform> Rest;
	TArray<FTransform> Posed;
	int32 Chest = INDEX_NONE, Neck = INDEX_NONE, Skull = INDEX_NONE, Jaw = INDEX_NONE;
	int32 LeftEar = INDEX_NONE, RightEar = INDEX_NONE, Brush = INDEX_NONE, Tip = INDEX_NONE;
	float Phase = 0.f, Clock = 0.f, Speed = 0.f;

	bool PrepareRig();
	void Inherit(int32 Bone);
	void Tilt(int32 Bone, FRotator Offset);
	void AimAt(int32 Bone, int32 Child, const FVector& Target);
	void Reach(const FLeg& Leg, FVector Foot, float PawPitch);
	void FinishPose();
};
