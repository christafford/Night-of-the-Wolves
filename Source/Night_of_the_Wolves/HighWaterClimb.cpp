#include "HighWaterChamber.h"
#include "HighWaterLayout.h"
#include "Night_of_the_WolvesCharacter.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "TwoBoneIK.h"


namespace
{
const FVector RungFace(double Height)
{
	// Coordinates relative to the deck. The paired rails lean against the ledge.
	return FVector(82. + .264 * Height, 0, Height);
}

double GripRung(double PreferredHeight, bool bOdd)
{
	constexpr double First = 20.5, Pitch = 29.;
	const double Origin = First + (bOdd ? Pitch : 0.);
	const double Pair = (PreferredHeight - Origin) / (2. * Pitch);
	const double Rest = FMath::FloorToDouble(Pair);
	const double Reach = FMath::SmoothStep(.23, .83, Pair - Rest);
	return FMath::Clamp(Origin + (Rest + Reach) * (2. * Pitch), First, First + 8. * Pitch);
}

void LadderLimb(UPoseableMeshComponent& Pose, FName Upper, FName Lower, FName End, const FVector& Goal,
	const FVector& Pole, float Weight)
{
	constexpr auto Space = EBoneSpaces::WorldSpace;
	FTransform A = Pose.GetBoneTransformByName(Upper, Space);
	FTransform B = Pose.GetBoneTransformByName(Lower, Space);
	FTransform C = Pose.GetBoneTransformByName(End, Space);
	const FQuat StartA = A.GetRotation(), StartB = B.GetRotation();
	AnimationCore::SolveTwoBoneIK(A, B, C, Pole, Goal, false, 1., 1.);
	Pose.SetBoneRotationByName(Upper, FQuat::Slerp(StartA, A.GetRotation(), Weight).Rotator(), Space);
	Pose.SetBoneRotationByName(Lower, FQuat::Slerp(StartB, B.GetRotation(), Weight).Rotator(), Space);
}
}

void AHighWaterChamber::BeginTraversal(ANight_of_the_WolvesCharacter& Max)
{
	EndRefillView(.30f);
	TraversalMax = &Max;
	StartFeet = StormfallLayout::LocalPoint(Max.GetActorLocation() - FVector::UpVector * Max.GetSimpleCollisionHalfHeight());
	bAtLanding = bTookRisenTorch = false;
	Max.StopJumping();
	Max.ConsumeMovementInputVector();
	Max.GetCharacterMovement()->StopMovementImmediately();
	Max.GetCharacterMovement()->DisableMovement();
	TraversalBody = Max.BeginWorkingPose();
}

void AHighWaterChamber::StartClimb(ANight_of_the_WolvesCharacter& Max, bool bUp)
{
	BeginTraversal(Max);
	ClimbTime = 0.f;
	bClimbingUp = bUp;
}

void AHighWaterChamber::PoseClimb(ANight_of_the_WolvesCharacter& Max, float Ascent, float Weight)
{
	using namespace HighWaterLayout;
	if (!TraversalBody) return;
	TraversalBody->CopyPoseFromSkeletalComponent(Max.GetMesh());
	const FVector Feet = StormfallLayout::LocalPoint(Max.GetActorLocation() - FVector::UpVector * Max.GetSimpleCollisionHalfHeight());
	for (bool bLeft : {false, true})
	{
		const double Sign = bLeft ? -1. : 1.;
		const double Height = Feet.Z - FloatLocal.Z;
		const double HandHeight = GripRung(Height + 134., bLeft);
		const double BootHeight = GripRung(Height + 18., !bLeft);
		FVector Hand = FloatLocal + RungFace(HandHeight) + FVector(-7, Sign*19., 2);
		// Hands go over the rock lip as Max brings a knee onto the shelf.
		const double Mantle = FMath::SmoothStep(.68f, .86f, Ascent);
		Hand = FMath::Lerp(Hand, FVector(Landing.X-87., Landing.Y+Sign*22., Landing.Z+5.), Mantle);
		const FVector Boot = FloatLocal + RungFace(BootHeight) + FVector(-13, Sign*15., 8);
		LadderLimb(*TraversalBody, bLeft ? TEXT("upperarm_l") : TEXT("upperarm_r"),
			bLeft ? TEXT("lowerarm_l") : TEXT("lowerarm_r"), bLeft ? TEXT("hand_l") : TEXT("hand_r"),
			HighWaterLayout::World(Hand), HighWaterLayout::World(Feet + FVector(-13,Sign*62.,91.)), Weight);
		LadderLimb(*TraversalBody, bLeft ? TEXT("thigh_l") : TEXT("thigh_r"),
			bLeft ? TEXT("calf_l") : TEXT("calf_r"), bLeft ? TEXT("foot_l") : TEXT("foot_r"),
			HighWaterLayout::World(Boot), HighWaterLayout::World(Feet + FVector(102,Sign*28.,69.)), Weight);
	}
	TraversalBody->RefreshBoneTransforms();
	Max.FollowDigHead(TraversalBody->GetSocketTransform(TEXT("head")));
}

