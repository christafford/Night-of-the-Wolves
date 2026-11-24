#include "DialogueFaceAnimInstance.h"

#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_CopyPoseFromMesh.h"
#include "DialogueArticulation.h"

namespace
{
enum class EFacialJoint : uint8
{
	None, Jaw, UpperLip, LowerLip, Corner, Brow, UpperLid, LowerLid, Eye, Cheek, UnderChin
};

struct FFaceJoint
{
	FCompactPoseBoneIndex Index;
	EFacialJoint Part;
	float Lateral;
	float Influence;
	FQuat ParentBindRotation;
	FVector HingeAxis;
};

EFacialJoint IdentifyJoint(const FString& Name, const FName Parent)
{
	static const FName Root(TEXT("FACIAL_C_FacialRoot"));
	static const FName LowerRotation(TEXT("FACIAL_C_LowerLipRotation"));
	static const FName MouthUpper(TEXT("FACIAL_C_MouthUpper"));
	static const FName MouthLower(TEXT("FACIAL_C_MouthLower"));
	if (Name == TEXT("FACIAL_C_Jaw") || Name == TEXT("FACIAL_C_LowerLipRotation"))
		return EFacialJoint::Jaw;
	if (Parent == MouthUpper)
		return Name.Contains(TEXT("Corner")) ? EFacialJoint::Corner : EFacialJoint::UpperLip;
	if (Parent == MouthLower)
		return EFacialJoint::LowerLip;
	if (Parent == LowerRotation && Name.Contains(TEXT("LipLower")))
		return EFacialJoint::LowerLip;
	if (Parent != Root)
		return EFacialJoint::None;
	if (Name.Contains(TEXT("LipUpper"))) return EFacialJoint::UpperLip;
	if (Name.Contains(TEXT("Forehead"))) return EFacialJoint::Brow;
	if (Name.EndsWith(TEXT("_Eye"))) return EFacialJoint::Eye;
	if (Name.EndsWith(TEXT("EyelidUpperA")) || Name.EndsWith(TEXT("EyelidUpperB"))) return EFacialJoint::UpperLid;
	if (Name.EndsWith(TEXT("EyelidLowerA")) || Name.EndsWith(TEXT("EyelidLowerB"))) return EFacialJoint::LowerLid;
	if (Name.Contains(TEXT("UnderChin"))) return EFacialJoint::UnderChin;
	if (Name.Contains(TEXT("Cheek")) || Name.Contains(TEXT("Nasolabial"))) return EFacialJoint::Cheek;
	return EFacialJoint::None;
}
}

