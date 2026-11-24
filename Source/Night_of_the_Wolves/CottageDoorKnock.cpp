#include "WolfTrailGameMode.h"
#include "CottageHermit.h"
#include "MaxDoorKnock.h"
#include "Night_of_the_WolvesCharacter.h"
#include "WolfCottageSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

void AWolfTrailGameMode::BeginKnock(ANight_of_the_WolvesCharacter& Max)
{
	const FVector P = Max.GetActorLocation();
	auto* PC = Cast<APlayerController>(Max.GetController());
	auto* Movement = Max.GetCharacterMovement();
	if (!PC || !Movement->IsMovingOnGround() || IsKnocking()) return;
	const FBox Pit = Cottage->GetPitBounds();
	const double Side = P.Y < CabinDoorLocation.Y ? -1. : 1.;
	const double Height = Pit.Max.Z + 147.;
	if (bPitDug)
	{
		// The leaf is recessed beyond the open hollow. Rap the boxed timber
		// at the nearest jamb while keeping a foot planted on the solid rim.
		KnockStance = FVector(CabinDoorLocation.X-94., Pit.GetCenter().Y + Side*(Pit.GetExtent().Y-7.), P.Z);
		KnockContact = FVector(CabinDoorLocation.X-42.5, CabinDoorLocation.Y+Side*105., Height);
	}
	else
	{
		const double Y = FMath::Clamp(P.Y-CabinDoorLocation.Y,-38.,38.);
		KnockStance = FVector(CabinDoorLocation.X-57., CabinDoorLocation.Y+Y, P.Z);
		KnockContact = FVector(CabinDoorLocation.X-7., KnockStance.Y+14., Height);
	}
	KnockFacing = (KnockContact-KnockStance).GetSafeNormal2D().Rotation();
	KnockApproach = KnockElapsed = 0.f;
	KnockHits = 0;
	bKnockPlanted = false;
	bKnockJumpAllowed = Movement->IsJumpAllowed();
	PC->SetIgnoreMoveInput(true);
	Max.StopJumping();
	Max.ConsumeMovementInputVector();
	Movement->StopMovementImmediately();
	Movement->SetJumpAllowed(false);
}

void AWolfTrailGameMode::UpdateKnock(ANight_of_the_WolvesCharacter& Max, float Dt)
{
	const FVector P = Max.GetActorLocation();
	if (!bKnockPlanted)
	{
		KnockApproach += Dt;
		FVector WalkTo = KnockStance;
		const FBox& Pit = Cottage->GetPitBounds();
		if (bPitDug && P.X <= Pit.Min.X+3. && FMath::Abs(P.Y-Pit.GetCenter().Y) < Pit.GetExtent().Y+2.)
		{
			// Walk around the outside corner before turning toward the jamb.
			WalkTo.X = P.X;
			WalkTo.Y = Pit.GetCenter().Y + FMath::Sign(KnockStance.Y-Pit.GetCenter().Y)*(Pit.GetExtent().Y+12.);
		}
		else if (!bPitDug && FMath::Abs(P.Y-CabinDoorLocation.Y)>64.)
			WalkTo.X = FMath::Min(P.X, CabinDoorLocation.X-100.);
		const FVector Step = WalkTo-P;
		const double Distance = Step.Size2D();
		if (FVector::DistSquared2D(P,KnockStance)>FMath::Square(3.5))
		{
			if (KnockApproach > 4.f) { EndKnock(); return; }
			Max.AddMovementInput(Step.GetSafeNormal2D(), FMath::Clamp(Distance/105.,.11,.68), true);
			return;
		}
		Max.ConsumeMovementInputVector();
		Max.GetCharacterMovement()->StopMovementImmediately();
		Max.SetActorRotation(FMath::RInterpConstantTo(Max.GetActorRotation(),KnockFacing,Dt,520.f));
		if (!Max.GetActorRotation().Equals(KnockFacing,1.f)) return;
		bKnockPlanted = true;
		Cottage->HideWindowBarrel();
	}
	KnockElapsed += Dt;
	const float Phase = FMath::Min(KnockElapsed / KnockSeconds, 1.f);
	Max.PoseDoorKnock(KnockContact, -FVector::ForwardVector, Phase);
	while (KnockHits < UE_ARRAY_COUNT(MaxDoorKnock::Hits) && Phase >= MaxDoorKnock::Hits[KnockHits])
	{
		constexpr float Strength[] = {.56f,.67f,.49f};
		Cottage->Impact(KnockContact, Strength[KnockHits], 1.62f+.06f*KnockHits);
		++KnockHits;
	}
	if (Phase >= 1.f)
	{
		EndKnock();
		bHermitReacting = true;
		bHermitFired = bHermitShouted = false;
		HermitElapsed = 0.f;
		Hermit->AnswerKnock(Max);
	}
}

void AWolfTrailGameMode::EndKnock()
{
	if (!IsKnocking()) return;
	KnockElapsed = -1.f;
	if (auto* PC = GetWorld()->GetFirstPlayerController())
	{
		if (auto* Max = Cast<ANight_of_the_WolvesCharacter>(PC->GetPawn()))
		{
			Max->EndDoorKnock();
			Max->GetCharacterMovement()->StopMovementImmediately();
			Max->GetCharacterMovement()->SetJumpAllowed(bKnockJumpAllowed);
		}
		PC->SetIgnoreMoveInput(false);
	}
}
