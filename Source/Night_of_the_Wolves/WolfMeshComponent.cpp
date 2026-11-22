#include "WolfMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

UWolfMeshComponent::UWolfMeshComponent()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Animal(TEXT("/Game/NightOfTheWolves/Wolves/SK_TimberWolf"));
	SetSkinnedAssetAndUpdate(Animal.Object);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	// The gameplay controller explicitly evaluates the rig, including while off camera.
	PrimaryComponentTick.bCanEverTick = false;
	BoundsScale = 1.45f;
}

bool UWolfMeshComponent::PrepareRig()
{
	if (!GetSkinnedAsset() || !BoneSpaceTransforms.Num()) return false;
	if (!Rest.IsEmpty()) return Legs.Num() == 4;
	const FReferenceSkeleton& Skeleton = GetSkinnedAsset()->GetRefSkeleton();
	Rest = Skeleton.GetRefBonePose();
	for (int32 I = 1; I < Rest.Num(); ++I) Rest[I] *= Rest[Skeleton.GetParentIndex(I)];
	Posed = Rest;
	Chest = GetBoneIndex(TEXT("Ribcage")); Neck = GetBoneIndex(TEXT("Neck"));
	Skull = GetBoneIndex(TEXT("Skull")); Jaw = GetBoneIndex(TEXT("Jaw"));
	Brush = GetBoneIndex(TEXT("Brush")); Tip = GetBoneIndex(TEXT("BrushTip"));
	LeftEar = GetBoneIndex(TEXT("EarL")); RightEar = GetBoneIndex(TEXT("EarR"));
	if (Chest < 0 || Neck < 0 || Skull < 0 || Jaw < 0 || Brush < 0 || Tip < 0 || LeftEar < 0 || RightEar < 0) return false;
	for (const bool bFore : {true, false})
	{
		for (const bool bLeft : {true, false})
		{
			const FString Prefix = FString(bFore ? TEXT("Fore") : TEXT("Hind")) + (bLeft ? TEXT("L") : TEXT("R"));
			FLeg Leg{GetBoneIndex(*(Prefix+TEXT("Upper"))), GetBoneIndex(*(Prefix+TEXT("Lower"))),
				GetBoneIndex(*(Prefix+TEXT("Pastern"))), GetBoneIndex(*(Prefix+TEXT("Paw"))), bFore, bFore != bLeft};
			if (Leg.Upper < 0 || Leg.Lower < 0 || Leg.Pastern < 0 || Leg.Paw < 0) return false;
			Legs.Add(Leg);
		}
	}
	return true;
}

void UWolfMeshComponent::ResetGait(float StartingPhase)
{
	Phase = StartingPhase;
	Clock = StartingPhase * 3.f;
	Speed = 0.f;
	AdvanceGait(0, 0);
}

void UWolfMeshComponent::Inherit(int32 Bone)
{
	const FReferenceSkeleton& Skeleton = GetSkinnedAsset()->GetRefSkeleton();
	Posed[Bone] = Skeleton.GetRefBonePose()[Bone] * Posed[Skeleton.GetParentIndex(Bone)];
}

void UWolfMeshComponent::Tilt(int32 Bone, FRotator Offset)
{
	Inherit(Bone);
	Posed[Bone].SetRotation(Offset.Quaternion() * Posed[Bone].GetRotation());
}

void UWolfMeshComponent::AimAt(int32 Bone, int32 Child, const FVector& Target)
{
	Inherit(Bone);
	const FVector RefDirection = (Rest[Child].GetLocation() - Rest[Bone].GetLocation()).GetSafeNormal();
	const FVector Direction = (Target - Posed[Bone].GetLocation()).GetSafeNormal();
	const FQuat Swing = FQuat::FindBetweenNormals(RefDirection, Direction);
	Posed[Bone].SetRotation(Swing * Rest[Bone].GetRotation());
}

void UWolfMeshComponent::Reach(const FLeg& Leg, FVector Foot, float PawPitch)
{
	Inherit(Leg.Upper);
	const FVector Socket = Posed[Leg.Upper].GetLocation();
	const FVector Ankle = Foot + (Rest[Leg.Pastern].GetLocation() - Rest[Leg.Paw].GetLocation());
	const FVector Axis = (Ankle - Socket).GetSafeNormal();
	const double A = FVector::Dist(Rest[Leg.Upper].GetLocation(), Rest[Leg.Lower].GetLocation());
	const double B = FVector::Dist(Rest[Leg.Lower].GetLocation(), Rest[Leg.Pastern].GetLocation());
	const double Length = FMath::Clamp(FVector::Dist(Ankle, Socket), FMath::Abs(A-B)+.01, A+B-.01);
	const double Along = (A*A - B*B + Length*Length) / (2.*Length);
	const FVector Flex = FVector(Axis.Z, 0, -Axis.X).GetSafeNormal() * (Leg.bFore ? 1. : -1.);
	const FVector Knee = Socket + Axis*Along + Flex*FMath::Sqrt(FMath::Max(0., A*A - Along*Along));
	AimAt(Leg.Upper, Leg.Lower, Knee);
	AimAt(Leg.Lower, Leg.Pastern, Ankle);
	AimAt(Leg.Pastern, Leg.Paw, Foot);
	Inherit(Leg.Paw);
	Posed[Leg.Paw].SetRotation(FRotator(PawPitch,0,0).Quaternion() * Rest[Leg.Paw].GetRotation());
}

