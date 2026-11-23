#include "DialogueVoiceComponent.h"
#include "DialogueCameraModifier.h"
#include "DialogueFaceAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Sound/SoundWave.h"

UDialogueVoiceComponent::UDialogueVoiceComponent()
{
	bAutoActivate = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UDialogueVoiceComponent::BindFace(USkeletalMeshComponent* InFace)
{
	Face = InFace;
	Face->AddTickPrerequisiteComponent(this);
	OnAudioPlaybackPercentNative.AddUObject(this, &ThisClass::OnMixerPosition);
}

void UDialogueVoiceComponent::SetMood(float Anger, float Interest)
{
	if (Interest > InterestTarget) InterestAge = 0.f;
	AngerTarget = Anger;
	InterestTarget = Interest;
	SetComponentTickEnabled(true);
}

void UDialogueVoiceComponent::PlayLine(USoundBase* Wave, EDialogueLine Line)
{
	if (!Wave || !Face) return;
	if (IsDelivering()) FinishLine(false);
	Stop();
	ActiveLine = Line;
	Duration = Line == EDialogueLine::HermitFallYell ? Wave->GetDuration() : GetDialogueDuration(Line);
	Elapsed = MixerSeconds = SampleAge = 0.f;
	bHeardMixer = false;
	SetSound(Wave);
	Play();
	bPlayingAudio = IsPlaying();
	SetComponentTickEnabled(true);
	if (Line != EDialogueLine::HermitWarning && Line != EDialogueLine::HermitFallYell)
		Shot = UDialogueCameraModifier::FrameSpeaker(Face, Line);
}

void UDialogueVoiceComponent::OnMixerPosition(const UAudioComponent*, const USoundWave* Wave, float Percent)
{
	if (!IsDelivering() || Wave != Sound) return;
	const float Seconds = Percent * Duration;
	// A newly reused source can report 99–100% before its first genuine buffer.
	// Reject jumps that exceed time since the last sample, including after an initial zero.
	const float Expected = bHeardMixer ? MixerSeconds + SampleAge : Elapsed;
	if (Seconds > Expected + .12f) return;
	if (bHeardMixer && Seconds <= MixerSeconds) return;
	MixerSeconds = Seconds;
	SampleAge = 0.f;
	bHeardMixer = true;
}

void UDialogueVoiceComponent::FinishLine(bool bCut)
{
	if (Shot.IsValid()) Shot->ReleaseSpeaker(Face, bCut);
	ActiveLine = EDialogueLine::None;
}

void UDialogueVoiceComponent::ResetPerformance()
{
	FinishLine(true);
	Stop();
	AngerWeight = InterestWeight = AngerTarget = InterestTarget = InterestAge = 0.f;
	if (Face)
		if (auto* Anim = Cast<UDialogueFaceAnimInstance>(Face->GetAnimInstance()))
			Anim->SetPerformance({}, 0.f, 0.f);
	SetComponentTickEnabled(false);
}

void UDialogueVoiceComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(Dt, TickType, TickFunction);
	if (!Face) return;
	FDialogueMouthPose Mouth;
	if (IsDelivering() && GetPlayState() != EAudioComponentPlayState::Paused)
	{
		Elapsed += Dt;
		SampleAge += Dt;
		// The final decoded buffer can still be on its way to the speakers after playback
		// stops. Let the mouth (and the portrait) finish on the audible clock.
		const bool bTail = bPlayingAudio && (!IsPlaying() || MixerSeconds >= Duration - .07f);
		const float Position = bHeardMixer ? MixerSeconds + (bTail ? SampleAge : FMath::Min(SampleAge, .035f))
			: Elapsed - (bPlayingAudio ? .045f : 0.f);
		const float Seconds = Position - (bPlayingAudio ? OutputLatency : 0.f);
		if (Seconds >= Duration)
			FinishLine(false);
		else
		{
			if (ActiveLine == EDialogueLine::HermitFallYell)
			{
				// The wordless cry opens on the first vowel and trails off before impact.
				Mouth.JawOpen = FMath::SmoothStep(.02f, .13f, Seconds) *
					(1.f - FMath::SmoothStep(Duration - .22f, Duration - .07f, Seconds));
				Mouth.LipWide = Mouth.JawOpen * .45f;
			}
			else Mouth = SampleDialogueMouth(ActiveLine, Seconds);
		}
	}

	AngerWeight = FMath::FInterpTo(AngerWeight, AngerTarget, Dt, 9.f);
	InterestWeight = FMath::FInterpTo(InterestWeight, InterestTarget, Dt, 8.f);
	InterestAge += Dt;
	// He glances at the sack, then brings his eyes back to Max with the smile still there.
	const float Glance = InterestWeight * (1.f - FMath::SmoothStep(.40f, 1.05f, InterestAge));
	const float Blink = InterestWeight * (1.f - FMath::Clamp(FMath::Abs(InterestAge - .16f) / .085f, 0.f, 1.f));
	if (auto* Anim = Cast<UDialogueFaceAnimInstance>(Face->GetAnimInstance()))
		Anim->SetPerformance(Mouth, AngerWeight, InterestWeight, Glance, Blink);
	if (!IsDelivering() && InterestAge > 1.05f && FMath::IsNearlyEqual(AngerTarget, AngerWeight) &&
		FMath::IsNearlyEqual(InterestTarget, InterestWeight))
		SetComponentTickEnabled(false);
}
