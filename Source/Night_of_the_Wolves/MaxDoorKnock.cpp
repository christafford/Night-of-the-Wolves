#include "MaxDoorKnock.h"
#include "WolfHeldItem.h"
#include "Night_of_the_WolvesCharacter.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TwoBoneIK.h"

namespace
{
constexpr auto KnockWorld = EBoneSpaces::WorldSpace;

void RotateKnockBone(UPoseableMeshComponent& Pose, FName Bone, FVector Axis, float Degrees)
{
	const FQuat Original = Pose.GetBoneTransformByName(Bone, KnockWorld).GetRotation();
	Pose.SetBoneRotationByName(Bone, (FQuat(Axis, FMath::DegreesToRadians(Degrees)) * Original).Rotator(), KnockWorld);
}

struct FFistBones
{
	FName Upper, Lower, Hand, Knuckle;
	FName Fingers[4][3];
	FName Thumb;
};

const FFistBones RightFist{
	TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r"), TEXT("middle_02_r"),
	{{TEXT("index_01_r"), TEXT("index_02_r"), TEXT("index_03_r")},
	 {TEXT("middle_01_r"), TEXT("middle_02_r"), TEXT("middle_03_r")},
	 {TEXT("ring_01_r"), TEXT("ring_02_r"), TEXT("ring_03_r")},
	 {TEXT("pinky_01_r"), TEXT("pinky_02_r"), TEXT("pinky_03_r")}},
	TEXT("thumb_02_r")
};
const FFistBones LeftFist{
	TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l"), TEXT("middle_02_l"),
	{{TEXT("index_01_l"), TEXT("index_02_l"), TEXT("index_03_l")},
	 {TEXT("middle_01_l"), TEXT("middle_02_l"), TEXT("middle_03_l")},
	 {TEXT("ring_01_l"), TEXT("ring_02_l"), TEXT("ring_03_l")},
	 {TEXT("pinky_01_l"), TEXT("pinky_02_l"), TEXT("pinky_03_l")}},
	TEXT("thumb_02_l")
};

void PoseFist(UPoseableMeshComponent& Pose, const FFistBones& Bones, const FVector& Inward,
	const FVector& Right, double Sign, const FVector& KnuckleGoal, float Weight)
{
	const FTransform Wrist = Pose.GetBoneTransformByName(Bones.Hand, KnockWorld);
	const FVector Along = Pose.GetBoneLocationByName(Bones.Fingers[1][0], KnockWorld) - Wrist.GetLocation();
	const FVector Across = Pose.GetBoneLocationByName(Bones.Fingers[3][0], KnockWorld)
		- Pose.GetBoneLocationByName(Bones.Fingers[0][0], KnockWorld);
	if (Along.IsNearlyZero() || Across.IsNearlyZero()) return;

	// Curl the fingers toward the palm before measuring the bent knuckle's wrist offset.
	const FVector CurlAxis = Across.GetSafeNormal() * Sign;
	constexpr float Curl[] = {52.f, 62.f, 33.f};
	for (const auto& Finger : Bones.Fingers)
	{
		for (int32 Joint = 0; Joint < UE_ARRAY_COUNT(Curl); ++Joint)
			RotateKnockBone(Pose, Finger[Joint], CurlAxis, Curl[Joint] * Weight);
	}
	RotateKnockBone(Pose, Bones.Thumb, Along.GetSafeNormal() * Sign, 24.f * Weight);

	const FVector Up = FVector::UpVector;
	const FQuat OldPalm = FRotationMatrix::MakeFromXY(Along, Across).ToQuat();
	const FQuat NewPalm = FRotationMatrix::MakeFromXY(Inward + .23 * Up, Right * Sign - .55 * Up).ToQuat();
	const FQuat WristRotation = FQuat::Slerp(Wrist.GetRotation(), NewPalm * OldPalm.Inverse() * Wrist.GetRotation(), Weight);
	Pose.SetBoneRotationByName(Bones.Hand, WristRotation.Rotator(), KnockWorld);
	const FVector KnuckleOffset = Pose.GetBoneLocationByName(Bones.Knuckle, KnockWorld) - Wrist.GetLocation();

	FTransform Upper = Pose.GetBoneTransformByName(Bones.Upper, KnockWorld);
	FTransform Lower = Pose.GetBoneTransformByName(Bones.Lower, KnockWorld);
	FTransform End = Wrist;
	const FQuat UpperRest = Upper.GetRotation();
	const FQuat LowerRest = Lower.GetRotation();
	const FVector ElbowPole = Upper.GetLocation() + Right * Sign * 34. - Inward * 18. - Up * 31.;
	AnimationCore::SolveTwoBoneIK(Upper, Lower, End, ElbowPole, KnuckleGoal - KnuckleOffset, false, 1., 1.);
	Pose.SetBoneRotationByName(Bones.Upper, FQuat::Slerp(UpperRest, Upper.GetRotation(), Weight).Rotator(), KnockWorld);
	Pose.SetBoneRotationByName(Bones.Lower, FQuat::Slerp(LowerRest, Lower.GetRotation(), Weight).Rotator(), KnockWorld);
	Pose.SetBoneRotationByName(Bones.Hand, WristRotation.Rotator(), KnockWorld);
}
}