void AHighWaterChamber::UpdateClimb(ANight_of_the_WolvesCharacter& Max, float Dt)
{
	using namespace HighWaterLayout;
	ClimbTime += Dt;
	const float Travel = FMath::Clamp(ClimbTime / FMath::Max(1.f, ClimbSeconds), 0.f, 1.f);
	const float Ascent = bClimbingUp ? Travel : 1.f - Travel;
	const FVector Entry = bClimbingUp ? StartFeet : LowerClimb;
	const FVector Top = bClimbingUp ? Landing : StartFeet;
	const float Rise = FMath::SmoothStep(.09f, .84f, Ascent);
	const double Height = (Landing.Z - FloatLocal.Z) * Rise;
	const FVector OnLadder = FloatLocal + RungFace(Height) - FVector(33, 0, 0);
	FVector Feet = FMath::Lerp(Entry, OnLadder, double(FMath::SmoothStep(0.f, .13f, Ascent)));
	Feet = FMath::Lerp(Feet, Top, double(FMath::SmoothStep(.72f, 1.f, Ascent)));
	Max.ConsumeMovementInputVector();
	Max.SetActorLocation(HighWaterLayout::World(Feet) + FVector::UpVector * (Max.GetSimpleCollisionHalfHeight() + 2.2), false);
	const auto Face = StormfallLayout::Facing;
	Max.SetActorRotation(FMath::RInterpTo(Max.GetActorRotation(), Face, Dt, 10.f));
	const float Weight = FMath::SmoothStep(0.f, .12f, Ascent) * (1.f - FMath::SmoothStep(.81f, 1.f, Ascent));
	PoseClimb(Max, Ascent, Weight);
	if (Travel >= 1.f)
	{
		bAtLanding = bClimbingUp;
		EndTraversal();
	}
}

void AHighWaterChamber::EndTraversal()
{
	if (ShoveController.IsValid()) ShoveController->SetIgnoreMoveInput(false);
	ShoveController.Reset();
	if (TraversalMax.IsValid())
	{
		TraversalMax->EndWorkingPose(TraversalBody);
		TraversalMax->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		TraversalMax->ConsumeMovementInputVector();
	}
	TraversalBody = nullptr;
	TraversalMax.Reset();
	ClimbTime = -1.f;
	SwimTime = -1.f;
	ShoveTime = -1.f;
	bBraced = false;
}

void AHighWaterChamber::StartShove(ANight_of_the_WolvesCharacter& Max)
{
	ShoveController = Cast<APlayerController>(Max.GetController());
	if (ShoveController.IsValid()) ShoveController->SetIgnoreMoveInput(true);
	Max.StopJumping();
	Max.ConsumeMovementInputVector();
	Max.GetCharacterMovement()->StopMovementImmediately();
	ShoveFrom = FloatDestination = FloatBed;
	ShoveDirection = (HighWaterLayout::Guides - FloatBed).GetSafeNormal2D();
	ShoveTime = 0.f;
	bBraced = false;
}