void UWolfMeshComponent::FinishPose()
{
	const auto& Skeleton = GetSkinnedAsset()->GetRefSkeleton();
	for (int32 I=0; I<Posed.Num(); ++I)
	{
		const int32 Parent = Skeleton.GetParentIndex(I);
		BoneSpaceTransforms[I] = Parent < 0 ? Posed[I] : Posed[I].GetRelativeTransform(Posed[Parent]);
	}
	RefreshBoneTransforms();
}

void UWolfMeshComponent::AdvanceGait(float Dt, float Travel, bool bSwimming, float Threat)
{
	if (!PrepareRig()) return;
	Clock += Dt;
	const float Scale = FMath::Max(.1f, float(GetComponentScale().X));
	const float Measured = Dt > SMALL_NUMBER ? Travel / (Dt * Scale) : 0.f;
	Speed = FMath::FInterpTo(Speed, FMath::Min(Measured,750.f), Dt, 12.f);
	const float Activity = FMath::Clamp(Speed / 75.f, 0.f, 1.f);
	const float Span = FMath::Clamp(13.f + .047f*Speed, 13.f, 37.f);
	constexpr float Contact = .56f;
	const float Hz = bSwimming ? 1.1f : FMath::Min(Speed*Contact/(2.f*Span), 4.4f);
	Phase = FMath::Fmod(Phase + Dt*Hz, 1.f);
	const float Beat = FMath::Cos(4*PI*(Phase-.20f));
	const float Breath = FMath::Sin(Clock*2.5f);
	Posed = Rest;
	Tilt(Chest, FRotator(-2.f*Activity*Beat - 3.f*Threat, 0, 0));
	Posed[Chest].AddToTranslation(FVector(0,0, Activity*(.9f - 2.6f*Beat) + .5f*Breath - 3.f*Threat));
	Tilt(Neck, FRotator(1.5f*Activity*Beat + 2.f*Threat + (bSwimming?11.f:0.f), 0, 0));
	Tilt(Skull, FRotator(Breath*.8f + Activity*Beat, FMath::Sin(Clock*1.7f)*1.4f, 0));
	Tilt(Jaw, FRotator(-2.f - 6.f*Threat - .9f*Breath, 0, 0));
	Tilt(LeftEar, FRotator(-6.f*Threat, 3.f*FMath::Sin(Clock*3.1f), 0));
	Tilt(RightEar, FRotator(-6.f*Threat, -2.f*FMath::Cos(Clock*2.8f), 0));
	Tilt(Brush, FRotator(-8.f*Activity - 7.f*Threat, 6.f*FMath::Sin(Clock*2.8f), 0));
	Tilt(Tip, FRotator(2.f*Beat*Activity, 8.f*FMath::Sin(Clock*2.8f-.6f), 0));

	for (const FLeg& Leg : Legs)
	{
		// Diagonal pairs share a contact beat. During stance the foot moves backwards relative to
		// the body; through swing the elbow or stifle flexes to carry the toes clear of the soil.
		const float P = FMath::Fmod(Phase + (Leg.bSecondBeat?.5f:0.f), 1.f);
		const bool bStance = P < Contact;
		const float T = bStance ? P/Contact : (P-Contact)/(1.f-Contact);
		const float Lift = bStance ? 0.f : FMath::Square(FMath::Sin(PI*T));
		const float StepX = bStance ? 1.f-2.f*T : -FMath::Cos(PI*T);
		FVector Foot = Rest[Leg.Paw].GetLocation();
		Foot.X += Activity*Span*StepX;
		Foot.Z += Activity*Lift*(Leg.bFore?16.f:19.f);
		if (bSwimming)
		{
			Foot.X += 7.f*FMath::Sin(2*PI*P);
			Foot.Z += 12.f + 8.f*FMath::Cos(2*PI*P);
		}
		Reach(Leg, Foot, -15.f*Lift*Activity);
	}
	FinishPose();
}

void UWolfMeshComponent::SetPouncePose(float Progress)
{
	if (!PrepareRig()) return;
	const float Launch = FMath::SmoothStep(0.f,.50f,Progress);
	const float Impact = FMath::SmoothStep(.68f,1.f,Progress);
	Posed = Rest;
	Tilt(Chest, FRotator(FMath::Lerp(8.f,3.f,Launch) - 5.f*Impact,0,0));
	Posed[Chest].AddToTranslation(FVector(0,0,FMath::Lerp(-17.f,3.f,Launch) - 3.f*Impact));
	Tilt(Neck, FRotator(4.f - 8.f*Launch,0,0));
	Tilt(Skull, FRotator(2.f + 3.f*Launch,0,0));
	Tilt(Jaw, FRotator(-9.f - 19.f*Launch + 8.f*Impact,0,0));
	Tilt(LeftEar, FRotator(-24.f,7,0)); Tilt(RightEar, FRotator(-24.f,-7,0));
	Tilt(Brush, FRotator(-11.f - 23.f*Launch,0,0));
	Tilt(Tip, FRotator(-9.f - 10.f*Launch,0,0));
	for (const FLeg& Leg : Legs)
	{
		FVector Foot = Rest[Leg.Paw].GetLocation();
		Foot.X += Leg.bFore ? FMath::Lerp(-4.f,42.f,Launch) - 5.f*Impact : FMath::Lerp(11.f,-24.f,Launch) + 16.f*Impact;
		Foot.Z += Leg.bFore ? Launch*(38.f + (Leg.bSecondBeat?2.f:0.f)) : Launch*(15.f + 9.f*Impact);
		Reach(Leg,Foot,Leg.bFore ? (18.f*Launch - 10.f*Impact) : -33.f*Launch);
	}
	FinishPose();
}
