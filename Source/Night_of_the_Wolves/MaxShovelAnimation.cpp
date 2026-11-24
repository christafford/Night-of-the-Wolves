#include "WolfHeldItem.h"
#include "MaxDigStroke.h"
#include "Night_of_the_WolvesCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TwoBoneIK.h"

namespace
{
struct FSpadeKey
{
	float Phase;
	FVector Blade; // cm above Max's stance, forward / right / up
	FVector Shaft;
	float HipDrop, Lean, Twist;
};

// Load the tool, stamp down, rock the handle to break the cover, then throw to the left.
const FSpadeKey Stroke[] = {
	{0.f,  {46,-12,33}, {-.23,.21,.95},   6, 14, -5},
	{.18f, {52,-13,47}, {-.21,.17,.97},   4, 10, -8},
	{MaxDigStroke::Plant, {55,-15,-5}, {-.27,.23,.94}, 19, 40, -9},
	{.46f, {57,-13,-4}, {-.58,.36,.73},  27, 56,  3},
	{.59f, {65,-29,39}, {-.51,.53,.68},  16, 35, -6},
	{.73f, {50,-68,79}, {-.34,.87,.36},   7, 17,-29},
	{.84f, {45,-70,83}, {-.29,.91,.28},   6, 15,-32},
	{1.f,  {46,-12,33}, {-.23,.21,.95},   6, 14, -5}
};

FSpadeKey SampleStroke(float Phase)
{
	int32 I = 1;
	while (I < UE_ARRAY_COUNT(Stroke)-1 && Phase>Stroke[I].Phase) ++I;
	const auto& A = Stroke[I-1];
	const auto& B = Stroke[I];
	const float Alpha = FMath::SmoothStep(A.Phase,B.Phase,Phase);
	return {Phase, FMath::Lerp(A.Blade,B.Blade,double(Alpha)), FMath::Lerp(A.Shaft,B.Shaft,double(Alpha)).GetSafeNormal(),
		FMath::Lerp(A.HipDrop,B.HipDrop,Alpha), FMath::Lerp(A.Lean,B.Lean,Alpha), FMath::Lerp(A.Twist,B.Twist,Alpha)};
}

constexpr auto World = EBoneSpaces::WorldSpace;

void PositionLimb(UPoseableMeshComponent& Body, FName Upper, FName Lower, FName End, const FVector& Goal, const FVector& Pole, float Weight)
{
	FTransform A=Body.GetBoneTransformByName(Upper,World);
	FTransform B=Body.GetBoneTransformByName(Lower,World);
	FTransform C=Body.GetBoneTransformByName(End,World);
	AnimationCore::SolveTwoBoneIK(A,B,C,Pole,FMath::Lerp(C.GetLocation(),Goal,double(Weight)),false,1.,1.);
	Body.SetBoneRotationByName(Upper,A.Rotator(),World);
	Body.SetBoneRotationByName(Lower,B.Rotator(),World);
}
}