void AHighWaterChamber::UpdateShove(ANight_of_the_WolvesCharacter& Max, float Dt)
{
	using namespace HighWaterLayout;
	ShoveTime += Dt;
	const FVector Rail = ShoveFrom + FVector(0,-77,76);
	const FVector Stance = Rail - ShoveDirection * 66.;
	const FVector Feet = StormfallLayout::LocalPoint(Max.GetActorLocation() - FVector::UpVector*Max.GetSimpleCollisionHalfHeight());
	if (!bBraced)
	{
		const FVector Approach = HighWaterLayout::World(Stance) - HighWaterLayout::World(Feet);
		if (ShoveTime > 3.f || Water > .14f) { EndTraversal(); return; }
		if (Approach.SizeSquared2D() > FMath::Square(11.))
		{
			// Let CharacterMovement walk the last step, including stepping down off
			// the deck or sliding against rocks at the wrong-wall resting place.
			Max.AddMovementInput(Approach.GetSafeNormal2D(), FMath::Clamp(Approach.Size2D()/75.,.16,.65), true);
			return;
		}
		BeginTraversal(Max);
		bBraced = true;
		ShoveTime = 0.f;
	}
	const float Phase = FMath::Clamp(ShoveTime / 1.25f, 0.f, 1.f);
	const float Drive = FMath::SmoothStep(.32f,.83f,Phase);
	const float Grip = FMath::SmoothStep(0.f,.29f,Phase) * (1.f-FMath::SmoothStep(.79f,1.f,Phase));
	const FVector Desired = StartFeet + ShoveDirection * (49.*Drive);
	const FVector Advance = HighWaterLayout::World(Desired) - HighWaterLayout::World(Feet);
	Max.SetActorLocation(Max.GetActorLocation() + FVector(Advance.X,Advance.Y,0), true);
	const FVector CurrentFeet = StormfallLayout::LocalPoint(Max.GetActorLocation() - FVector::UpVector*Max.GetSimpleCollisionHalfHeight());
	const double Pushed = FMath::Max(0., FVector::DotProduct(CurrentFeet-StartFeet, ShoveDirection)) + 14.*Drive;
	FloatBed = ShoveFrom + ShoveDirection*Pushed;
	const FVector Heading = StormfallLayout::Facing.RotateVector(ShoveDirection);
	Max.SetActorRotation(FMath::RInterpTo(Max.GetActorRotation(), Heading.Rotation(), Dt, 12.f));
	if (TraversalBody)
	{
		TraversalBody->CopyPoseFromSkeletalComponent(Max.GetMesh());
		constexpr auto Space = EBoneSpaces::WorldSpace;
		const FQuat Spine = TraversalBody->GetBoneTransformByName(TEXT("spine_01"),Space).GetRotation();
		const FQuat Load(Max.GetActorRightVector(),FMath::DegreesToRadians((22.f-7.f*Drive)*Grip));
		TraversalBody->SetBoneRotationByName(TEXT("spine_01"),(Load*Spine).Rotator(),Space);
		const FVector FootWorld = HighWaterLayout::World(CurrentFeet);
		const FVector BarCenter = FloatBed + FVector(0,-77,76);
		for (bool bLeft : {true,false})
		{
			const double Sign = bLeft ? -1. : 1.;
			const FVector Wrist = BarCenter + FVector(-Sign*20.,0,4.) - ShoveDirection*5.;
			const FVector Pole = FootWorld + FVector::UpVector*87. + Max.GetActorRightVector()*Sign*48.;
			LadderLimb(*TraversalBody, bLeft ? TEXT("upperarm_l") : TEXT("upperarm_r"),
				bLeft ? TEXT("lowerarm_l") : TEXT("lowerarm_r"), bLeft ? TEXT("hand_l") : TEXT("hand_r"),
				HighWaterLayout::World(Wrist), Pole, Grip);
		}
		TraversalBody->RefreshBoneTransforms();
		Max.FollowDigHead(TraversalBody->GetSocketTransform(TEXT("head")));
	}
	if (Phase >= 1.f)
	{
		// Both hands let go after the first half-step. The released float coasts
		// across the wet silt and loses speed as the guide posts catch it.
		FloatDestination = Guides;
		EndTraversal();
	}
}

