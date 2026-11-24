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
	void SetIdentity(int32 Member);
	void SetHuntGesture(float SignedSignal, float Investigate);
	int32 GetIdentity() const { return Identity; }
	void ResetGait(float StartingPhase);
	void AdvanceGait(float DeltaSeconds, float TravelCm, bool bSwimming = false, float Threat = 0.f, float Peer = 0.f);
	void SetPouncePose(float Progress);
	void SetStrugglePose(float Weight, float Beat);
	void SetFireRecoil(float Weight);

private:
	struct FLeg { int32 Upper, Lower, Pastern, Paw; bool bFore, bSecondBeat; };
	TArray<FLeg> Legs;
	TArray<FTransform> Rest;
	TArray<FTransform> Posed;
	int32 Chest = INDEX_NONE, Neck = INDEX_NONE, Skull = INDEX_NONE, Jaw = INDEX_NONE;
	int32 LeftEar = INDEX_NONE, RightEar = INDEX_NONE, Brush = INDEX_NONE, Tip = INDEX_NONE;
	int32 Identity = 0;
	float SignalTarget = 0, InvestigateTarget = 0, SignalPose = 0, InvestigatePose = 0;
	float Phase = 0.f, Clock = 0.f, Speed = 0.f;

	bool PrepareRig();
	void Inherit(int32 Bone);
	void Tilt(int32 Bone, FRotator Offset);
	void AimAt(int32 Bone, int32 Child, const FVector& Target);
	void Reach(const FLeg& Leg, FVector Foot, float PawPitch);
	void FinishPose();
};