void UWolfHeldItem::UpdateSpadePose(ANight_of_the_WolvesCharacter& Max, float Fraction)
{
	auto* Body = Max.GetMesh();
	if (!DigBody)
	{
		DigBody = NewObject<UPoseableMeshComponent>(&Max);
		DigBody->SetupAttachment(Body);
		DigBody->SetSkinnedAssetAndUpdate(Body->GetSkinnedAsset());
		DigBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DigBody->SetCanEverAffectNavigation(false);
		DigBody->RegisterComponent();
		for (int32 I=0; I<Body->GetNumMaterials(); ++I) DigBody->SetMaterial(I, Body->GetMaterial(I));
		Max.BeginDigAppearance(DigBody);
		Body->SetVisibility(false);
		for (int32 I=0; ClodMesh && I<6; ++I)
		{
			auto* Clod = NewObject<UStaticMeshComponent>(&Max);
			Clod->SetupAttachment(this);
			Clod->SetStaticMesh(ClodMesh);
			for (int32 Slot=0; Slot<Clod->GetNumMaterials(); ++Slot) Clod->SetMaterial(Slot,Soil);
			Clod->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Clod->SetCanEverAffectNavigation(false);
			Clod->SetVisibility(false);
			Clod->RegisterComponent();
			Clods.Add(Clod);
		}
	}
	DigBody->CopyPoseFromSkeletalComponent(Body);
	const float Weight = FMath::SmoothStep(0.f,.10f,Fraction)*(1.f-FMath::SmoothStep(.925f,1.f,Fraction));
	const float Cycle = MaxDigStroke::Cycle(Fraction);
	const float Phase = FMath::Frac(Cycle);
	const FSpadeKey Key = SampleStroke(Phase);
	const FVector Forward = Max.GetActorForwardVector();
	const FVector Side = Max.GetActorRightVector();
	const FVector Up = FVector::UpVector;
	FVector Feet = Max.GetActorLocation();
	const FFindFloorResult& Ground = Max.GetCharacterMovement()->CurrentFloor;
	Feet.Z = Ground.IsWalkableFloor() ? Ground.HitResult.ImpactPoint.Z : Feet.Z-Max.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FTransform Stance(Max.GetActorQuat(),Feet);
	const float Cut = (FMath::Min(2,FMath::FloorToInt(Cycle))-1)*6.f;
	const FVector Blade = Stance.TransformPosition(Key.Blade+FVector(0,Cut,0));
	const FVector Shaft = Stance.TransformVectorNoScale(Key.Shaft);
	const FBox Bounds = Held->GetStaticMesh()->GetBoundingBox();
	const double Tip = -Bounds.Min.Z;
	const FQuat BladeRotation = Bounds.GetSize().X > Bounds.GetSize().Y ?
		FRotationMatrix::MakeFromZX(Shaft,Side).ToQuat() : FRotationMatrix::MakeFromZY(Shaft,Side).ToQuat();
	const FTransform WorkingTool(BladeRotation,Blade+Shaft*Tip);
	FTransform Tool;
	Tool.Blend(GetComponentTransform(),WorkingTool,Weight);
	Held->SetWorldTransform(Tool);

	// Keep the soles planted as the pelvis lowers and the chest turns over the tool.
	const FVector LeftBoot = DigBody->GetBoneLocationByName(TEXT("foot_l"), World);
	const FVector RightBoot = DigBody->GetBoneLocationByName(TEXT("foot_r"), World);
	const FVector Hips = DigBody->GetBoneLocationByName(TEXT("pelvis"), World);
	DigBody->SetBoneLocationByName(TEXT("pelvis"),Hips + Weight*(Forward*(Key.HipDrop*.17f)-Up*Key.HipDrop),World);
	const FQuat Spine = DigBody->GetBoneTransformByName(TEXT("spine_01"), World).GetRotation();
	const FQuat Bend(Side,FMath::DegreesToRadians(Key.Lean*Weight));
	const FQuat Twist(Up,FMath::DegreesToRadians(Key.Twist*Weight));
	DigBody->SetBoneRotationByName(TEXT("spine_01"),(Twist*Bend*Spine).Rotator(),World);

	for (bool bLeft : {true,false})
	{
		const float Sign = bLeft ? -1.f : 1.f;
		const FVector Knee = Feet + Forward*54.f + Side*Sign*21.f + Up*43.f;
		PositionLimb(*DigBody,bLeft ? TEXT("thigh_l") : TEXT("thigh_r"),bLeft ? TEXT("calf_l") : TEXT("calf_r"),
			bLeft ? TEXT("foot_l") : TEXT("foot_r"),bLeft ? LeftBoot : RightBoot,Knee,1.f);
		const FName Hand = bLeft ? TEXT("hand_l") : TEXT("hand_r");
		const FVector Palm = WorkingTool.TransformPosition(FVector(0,0,bLeft ? -2. : Bounds.Max.Z-4.));
		const FVector Wrist = Palm + Side*Sign*6. - Forward*2.7;
		PositionLimb(*DigBody,bLeft ? TEXT("upperarm_l") : TEXT("upperarm_r"),bLeft ? TEXT("lowerarm_l") : TEXT("lowerarm_r"),
			Hand,Wrist,Feet + Side*Sign*57.f - Forward*16.f + Up*105.f,Weight);
		const FTransform PalmBone = DigBody->GetBoneTransformByName(Hand,World);
		const FVector Knuckle = DigBody->GetBoneLocationByName(bLeft ? TEXT("middle_01_l") : TEXT("middle_01_r"),World);
		const FVector AlongPalm = (Knuckle-PalmBone.GetLocation()).GetSafeNormal();
		const FQuat Close = FQuat::FindBetweenNormals(AlongPalm,(Palm-Wrist).GetSafeNormal());
		DigBody->SetBoneRotationByName(Hand,(FQuat::Slerp(FQuat::Identity,Close,Weight)*PalmBone.GetRotation()).Rotator(),World);
	}
	DigBody->RefreshBoneTransforms();
	Max.FollowDigHead(DigBody->GetSocketTransform(TEXT("head")));

	// Clods ride the blade until the upward sweep releases them. Each follows a
	// shorter or longer ballistic arc before disappearing into the leaf litter.
	constexpr float Release = .71f;
	const FVector Load = Blade + Shaft*8.f + Up*4.f;
	const FSpadeKey ReleaseKey = SampleStroke(Release);
	const FVector Launch = Stance.TransformPosition(ReleaseKey.Blade+FVector(0,Cut,0) + ReleaseKey.Shaft*8.f) + Up*4.f;
	for (int32 I=0; I<Clods.Num(); ++I)
	{
		const float Time = (Phase-Release)*2.02f;
		const FVector Scatter = Side*((I*7 % 13)-6.) + Forward*((I*3 % 9)-4.) + Up*(I%3);
		const FVector Velocity = -Side*(128.+7.*I) + Forward*(35.-15.*I) + Up*(35.+I*8.);
		const FVector P = (Time<0 ? Load : Launch + Velocity*Time - Up*(490.*Time*Time)) + Scatter;
		UStaticMeshComponent* Clod = Clods[I];
		Clod->SetVisibility(Phase>.35f && P.Z>Feet.Z-2.);
		if (!Clod->IsVisible()) continue;
		const double Radius = 1.7 + (I*3 % 5)*.67;
		const double Scale = Radius/FMath::Max(1.,ClodMesh->GetBounds().BoxExtent.GetMax());
		Clod->SetWorldLocationAndRotation(P,FRotator(I*49.+Time*170.,I*81.+Time*105.,I*92.));
		Clod->SetWorldScale3D(FVector(Scale));
	}
}

void UWolfHeldItem::EndDigPose()
{
	auto* Max = CastChecked<ANight_of_the_WolvesCharacter>(GetOwner());
	Max->EndDigAppearance();
	Max->GetMesh()->SetVisibility(true);
	DigBody->DestroyComponent();
	DigBody = nullptr;
	Held->SetRelativeTransform(FTransform::Identity);
	for (UStaticMeshComponent* Clod : Clods) Clod->DestroyComponent();
	Clods.Empty();
}
