#pragma once

#include "Components/AudioComponent.h"
#include "DialogueArticulation.h"
#include "DialogueVoiceComponent.generated.h"

class UDialogueCameraModifier;
class USkeletalMeshComponent;

// One voice, face and playback cursor per speaker. The mixer owns the timing.
UCLASS(Config=Game)
class UDialogueVoiceComponent : public UAudioComponent
{
	GENERATED_BODY()

public:
	UDialogueVoiceComponent();
	void BindFace(USkeletalMeshComponent* InFace);
	void PlayLine(USoundBase* Wave, EDialogueLine Line);
	void SetMood(float Anger, float Interest, float Fear = 0.f);
	void ResetPerformance();
	bool IsDelivering() const { return ActiveLine != EDialogueLine::None; }
	virtual void TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;

private:
	// Linux mixer's source cursor precedes the audio reaching the output device.
	UPROPERTY(EditAnywhere, Config, Category="Dialogue", meta=(ClampMin="0", ClampMax="0.5", Units="s"))
	float OutputLatency = 0.f;
	float FearWeight = 0.f, FearTarget = 0.f;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Face;
	TWeakObjectPtr<UDialogueCameraModifier> Shot;
	EDialogueLine ActiveLine = EDialogueLine::None;
	float Elapsed = 0.f;
	float Duration = 0.f;
	float MixerSeconds = 0.f;
	float SampleAge = 0.f;
	float AngerTarget = 0.f, InterestTarget = 0.f;
	float AngerWeight = 0.f, InterestWeight = 0.f;
	float InterestAge = 0.f;
	bool bHeardMixer = false;
	bool bPlayingAudio = false;

	void OnMixerPosition(const UAudioComponent* Component, const USoundWave* Wave, float Percent);
	void FinishLine(bool bCut);
};