void AHighWaterChamber::SwimToShelf(ANight_of_the_WolvesCharacter& Max, bool bHoldingItem, float Dt)
{
	using namespace HighWaterLayout;
	if (!IsSwimmingOut())
	{
		BeginTraversal(Max);
		SwimTime = 0.f;
		SwimBank = FVector(504., FMath::Clamp(StartFeet.Y, 1915., 3083.), ShelfZ);
		const FVector Tongue(Board.X - 28., FMath::Clamp(StartFeet.Y, Board.Y-56., Board.Y+56.), ShelfZ);
		// East of the ladder, the solid rock landing can lie between Max and the
		// boarding tongue. Swim straight back along the open water beside it.
		if (StartFeet.X < 1125. && FVector::DistSquared2D(StartFeet, Tongue) < FVector::DistSquared2D(StartFeet, SwimBank))
			SwimBank = Tongue;
	}
	SwimTime += Dt;
	const double Duration = FMath::Clamp(FVector::Dist2D(StartFeet, SwimBank) / 220., 1.35, 4.);
	const double Phase = FMath::Min(1., SwimTime / Duration);
	FVector Foot = FMath::Lerp(StartFeet, SwimBank, FMath::SmoothStep(0., 1., Phase));
	const float Level = FMath::Lerp(LowWater, HighWater, Water);
	const double Buoyancy = FMath::SmoothStep(0., .20, Phase);
	const double Bank = FMath::SmoothStep(.62, 1., Phase);
	const double Stroke = SwimTime * 6.1;
	Foot.Z = FMath::Lerp(FMath::Lerp(StartFeet.Z, Level - 78. + 4. * FMath::Sin(Stroke), Buoyancy), SwimBank.Z, Bank);
	Max.ConsumeMovementInputVector();
	Max.SetActorLocation(HighWaterLayout::World(Foot) + FVector::UpVector * (Max.GetSimpleCollisionHalfHeight() + 2.2), false);
	const FVector Direction = (HighWaterLayout::World(SwimBank) - HighWaterLayout::World(StartFeet)).GetSafeNormal2D();
	Max.SetActorRotation(FMath::RInterpTo(Max.GetActorRotation(), Direction.Rotation(), Dt, 8.f));
	if (TraversalBody)
	{
		TraversalBody->CopyPoseFromSkeletalComponent(Max.GetMesh());
		const float Weight = FMath::SmoothStep(0.,.13,Phase) * (1. - FMath::SmoothStep(.78,1.,Phase));
		for (bool bLeft : {true,false})
		{
			const double Sign = bLeft ? -1. : 1.;
			const FVector StrokeHand(27. + 24.*FMath::Cos(Stroke) * Sign, Sign*(32. + 13.*FMath::Sin(Stroke) * Sign), 92.);
			const FVector Hand = bHoldingItem && !bLeft ? FVector(18,27,149) : StrokeHand;
			const FTransform Stance(Max.GetActorQuat(), HighWaterLayout::World(Foot));
			LadderLimb(*TraversalBody,bLeft ? TEXT("upperarm_l") : TEXT("upperarm_r"),
				bLeft ? TEXT("lowerarm_l") : TEXT("lowerarm_r"), bLeft ? TEXT("hand_l") : TEXT("hand_r"),
				Stance.TransformPosition(Hand), Stance.TransformPosition({-10,Sign*52.,94}), Weight);
		}
		TraversalBody->RefreshBoneTransforms();
		Max.FollowDigHead(TraversalBody->GetSocketTransform(TEXT("head")));
	}
	if (Phase >= 1.) EndTraversal();
}
