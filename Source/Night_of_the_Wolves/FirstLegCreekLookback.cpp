#include "FirstLegCrossing.h"
#include "FirstLegCrossingLayout.h"
#include "WolfPackComponent.h"
#include "WolfMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/App.h"

namespace
{
constexpr float BridgeArrival = 1.65f;
constexpr float LeaveBridge = 3.1f;
constexpr double WaterSurface = -209.;
struct FTrackMark
{
	float Time;
	double X, Y;
};

// Move back around the broken handrail, then enter the water downriver. The
// return to play is mid-stroke, well before the wolves climb out on Max's bank.
constexpr FTrackMark Tracks[] = {
	{BridgeArrival,9741,-1511}, {LeaveBridge,9741,-1511},
	{3.70f,9534,-1547}, {4.17f,9541,-1758}, {4.85f,9682,-2040}, {5.63f,9852,-2165},
	{6.65f,10024,-2265}, {8.1f,10193,-2337}, {14.9f,10680,-2450},
	{16.65f,10820,-2384}, {19.1f,11070,-2180}
};

FVector CreekFeet(float T, int32 Wolf, FVector Start)
{
	constexpr double Lanes[] = {0., -1., .93, -1.76, 1.65};
	const double Lane = Lanes[Wolf];
	FVector Position(Tracks[0].X,Tracks[0].Y,64.);
	if (T < BridgeArrival)
	{
		Position += FVector(-FMath::Abs(Lane)*104., Lane*76., 0.);
		return FMath::Lerp(Start, Position, FMath::SmoothStep(0.f,BridgeArrival,T));
	}
	for (int32 I=1; I<UE_ARRAY_COUNT(Tracks); ++I)
	{
		const auto& A = Tracks[I-1];
		const auto& B = Tracks[I];
		const float Alpha = FMath::Clamp((T-A.Time)/(B.Time-A.Time),0.f,1.f);
		Position = FVector(FMath::Lerp(A.X,B.X,Alpha),FMath::Lerp(A.Y,B.Y,Alpha),0.);
		if (T<=B.Time) break;
	}
	const float Turning = FMath::SmoothStep(3.1f,4.2f,T);
	const float Entering = FMath::SmoothStep(4.85f,6.3f,T);
	// The outer wolves arrive behind the leader and take separate paths round
	// the end post. Once afloat, each has its own channel through the current.
	Position.X -= FMath::Abs(Lane)*104.*(1.-Turning) + Lane*68.*Turning*(1.-Entering);
	Position.Y += Lane*(76.+Entering*33.);

	const double Bend = FirstLegLayout::StreamBend(Position.Y);
	const double DepthFromLip = FMath::Min(Position.X-(9800.+Bend), (10900.+Bend)-Position.X);
	const double Bank = FMath::Clamp((245.-DepthFromLip)/250., 0., 1.);
	const double Surface = -255. + 298.*(1.-FMath::Pow(1.-Bank,1.65));
	const double SwimZ = WaterSurface - (Wolf==0 ? 66. : 61.) + 2.6*FMath::Sin(5.3*T+Wolf);
	// Beyond the shingle shelf they paddle with their backs at the surface.
	// The artistic creek bed under deep water has no bearing on their waterline.
	const double Wet = FMath::SmoothStep(172.,260.,DepthFromLip);
	const double FeetZ = FMath::Lerp(Surface,SwimZ,Wet);
	const double WeightOnPlanks = 1.-FMath::SmoothStep(LeaveBridge,3.68f,T);
	Position.Z = FMath::Lerp(FeetZ,64.,WeightOnPlanks);
	return Position;
}
}

bool AFirstLegCrossing::IsWatchingCreekPack() const
{
	return Phase == EPhase::CreekLookback;
}

void AFirstLegCrossing::BeginCreekLookback(APawn& Pawn)
{
	ACharacter* Max = Cast<ACharacter>(&Pawn);
	APlayerController* PC = Cast<APlayerController>(Pawn.GetController());
	if (!PC || !Max) return;
	if (!CreekCamera) CreekCamera = GetWorld()->SpawnActor<ACameraActor>();
	if (!CreekCamera || !GetLeadWolf()) return;

	RestoreCableView();
	for (int32 I=0; I<UWolfPackComponent::Count; ++I)
		CreekStart[I] = Pack->GetMember(I)->GetComponentLocation();
	CueTime = 0.f;
	CreekShotTime = 0.f;
	bReturningFromCreek = false;
	Phase = EPhase::CreekLookback;
	CreekViewer = PC;
	CreekPawn = Max;
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	bCreekJumpAllowed = Max->GetCharacterMovement()->IsJumpAllowed();
	Max->GetCharacterMovement()->SetJumpAllowed(false);
	Max->StopJumping();
	Max->ConsumeMovementInputVector();
	Max->GetCharacterMovement()->StopMovementImmediately();
	FrameCreekPack(0.f);
	PC->SetViewTargetWithBlend(CreekCamera,.36f,VTBlend_Cubic,0.f,true);
}

