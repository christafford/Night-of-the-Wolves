#pragma once

#include "Animation/AnimInstance.h"
#include "DialogueFaceAnimInstance.generated.h"

struct FDialogueMouthPose;

// Copies the character's animated body pose and articulates its own MetaHuman face.
UCLASS(Transient)
class UDialogueFaceAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void SetPerformance(const FDialogueMouthPose& Mouth, float Anger, float GoldInterest,
		float LookDown = 0.f, float Blink = 0.f, float Fear = 0.f);

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;

private:
	friend struct FDialogueFaceProxy;
	float JawOpen = 0.f;
	float LipRound = 0.f;
	float LipWide = 0.f;
	float LipClose = 0.f;
	float LipTeeth = 0.f;
	float Scowl = 0.f;
	float Interest = 0.f;
	float DownwardGaze = 0.f;
	float BlinkAmount = 0.f;
	float Alarm = 0.f;
};
