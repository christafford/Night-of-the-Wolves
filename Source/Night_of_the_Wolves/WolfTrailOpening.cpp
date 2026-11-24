#include "WolfTrailOpening.h"
#include "WolfPackComponent.h"
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

void AWolfTrailOpening::Start(USplineComponent& Trail, APawn& Max, UWolfPackComponent& HuntingPack, float MaxDistance)
{
	Route = &Trail;
	Player = &Max;

	Pack = &HuntingPack;
	// The alpha takes point about six metres behind Max. The pressure wolf
	// settles another body length back, at the trail's usual pursuit mark.
	Destination = FMath::Max(0.f,MaxDistance-596.f);
	RunDuration = FMath::Max(TitleDissolve+2.f,Destination/RunSpeed + .5f*(Acceleration+1.05f));
	FrameCamera(0.f,0.f);
	UE_LOG(LogTemp, Display, TEXT("TRAIL OPENING title: %.2fs hold, %.2fs dissolve, %.0fcm trail run in %.2fs"), TitleHold,TitleDissolve,Destination,RunDuration);
}

float AWolfTrailOpening::TitleOpacity() const
{
	return 1.f - FMath::SmoothStep(TitleHold, TitleHold + TitleDissolve, Elapsed);
}

void AWolfTrailOpening::FrameCamera(float RunTime,float Dt)
{
	const float MoveTime = FMath::Min(RunTime,RunDuration);
	const float Brake = FMath::Min(1.05f, RunDuration * .5f);
	const float Ramp = FMath::Min(Acceleration, RunDuration * .25f);
	const float EffectiveSpeed = Destination / (RunDuration - .5f*(Ramp + Brake));
	float Travel = EffectiveSpeed * (MoveTime - .5f*Ramp);
	if (MoveTime < Ramp)
		Travel = .5f * EffectiveSpeed * MoveTime*MoveTime / Ramp;
	else if (MoveTime > RunDuration - Brake)
		Travel = Destination - .5f * EffectiveSpeed * FMath::Square(RunDuration - MoveTime) / Brake;

	const float Peel = FMath::SmoothStep(Destination-1930.f,Destination-1040.f,Travel);
	const float Gait = FMath::SmoothStep(0.f,Ramp,MoveTime) * (1.f-Peel);
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

	if (Peel > 0.f)
	{
		// The camera cranes away from the alpha's shoulder into a moving group
		// shot. Using a second rail sample keeps the lens inside the winding trail.
		const FVector RailBehind = Route->GetLocationAtDistanceAlongSpline(FMath::Max(0.f,Travel-590.f),Space);
		const FVector WideEye = RailBehind + Side*165.f + FVector::UpVector*215.f;
		const FVector PackCenter = Route->GetLocationAtDistanceAlongSpline(FMath::Max(0.f,Travel-68.f),Space)
			+ FVector::UpVector*49.f;
		Eye = FMath::Lerp(Eye,WideEye,Peel);
		Rotation = FQuat::Slerp(Rotation,(PackCenter-WideEye).Rotation().Quaternion(),Peel);
		FOV = FMath::Lerp(FOV,77.f,Peel);
	}
	Pack->RunOpening(*Route,Travel,Destination,Dt,FVector::DistSquared2D(Eye,Ground)>FMath::Square(120.f));
	const float Handoff = FMath::SmoothStep(RunDuration+LeaderHold,RunDuration+LeaderHold+CameraHandoff,RunTime);
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
	if (!Route.IsValid() || !Player.IsValid() || !Pack.IsValid()) return true;
	Elapsed += DeltaSeconds;
	const float RunTime = FMath::Max(Elapsed - TitleHold,0.f);
	if (!bBreathing && Elapsed >= TitleHold)
	{
		bBreathing = true;
		Breath->FadeIn(FMath::Min(1.15f,TitleDissolve),BreathingVolume);
		UE_LOG(LogTemp,Display,TEXT("TRAIL OPENING wolf POV: gallop and close breathing"));
	}
	if (!bBreathFading && RunTime >= RunDuration - 2.f)
	{
		bBreathFading = true;
		Breath->FadeOut(.95f,0.f);
		UE_LOG(LogTemp,Display,TEXT("TRAIL OPENING handoff: approaching Max"));
	}
	FrameCamera(RunTime,DeltaSeconds);
	return RunTime >= RunDuration+LeaderHold+CameraHandoff;
}
