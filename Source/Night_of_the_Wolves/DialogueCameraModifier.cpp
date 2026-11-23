#include "DialogueCameraModifier.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
FVector Eyes(const USkeletalMeshComponent* Face)
{
	static const FName Left(TEXT("FACIAL_L_Eye"));
	static const FName Right(TEXT("FACIAL_R_Eye"));
	return (Face->GetSocketLocation(Left) + Face->GetSocketLocation(Right)) * .5;
}
}

UDialogueCameraModifier* UDialogueCameraModifier::FrameSpeaker(USkeletalMeshComponent* Face, EDialogueLine Line)
{
	auto* PC = Face->GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager) return nullptr;
	auto* Manager = PC->PlayerCameraManager.Get();
	auto* Shot = Cast<UDialogueCameraModifier>(Manager->FindCameraModifierByClass(StaticClass()));
	if (!Shot) Shot = Cast<UDialogueCameraModifier>(Manager->AddNewCameraModifier(StaticClass()));
	if (!Shot) return nullptr;

	const FVector Eye = Eyes(Face);
	const FVector CameraOffset = Manager->GetCameraLocation() - Eye;
	const float Existing = Shot->Subject.Get() == Face ? Shot->BlendWeight() : 0.f;
	if (Existing < .01f)
	{
		Shot->SmoothedFocus = Eye;
		Shot->DirectionSign = FVector::DotProduct(CameraOffset, Face->GetForwardVector()) < 0.f ? -1.f : 1.f;
		// A long trip through the trees (or through the lip of the pit) is an editorial cut,
		// followed by a short lens push. Nearby views can dolly all the way in.
		Shot->bCutToPortrait = CameraOffset.Size() > 650.f || Line == EDialogueLine::HermitPit;
	}
	Shot->Subject = Face;
	Shot->Line = Line;
	Shot->StartAlpha = Existing;
	Shot->TransitionTime = Face->GetWorld()->GetTimeSeconds();
	Shot->bEntering = true;
	Shot->StartControlRotation = PC->GetControlRotation();
	return Shot;
}

float UDialogueCameraModifier::BlendWeight() const
{
	const float Age = GetWorld()->GetTimeSeconds() - TransitionTime;
	return FMath::Lerp(StartAlpha, bEntering ? 1.f : 0.f,
		FMath::SmoothStep(0.f, bEntering ? .23f : .22f, Age));
}

void UDialogueCameraModifier::ReleaseSpeaker(USkeletalMeshComponent* Face, bool bCut)
{
	if (Subject.Get() != Face) return;
	if (bCut) { Subject.Reset(); return; }
	if (bEntering)
	{
		StartAlpha = BlendWeight();
		bEntering = false;
		TransitionTime = GetWorld()->GetTimeSeconds();
	}
}

bool UDialogueCameraModifier::ModifyCamera(float Dt, FMinimalViewInfo& POV)
{
	auto* Face = Subject.Get();
	if (!Face) return false;
	const bool bBandit = Line == EDialogueLine::BanditDemand || Line == EDialogueLine::BanditGold ||
		Line == EDialogueLine::BanditStay;
	const auto* PC = CameraOwner->GetOwningPlayerController();
	if (bEntering && bBandit && PC)
	{
		const FRotator Turn = (PC->GetControlRotation() - StartControlRotation).GetNormalized();
		if (FMath::Abs(Turn.Yaw) > 18.f || FMath::Abs(Turn.Pitch) > 15.f)
			ReleaseSpeaker(Face, false);
	}
	const float Weight = BlendWeight();
	if (!bEntering && Weight < .001f) { Subject.Reset(); return false; }

	const FVector Eye = Eyes(Face);
	if (LastFrame != GFrameCounter)
	{
		// Just enough stabilization to leave his head movement readable in frame.
		SmoothedFocus = FMath::VInterpTo(SmoothedFocus, Eye, Dt, 22.f);
		LastFrame = GFrameCounter;
	}
	const bool bInPit = Line == EDialogueLine::HermitPit;
	const FVector Forward = Face->GetRightVector().GetSafeNormal2D(); // Creator heads face mesh +Y.
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Forward);
	const double Offset = bBandit ? -25.0 : -DirectionSign * 32.0;
	const FVector Focus = SmoothedFocus - FVector(0, 0, bBandit ? 4.f : 6.f);
	// His head is still above the rim during the plea. Stay under the brim.
	FVector Location = Focus + Forward * (bInPit ? 109.0 : 122.0) + Side * Offset +
		FVector(0, 0, bInPit ? -5.f : 7.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DialogueCamera), false, Face->GetOwner());
	if (PC) Params.AddIgnoredActor(PC->GetPawn());
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Focus, Location, ECC_Camera, Params) && !Hit.bStartPenetrating)
		Location = Hit.Location - (Location - Focus).GetSafeNormal() * 4.f;
	const FRotator PortraitLook = (Focus - Location).Rotation();
	const float PortraitFov = bInPit ? 33.f : 30.f;
	const float Travel = bCutToPortrait ? 1.f : Weight;
	POV.Location = FMath::Lerp(POV.Location, Location, Travel);
	POV.Rotation = FQuat::Slerp(POV.Rotation.Quaternion(), PortraitLook.Quaternion(), Travel).Rotator();
	POV.FOV = FMath::Lerp(bCutToPortrait ? 44.f : POV.FOV, PortraitFov, Weight);
	return false;
}