// The exported Creator heads contain the facial joint weights and have no post-process rig.
// Pose the joint groups in the sculpt's reference axes, then let the bone hierarchy carry
// eyelids, teeth, tongue, lip detail and skinned whiskers with them.
struct FDialogueFaceProxy final : public FAnimInstanceProxy
{
	explicit FDialogueFaceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance)
	{
		BodyPose.bUseAttachedParent = true;
	}

	FAnimNode_Base* GetCustomRootNode() override { return &BodyPose; }
	void GetCustomNodes(TArray<FAnimNode_Base*>& Nodes) override { Nodes.Add(&BodyPose); }

	void PreUpdate(UAnimInstance* Instance, float Delta) override
	{
		FAnimInstanceProxy::PreUpdate(Instance, Delta);
		const auto& Face = *CastChecked<UDialogueFaceAnimInstance>(Instance);
		Mouth = { Face.JawOpen, Face.LipRound, Face.LipWide, Face.LipClose, Face.LipTeeth };
		Anger = Face.Scowl;
		Interest = Face.Interest;
		LookDown = Face.DownwardGaze;
		Blink = Face.BlinkAmount;
		Fear = Face.Alarm;
	}

	void CacheFace(const FBoneContainer& Bones)
	{
		Joints.Reset();
		const FReferenceSkeleton& Ref = Bones.GetReferenceSkeleton();
		TArray<FTransform> Bind;
		Ref.GetBoneAbsoluteTransforms(Bind);
		const TArray<FBoneIndexType>& Required = Bones.GetBoneIndicesArray();
		for (int32 Compact = 0; Compact < Required.Num(); ++Compact)
		{
			const int32 MeshIndex = Required[Compact];
			const int32 Parent = Ref.GetParentIndex(MeshIndex);
			if (Parent == INDEX_NONE) continue;
			const FString Name = Ref.GetBoneName(MeshIndex).ToString();
			const EFacialJoint Part = IdentifyJoint(Name, Ref.GetBoneName(Parent));
			if (Part == EFacialJoint::None) continue;
			// Positive X is the sculpt's left, positive Y is forward and Z is up.
			const float Side = Bind[MeshIndex].GetLocation().X;
			const float Weight = Name.Contains(TEXT("Skin")) ? .60f : 1.f;
			Joints.Add({FCompactPoseBoneIndex(Compact), Part, Side, Weight, Bind[Parent].GetRotation(),
				Bind[MeshIndex].InverseTransformVectorNoScale(FVector::XAxisVector)});
		}
		BoneSerial = Bones.GetSerialNumber();
	}

	bool Evaluate(FPoseContext& Output) override
	{
		BodyPose.Evaluate_AnyThread(Output);
		const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
		if (BoneSerial != Bones.GetSerialNumber()) CacheFace(Bones);

		const float Open = FMath::Max(Mouth.JawOpen, .50f * Fear) * (1.f - Mouth.LipClose) * (1.f - .55f * Mouth.LipTeeth);
		for (const FFaceJoint& Joint : Joints)
		{
			FVector Offset = FVector::ZeroVector;
			float Angle = 0.f;
			const float Side = FMath::Clamp(Joint.Lateral / 2.4f, -1.f, 1.f);
			const float Outer = FMath::Abs(Side);
			switch (Joint.Part)
			{
			case EFacialJoint::Jaw:
				// The jaw hinge and lower-lip/teeth hinge are sibling controls.
				Angle = -22.f * Open;
				break;
			case EFacialJoint::UpperLip:
			case EFacialJoint::LowerLip:
			{
				const bool bUpper = Joint.Part == EFacialJoint::UpperLip;
				Offset.X = Side * (.76f * Mouth.LipWide - 1.04f * Mouth.LipRound);
				Offset.Y = (.88f - .22f * Outer) * Mouth.LipRound - .12f * Mouth.LipWide;
				Offset.Z = bUpper ? (.34f * Open - .22f * Mouth.LipClose + .23f * Mouth.LipTeeth)
					: (.23f * Mouth.LipClose + .51f * Mouth.LipTeeth);
				if (!bUpper) Offset.Y -= .54f * Mouth.LipTeeth;
				Offset *= Joint.Influence;
				break;
			}
			case EFacialJoint::Corner:
				Offset.X = Side * (.92f * Mouth.LipWide - 1.03f * Mouth.LipRound + .27f * Interest + .42f * Fear);
				Offset.Y = .63f * Mouth.LipRound - .29f * Mouth.LipWide - .33f * Interest;
				Offset.Z = -.72f * Open + (.36f + .16f * Side) * Interest - .28f * Anger - .42f * Fear;
				break;
			case EFacialJoint::Brow:
			{
				const float Inner = 1.f - FMath::Clamp(FMath::Abs(Joint.Lateral) / 5.f, 0.f, 1.f);
				Offset.X = -.44f * Side * Anger * Inner;
				Offset.Z = -Anger * (.14f + .78f * Inner) + Interest * (.27f + .41f * Inner + .16f * Side);
				Offset.Z += (.68f + .85f * Inner) * Fear;
				Offset.X -= .26f * Side * Fear * Inner;
				Offset *= Joint.Influence;
				break;
			}
			case EFacialJoint::Eye: Angle = -7.f * LookDown; break;
			case EFacialJoint::UpperLid: Angle = -33.f * Blink - 6.f * Anger - 4.f * LookDown + 1.5f * Interest + 10.f * Fear; break;
			case EFacialJoint::LowerLid: Angle = 7.f * Blink + 3.f * Anger + .75f * Interest; break;
			case EFacialJoint::Cheek:
				Offset = FVector(Side * (.18f * Mouth.LipWide - .19f * Mouth.LipRound), -.12f * Interest,
					.29f * Interest - .25f * Open);
				break;
			case EFacialJoint::UnderChin: Offset.Z = -.75f * Open; break;
			default: break;
			}
			FTransform& Bone = Output.Pose[Joint.Index];
			Bone.AddToTranslation(Joint.ParentBindRotation.UnrotateVector(Offset));
			if (Angle != 0.f)
			{
				Bone.SetRotation((Bone.GetRotation() * FQuat(Joint.HingeAxis, FMath::DegreesToRadians(Angle))).GetNormalized());
			}
		}
		return true;
	}

	FAnimNode_CopyPoseFromMesh BodyPose;
	TArray<FFaceJoint> Joints;
	int32 BoneSerial = INDEX_NONE;
	FDialogueMouthPose Mouth;
	float Anger = 0.f, Interest = 0.f, LookDown = 0.f, Blink = 0.f, Fear = 0.f;
};

FAnimInstanceProxy* UDialogueFaceAnimInstance::CreateAnimInstanceProxy() { return new FDialogueFaceProxy(this); }
void UDialogueFaceAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) { delete InProxy; }

void UDialogueFaceAnimInstance::SetPerformance(const FDialogueMouthPose& Mouth, float Anger, float GoldInterest, float LookDown, float Blink, float Fear)
{
	JawOpen = Mouth.JawOpen;
	LipRound = Mouth.LipRound;
	LipWide = Mouth.LipWide;
	LipClose = Mouth.LipClose;
	LipTeeth = Mouth.LipTeeth;
	Scowl = Anger;
	Interest = GoldInterest;
	DownwardGaze = LookDown;
	BlinkAmount = Blink;
	Alarm = Fear;
}