void AFirstLegCrossing::FrameCreekPack(float StoryTime)
{
	const FVector Lead = Pack->GetMember(0)->GetComponentLocation();
	const FVector Left = Pack->GetMember(1)->GetComponentLocation();
	const FVector Right = Pack->GetMember(2)->GetComponentLocation();
	const float TrackingSwimmers = FMath::SmoothStep(4.1f,6.85f,StoryTime);
	const FVector Group = .55*Lead + .225*(Left+Right);
	// Open on both severed ends across the stream. While the leader slows,
	// push in on the near stub from over the channel. The lens then tracks
	// downriver on the same side of the dogs throughout the descent.
	const float PushIn = FMath::SmoothStep(.7f,2.22f,StoryTime);
	const FVector BridgeGap(10335,-1505,0);
	const FVector GroupFocus = Group + FVector(34,-10,FMath::Lerp(38.f,55.f,TrackingSwimmers));
	const FVector Focus = FMath::Lerp(BridgeGap,GroupFocus,PushIn);
	const FVector TrackingOffset = FMath::Lerp(FVector(538,-320,338),FVector(385,-370,288),TrackingSwimmers);
	const FVector Eye = FMath::Lerp(FVector(10450,-2615,470),GroupFocus+TrackingOffset,PushIn);
	CreekCamera->SetActorLocationAndRotation(Eye,(Focus-Eye).Rotation());
	CreekCamera->GetCameraComponent()->SetFieldOfView(FMath::Lerp(77.f,53.f,PushIn));
}

void AFirstLegCrossing::TickCreekLookback(float Delta)
{
	if (!CreekViewer.IsValid() || !CreekPawn.IsValid() || CreekViewer->GetPawn()!=CreekPawn.Get())
	{
		EndCreekLookback();
		return;
	}
	const float Length = FMath::Max(4.f,CreekLookbackSeconds);
	CreekShotTime = FMath::Min(Length,CreekShotTime+Delta);
	const float StoryTime = CreekShotTime*CreekHandoff/Length;
	PlaceCreekPack(StoryTime,Delta);
	FrameCreekPack(StoryTime);
	constexpr float HandbackBlend = .52f;
	if (!bReturningFromCreek && CreekShotTime>=Length-HandbackBlend)
	{
		bReturningFromCreek = true;
		CreekViewer->SetViewTargetWithBlend(CreekPawn.Get(),HandbackBlend,VTBlend_Cubic,0.f,true);
	}
	if (CreekShotTime>=Length)
		EndCreekLookback();
}

void AFirstLegCrossing::EndCreekLookback()
{
	if (CreekPawn.IsValid())
		CreekPawn->GetCharacterMovement()->SetJumpAllowed(bCreekJumpAllowed);
	if (auto* PC=CreekViewer.Get())
	{
		if (APawn* Current = PC->GetPawn())
			PC->SetViewTarget(Current);
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
	CreekPawn.Reset();
	CreekViewer.Reset();
	if (Phase==EPhase::CreekLookback) Phase=EPhase::FarBank;
}

void AFirstLegCrossing::PlaceCreekPack(float StoryTime, float Delta, bool bSnap)
{
	FVector Feet[UWolfPackComponent::Count], Facing[UWolfPackComponent::Count];
	float Peer[UWolfPackComponent::Count];
	for (int32 I=0; I<UWolfPackComponent::Count; ++I)
	{
		const float T = FMath::Max(0.f,StoryTime-I*.19f);
		Feet[I] = CreekFeet(T,I,CreekStart[I]);
		Facing[I] = (CreekFeet(T+.075f,I,CreekStart[I])-Feet[I]).GetSafeNormal2D();
		if (T>=BridgeArrival && T<=LeaveBridge)
			Facing[I] = FMath::Lerp(FVector(1,.08,0),FVector(.12,-1,0),FMath::SmoothStep(2.75f,LeaveBridge,T)).GetSafeNormal();
		else if (Facing[I].IsNearlyZero()) Facing[I]=FVector::ForwardVector;
		Peer[I] = FMath::SmoothStep(1.58f,2.07f,T)*(1.f-FMath::SmoothStep(2.61f,3.08f,T));
	}
	Pack->FollowCreek(Delta,Feet,Facing,Peer,bSnap);
	if (!FApp::CanEverRender()) return;
	if (CreekWakes.IsEmpty())
	{
		// Two broken ripples peel away from each shoulder. They follow the live
		// swimmers and disappear individually as their feet regain the bank.
		for (int32 I=0; I<4*UWolfPackComponent::Count; ++I)
		{
			auto* Ripple = Part(Round,FoamMat);
			Ripple->SetLightingChannels(true,true,false);
			Ripple->SetCastShadow(false);
			Ripple->SetVisibility(false);
			CreekWakes.Add(Ripple);
		}
	}
	for (int32 I=0; I<UWolfPackComponent::Count; ++I)
	{
		auto* Wolf = Pack->GetMember(I);
		const FVector P = Wolf->GetComponentLocation();
		const FVector Forward = Wolf->GetForwardVector().GetSafeNormal2D();
		const FVector Side(-Forward.Y,Forward.X,0);
		const float Immersion = FMath::GetMappedRangeValueClamped(FVector2D(-238.,-253.),FVector2D(0.,1.),P.Z);
		for (int32 J=0; J<4; ++J)
		{
			auto* Ripple = CreekWakes[I*4+J].Get();
			Ripple->SetVisibility(Immersion>.01f);
			if (Immersion<=.01f) continue;
			const float Sign = (J%2==0) ? -1.f : 1.f;
			const float Beat = FMath::Frac(StoryTime*1.35f + I*.27f + (J/2)*.5f);
			const FVector Pxy = P + Forward*(12.-Beat*115.) + Side*(Sign*(14.+Beat*39.));
			Ripple->SetWorldLocationAndRotation(FVector(Pxy.X,Pxy.Y,WaterSurface+1.7),FRotator(0,Forward.Rotation().Yaw-Sign*19.,0));
			const float Crest = (.38f+.62f*FMath::Sin(PI*Beat))*Immersion;
			Ripple->SetWorldScale3D(FVector(.74f*Crest,.080f*Crest,.017f));
		}
	}
}

void AFirstLegCrossing::HideCreekWakes()
{
	for (UStaticMeshComponent* Ripple : CreekWakes) Ripple->SetVisibility(false);
}
