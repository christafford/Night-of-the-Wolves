#include "WolfTrailOpening.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float Acceleration = .6f;
constexpr float StridesPerSecond = 2.65f;
}

AWolfTrailOpening::AWolfTrailOpening()
{
	GetCameraComponent()->bConstrainAspectRatio = false;
	Breath = CreateDefaultSubobject<UAudioComponent>(TEXT("Wolf running breath"));
	Breath->SetupAttachment(GetRootComponent());
	Breath->bAutoActivate = false;
	Breath->bAllowSpatialization = false;
	Breath->bIsUISound = false;
	static ConstructorHelpers::FObjectFinder<USoundBase> Pant(TEXT("/Game/NightOfTheWolves/Presentation/WolfPOVRunningBreath"));
	Breath->SetSound(Pant.Object);
}

void AWolfTrailOpening::Start(USplineComponent& Trail, APawn& Max, float MaxDistance)
{
	Route = &Trail;
	Player = &Max;

	// Aim for the live spring-arm socket, behind the pawn. Its horizontal arm
	// length also works before the first spring-arm tick has resolved collision.
	float FollowDistance = 400.f;
	if (const auto* Arm = Max.FindComponentByClass<USpringArmComponent>())
		FollowDistance = Arm->TargetArmLength;
	Destination = FMath::Max(0.f, MaxDistance - FollowDistance);
	RunDuration = FMath::Max(TitleDissolve + CameraHandoff, Destination / RunSpeed + .5f * (Acceleration + CameraHandoff));
	FrameCamera(0.f);
	UE_LOG(LogTemp, Display, TEXT("TRAIL OPENING title: %.2fs hold, %.2fs dissolve, %.0fcm trail run in %.2fs"), TitleHold,TitleDissolve,Destination,RunDuration);
}

float AWolfTrailOpening::TitleOpacity() const
{
	return 1.f - FMath::SmoothStep(TitleHold, TitleHold + TitleDissolve, Elapsed);
}

void AWolfTrailOpening::FrameCamera(float RunTime)
{
	const float Brake = FMath::Min(CameraHandoff, RunDuration * .5f);
	const float Ramp = FMath::Min(Acceleration, RunDuration * .25f);
	const float EffectiveSpeed = Destination / (RunDuration - .5f*(Ramp + Brake));
	float Travel = EffectiveSpeed * (RunTime - .5f*Ramp);
	if (RunTime < Ramp)
		Travel = .5f * EffectiveSpeed * RunTime*RunTime / Ramp;
	else if (RunTime > RunDuration - Brake)
		Travel = Destination - .5f * EffectiveSpeed * FMath::Square(RunDuration - RunTime) / Brake;

	const float Handoff = FMath::SmoothStep(RunDuration-Brake,RunDuration,RunTime);
	const float Gait = FMath::SmoothStep(0.f,Ramp,RunTime) * (1.f-Handoff);
	const auto Space = ESplineCoordinateSpace::World;
	const FVector Ground = Route->GetLocationAtDistanceAlongSpline(Travel,Space);
	// The authored spline consists of short straight segments. A forward sightline
	// across several samples anticipates bends without snapping at their joins.
	const FVector LookAhead = Route->GetLocationAtDistanceAlongSpline(Travel + 390.f,Space);
	const FVector Back = Route->GetLocationAtDistanceAlongSpline(FMath::Max(0.f,Travel - 95.f),Space);
	const FVector Heading = (LookAhead-Back).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(FVector::UpVector,Heading).GetSafeNormal();
	const float Phase = UE_TWO_PI * StridesPerSecond * RunTime;
	const float Bounce = FMath::Sin(Phase) + .17f*FMath::Sin(2.f*Phase-.55f);
	FVector Eye = Ground + FVector::UpVector*(EyeHeight + Gait*HeadBob*Bounce) + Side*(Gait*2.8f*FMath::Sin(.5f*Phase));
	FRotator Facing = Heading.Rotation();
	Facing.Pitch += 1.4f + Gait*1.15f*FMath::Sin(Phase+.55f);
	Facing.Roll = Gait*.85f*FMath::Sin(.5f*Phase);
	FQuat Rotation = Facing.Quaternion();
	float FOV = 83.f;

	if (Handoff > 0.f)
	{
		// Sample Max's actual camera including spring-arm collision, offsets and FOV.
		// The final frame is identical to the next view target.
		FMinimalViewInfo Live;
		Player->CalcCamera(0.f,Live);
		Eye = FMath::Lerp(Eye,Live.Location,Handoff);
		Rotation = FQuat::Slerp(Rotation,Live.Rotation.Quaternion(),Handoff);
		FOV = FMath::Lerp(FOV,Live.FOV,Handoff);
	}
	SetActorLocationAndRotation(Eye,Rotation);
	GetCameraComponent()->SetFieldOfView(FOV);
}

bool AWolfTrailOpening::Advance(float DeltaSeconds)
{
	if (!Route.IsValid() || !Player.IsValid()) return true;
	Elapsed += DeltaSeconds;
	const float RunTime = FMath::Clamp(Elapsed - TitleHold,0.f,RunDuration);
	if (!bBreathing && Elapsed >= TitleHold)
	{
		bBreathing = true;
		Breath->FadeIn(FMath::Min(1.15f,TitleDissolve),BreathingVolume);
		UE_LOG(LogTemp,Display,TEXT("TRAIL OPENING wolf POV: gallop and close breathing"));
	}
	if (!bBreathFading && RunTime >= RunDuration - FMath::Min(CameraHandoff,RunDuration*.5f))
	{
		bBreathFading = true;
		Breath->FadeOut(FMath::Max(.01f,RunDuration-RunTime),0.f);
		UE_LOG(LogTemp,Display,TEXT("TRAIL OPENING handoff: approaching Max"));
	}
	FrameCamera(RunTime);
	return RunTime >= RunDuration;
}
