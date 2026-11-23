#pragma once

#include "Camera/CameraModifier.h"
#include "DialogueArticulation.h"
#include "DialogueCameraModifier.generated.h"

class USkeletalMeshComponent;

// Composes the portrait over the current player or reveal camera. Player control rotation stays live.
UCLASS()
class UDialogueCameraModifier : public UCameraModifier
{
	GENERATED_BODY()
public:
	static UDialogueCameraModifier* FrameSpeaker(USkeletalMeshComponent* Face, EDialogueLine Line);
	void ReleaseSpeaker(USkeletalMeshComponent* Face, bool bCut);
	virtual bool ModifyCamera(float Dt, FMinimalViewInfo& POV) override;
private:
	TWeakObjectPtr<USkeletalMeshComponent> Subject;
	EDialogueLine Line = EDialogueLine::None;
	bool bEntering = false, bCutToPortrait = false;
	float StartAlpha = 0.f, DirectionSign = 1.f;
	double TransitionTime = 0;
	FRotator StartControlRotation = FRotator::ZeroRotator;
	FVector SmoothedFocus = FVector::ZeroVector;
	uint64 LastFrame = MAX_uint64;
	float BlendWeight() const;
};
