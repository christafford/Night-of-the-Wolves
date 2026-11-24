#include "FirstLegCrossing.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "UObject/ConstructorHelpers.h"

void AFirstLegCrossing::InitializeCableSounds()
{
	WheelAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("RollingSheave"));
	HardwareAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("CableHardware"));
	for (auto* Audio : {WheelAudio.Get(), HardwareAudio.Get()})
	{
		Audio->SetupAttachment(RootComponent);
		Audio->bAutoActivate = false;
		Audio->bOverrideAttenuation = true;
		Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(350);
		Audio->AttenuationOverrides.FalloffDistance = 1600;
	}
	static ConstructorHelpers::FObjectFinder<USoundWave> Roll(TEXT("/Game/NightOfTheWolves/Crossing/Foley/SheaveRoll"));
	static ConstructorHelpers::FObjectFinder<USoundWave> Eye(TEXT("/Game/NightOfTheWolves/Crossing/Foley/ShackleClink"));
	static ConstructorHelpers::FObjectFinder<USoundWave> Stop(TEXT("/Game/NightOfTheWolves/Crossing/Foley/EndStop"));
	WheelAudio->SetSound(Roll.Object);
	EyeClink = Eye.Object;
	StopClank = Stop.Object;
}

void AFirstLegCrossing::StartCableRoll(const FVector& Position)
{
	WheelAudio->SetWorldLocation(Position);
	WheelAudio->SetPitchMultiplier(.70f);
	WheelAudio->SetVolumeMultiplier(.58f);
	WheelAudio->FadeIn(.10f);
}

void AFirstLegCrossing::UpdateCableRoll(const FVector& Position, float SpeedFraction)
{
	if (SpeedFraction <= 0.f)
	{
		WheelAudio->Stop();
		return;
	}
	const float Speed = FMath::Min(SpeedFraction, 1.f);
	WheelAudio->SetWorldLocation(Position);
	WheelAudio->SetPitchMultiplier(FMath::Lerp(.70f, 1.20f, Speed));
	WheelAudio->SetVolumeMultiplier(FMath::Lerp(.48f, .88f, Speed));
}

void AFirstLegCrossing::SoundCableLatch(const FVector& Position, bool bBuffer)
{
	HardwareAudio->SetWorldLocation(Position);
	HardwareAudio->SetSound(bBuffer ? StopClank : EyeClink);
	HardwareAudio->SetVolumeMultiplier(bBuffer ? .86f : .71f);
	HardwareAudio->Play();
}