UPoseableMeshComponent* ANight_of_the_WolvesCharacter::BeginWorkingPose()
{
	USkeletalMeshComponent* Original = GetMesh();
	if (!Original->GetSkinnedAsset()) return nullptr;
	auto* Working = NewObject<UPoseableMeshComponent>(this);
	Working->SetupAttachment(Original);
	Working->SetSkinnedAssetAndUpdate(Original->GetSkinnedAsset());
	Working->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Working->SetCanEverAffectNavigation(false);
	Working->RegisterComponent();
	for (int32 Slot = 0; Slot < Original->GetNumMaterials(); ++Slot)
		Working->SetMaterial(Slot, Original->GetMaterial(Slot));
	bWorkingBodyWasVisible = Original->IsVisible();
	WorkingBodyTick = Original->VisibilityBasedAnimTickOption;
	Original->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	BeginDigAppearance(Working);
	if (auto* Item = FindComponentByClass<UWolfHeldItem>())
		Item->AttachToComponent(Working, FAttachmentTransformRules::KeepRelativeTransform, TEXT("hand_r"));
	Original->SetVisibility(false);
	return Working;
}

void ANight_of_the_WolvesCharacter::EndWorkingPose(UPoseableMeshComponent* Pose)
{
	if (!IsValid(Pose)) return;
	EndDigAppearance();
	if (auto* Item = FindComponentByClass<UWolfHeldItem>())
		Item->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, TEXT("hand_r"));
	GetMesh()->SetVisibility(bWorkingBodyWasVisible);
	GetMesh()->VisibilityBasedAnimTickOption = WorkingBodyTick;
	Pose->DestroyComponent();
}

void ANight_of_the_WolvesCharacter::PoseDoorKnock(const FVector& Contact, const FVector& OutwardNormal, float Fraction)
{
	if (!KnockBody) KnockBody = BeginWorkingPose();
	if (!KnockBody) return;
	KnockBody->CopyPoseFromSkeletalComponent(GetMesh());
	const FVector Inward = (-OutwardNormal).GetSafeNormal2D(UE_SMALL_NUMBER, GetActorForwardVector());
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Inward);
	if (KnockStance.IsEmpty())
	{
		bKnockLeft = FVector::DotProduct(Contact - GetActorLocation(), Right) < -6.;
		if (Fraction >= .25f) KnockStance = KnockBody->BoneSpaceTransforms;
	}
	// Let Max settle during the approach, then hold his stance through the three raps.
	// Blend back onto live locomotion when he lowers his arm.
	if (KnockStance.Num() == KnockBody->BoneSpaceTransforms.Num())
	{
		const float Release = FMath::SmoothStep(.84f, 1.f, Fraction);
		for (int32 I = 0; I < KnockStance.Num(); ++I)
			KnockBody->BoneSpaceTransforms[I].BlendWith(KnockStance[I], 1.f - Release);
	}
	const float Weight = FMath::SmoothStep(0.f, .30f, Fraction) * (1.f - FMath::SmoothStep(.81f, 1.f, Fraction));
	float Strike = 0.f;
	for (float Hit : MaxDoorKnock::Hits)
	{
		const float Reach = FMath::SmoothStep(Hit - .078f, Hit, Fraction);
		const float Rebound = 1.f - FMath::SmoothStep(Hit + .008f, Hit + .086f, Fraction);
		Strike = FMath::Max(Strike, Reach * Rebound);
	}
	const double Sign = bKnockLeft ? -1. : 1.;
	RotateKnockBone(*KnockBody, TEXT("spine_01"), Right, (12.f + 2.f * Strike) * Weight);
	RotateKnockBone(*KnockBody, TEXT("spine_03"), Right, 6.f * Weight);
	RotateKnockBone(*KnockBody, TEXT("spine_03"), FVector::UpVector, -Sign * 7.f * Weight);
	RotateKnockBone(*KnockBody, TEXT("neck_01"), Right, -7.f * Weight);
	const FVector Target = Contact - Inward * FMath::Lerp(17., 1.15, double(Strike))
		+ FVector::UpVector * (3.5 * (1. - Strike));
	PoseFist(*KnockBody, bKnockLeft ? LeftFist : RightFist, Inward, Right, Sign, Target, Weight);
	KnockBody->RefreshBoneTransforms();
	FollowDigHead(KnockBody->GetSocketTransform(TEXT("head")));
}

FVector ANight_of_the_WolvesCharacter::KnockingKnuckle() const
{
	return KnockBody ? KnockBody->GetSocketLocation(bKnockLeft ? LeftFist.Knuckle : RightFist.Knuckle) : FVector::ZeroVector;
}

void ANight_of_the_WolvesCharacter::EndDoorKnock()
{
	if (!KnockBody) return;
	EndWorkingPose(KnockBody);
	KnockBody = nullptr;
	KnockStance.Reset();
}
