#include "FirstLegCrossing.h"
#include "FirstLegCrossingLayout.h"
#include "WolfTrailGameMode.h"
#include "DialogueVoiceComponent.h"
#include "WolfMeshComponent.h"
#include "WolfPackComponent.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
constexpr float FlightSeconds = .54f;
constexpr float HoldAfterHit = 1.67f;
constexpr float AlarmAt = .22f;
}

float AFirstLegCrossing::BanditImpactTime() const
{
	return RealizationSeconds + PullbackSeconds + FlightSeconds;
}

float AFirstLegCrossing::BanditShotEnd() const
{
	return BanditImpactTime() + HoldAfterHit;
}

bool AFirstLegCrossing::HasSurvivedRobbery() const
{
	return Phase==EPhase::Diversion && bWolfIntercept && bBanditStruck &&
		TimeUntilBanditFires<0.f && !AttackViewer.IsValid() && AttackTime>=BanditShotEnd();
}

bool AFirstLegCrossing::IsBreakingAway(const FVector& Position) const
{
	using namespace FirstLegLayout;
	const FVector UpRoad=(BanditMark-GoldPatch).GetSafeNormal2D();
	const double PastGunLine=FVector::DotProduct(Position-BanditMark,UpRoad);
	return PastGunLine>=-210. || FVector::DistSquared2D(Position,TossOrigin)>FMath::Square(RobberyMoveAllowance);
}

void AFirstLegCrossing::CoverMax(AWolfTrailGameMode& Game,const APawn& Pawn,float Delta,bool bBreakingAway)
{
	if (TimeUntilBanditFires<0.f)
	{
		// Once he has Max covered, slipping back behind the line or turning
		// the camera cannot make him resume collecting the gold.
		TimeUntilBanditFires=BanditAimSeconds;
		PendingSpeech.Reset();
		Speech->ResetPerformance();
		Speech->SetMood(.9f,0.f);
		Bandit->PlayAnimation(AimAnim,true);
		Speech->PlayLine(StayVoice,EDialogueLine::BanditStay);
		StageCue(TEXT("“Stay right there.”"),GetDialogueDuration(EDialogueLine::BanditStay));
		UE_LOG(LogTemp,Display,TEXT("BANDIT covers Max: pouch landed=%d, pack approach=%d, broke away=%d"),bPouchLanded,bWolfIntercept,bBreakingAway);
	}
	const FVector ToMax=Pawn.GetActorLocation()-BanditRoot->GetComponentLocation();
	const FRotator Aim(0,ToMax.Rotation().Yaw,0);
	BanditRoot->SetWorldRotation(FMath::RInterpTo(BanditRoot->GetComponentRotation(),Aim,Delta,10.f));
	if (bBreakingAway) TimeUntilBanditFires=FMath::Min(TimeUntilBanditFires,.65f);
	TimeUntilBanditFires-=Delta;
	if (TimeUntilBanditFires<=0.f)
	{
		UGameplayStatics::PlaySoundAtLocation(this,Shot,Bandit->GetSocketLocation(TEXT("hand_r")),.8f);
		Game.Die(TEXT("Bandit"));
	}
}

void AFirstLegCrossing::BeginBanditAttack(APawn& Pawn)
{
	auto* PC = Cast<APlayerController>(Pawn.GetController());
	auto* Leader = Pack->GetLeadWolf();
	if (!PC || !Leader) return;
	if (!AttackCamera) AttackCamera = GetWorld()->SpawnActor<ACameraActor>();
	if (!AttackCamera) return;

	AttackTime = 0.f;
	AttackViewer = PC;
	AttackPawn = &Pawn;
	CollectingRotation = BanditRoot->GetComponentRotation();
	PounceStart = Leader->GetComponentLocation();
	const FVector Feet = BanditRoot->GetComponentLocation();
	AttackAxis = (Feet - Leader->GetComponentLocation()).GetSafeNormal2D();
	if (AttackAxis.IsNearlyZero()) AttackAxis = FVector::ForwardVector;

	// Use the side of the clearing with the longest clear sightline.
	const FVector Across(-AttackAxis.Y, AttackAxis.X, 0);
	const FVector Focus = Feet + FVector(0,0,90) - AttackAxis * 48.;
	const FVector Lift(0,0,82);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BanditShot), false, &Pawn);
	double Best = -1;
	for (const int32 Sign : {1, -1})
	{
		const FVector Side = Across * Sign;
		const FVector Offset = Side * 475. - AttackAxis * 124. + Lift;
		FHitResult Obstruction;
		const bool Blocked = GetWorld()->LineTraceSingleByChannel(Obstruction, Focus, Focus + Offset, ECC_Camera, Query);
		const double Clear = Blocked ? Obstruction.Distance : Offset.Size();
		if (Clear > Best) { Best = Clear; CameraSide = Side; }
	}

	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	if (ACharacter* Max = Cast<ACharacter>(&Pawn))
	{
		auto* Movement = Max->GetCharacterMovement();
		bJumpWasAllowed = Movement->IsJumpAllowed();
		Movement->SetJumpAllowed(false);
		Max->StopJumping();
		Movement->StopMovementImmediately();
	}
	Bandit->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	BanditFace->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	FrameBanditAttack();
	PC->SetViewTarget(AttackCamera);
	if (Snarl) UGameplayStatics::PlaySoundAtLocation(this, Snarl, Leader->GetComponentLocation(), .56f, .88f);
	UE_LOG(LogTemp, Display, TEXT("BANDIT TAKEDOWN: heard the pack at %.0f cm"), FVector::Dist2D(Feet, Leader->GetComponentLocation()));
}

void AFirstLegCrossing::FrameBanditAttack()
{
	const FVector Eyes = .5 * (BanditFace->GetSocketLocation(TEXT("FACIAL_L_Eye")) +
		BanditFace->GetSocketLocation(TEXT("FACIAL_R_Eye")));
	const FVector PortraitFocus = Eyes - FVector(0,0,4);
	const FVector Portrait = PortraitFocus + BanditFace->GetRightVector().GetSafeNormal2D() * 113. +
		CameraSide * 20. + FVector(0,0,4);
	const FVector Feet = BanditRoot->GetComponentLocation();
	const FVector Torso = Bandit->GetSocketLocation(TEXT("spine_04"));
	const FVector ActionFocus = .5 * (Torso + Feet + FVector(0,0,62)) - AttackAxis * 52.;
	const FVector Wide = Feet - AttackAxis * 130. + CameraSide * 472. + FVector(0,0,205);
	const float WideWeight = FMath::SmoothStep(RealizationSeconds, RealizationSeconds + PullbackSeconds, AttackTime);
	const FVector Focus = FMath::Lerp(PortraitFocus, ActionFocus, WideWeight);
	FVector Eye = FMath::Lerp(Portrait, Wide, WideWeight);

	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BanditPullback), false, AttackPawn.Get());
	if (GetWorld()->LineTraceSingleByChannel(Hit, Focus, Eye, ECC_Camera, Query) && !Hit.bStartPenetrating)
		Eye = Hit.Location - (Eye - Focus).GetSafeNormal() * 6.;
	AttackCamera->SetActorLocationAndRotation(Eye, (Focus - Eye).Rotation());
	AttackCamera->GetCameraComponent()->SetFieldOfView(FMath::Lerp(31.f, 57.f, WideWeight));
}

void AFirstLegCrossing::AdvanceBanditAttack(float Delta)
{
	UWolfMeshComponent* Leader = Pack->GetLeadWolf();
	if (!Leader) return;
	const FVector Feet = BanditRoot->GetComponentLocation();
	const float FlightStart = RealizationSeconds + PullbackSeconds;
	const float SinceHit = AttackTime - BanditImpactTime();
	const float Turn = FMath::SmoothStep(AlarmAt, RealizationSeconds, AttackTime);
	if (!bBanditStruck)
	{
		if (AttackTime >= AlarmAt && AttackTime - Delta < AlarmAt)
			Speech->SetMood(0.f, 0.f, 1.f);
		BanditRoot->SetWorldRotation(FQuat::Slerp(CollectingRotation.Quaternion(), (-AttackAxis).Rotation().Quaternion(), Turn));
		// Recover from the crouch as he hears the growl, then brace facing the noise.
		Bandit->SetPosition(FMath::Lerp(.13f, StoopAnim->GetPlayLength() - .02f, Turn), false);
	}
	if (AttackTime < FlightStart)
	{
		const FVector Goal = Feet - AttackAxis * 236.;
		const FVector Before = Leader->GetComponentLocation();
		const FVector Next = FMath::VInterpConstantTo(Before, Goal, Delta, 530.f);
		Leader->SetWorldLocationAndRotation(Next, AttackAxis.Rotation());
		Leader->AdvanceGait(Delta, FVector::Dist2D(Before, Next), false, 1.f);
		if (AttackTime > FlightStart - .18f) Leader->SetPouncePose(0.f);
		PounceStart = Next;
	}
	else
	{
		if (SinceHit >= 0.f && !bBanditStruck)
		{
			bBanditStruck = true;
			Bandit->PlayAnimation(DeathAnim, false);
			Speech->SetVolumeMultiplier(.95f);
			Speech->PlayLine(PainYell, EDialogueLine::PainYell);
			UE_LOG(LogTemp, Display, TEXT("BANDIT TAKEDOWN: leader struck, fall and pain yell"));
		}
		const float Flight = FMath::Clamp((AttackTime - FlightStart) / FlightSeconds, 0.f, 1.f);
		const float Down = FMath::SmoothStep(.06f, .82f, SinceHit);
		const float Pull = FMath::Sin(FMath::Max(0.f, SinceHit) * 8.f);
		if (bBanditStruck)
		{
			// The mannequin fall is authored for a character capsule with a moving
			// root. Seat this standalone mesh on the road as the wolf knocks him over.
			SetFallenPose(Down);
		}
		const FRotator Facing(FMath::Lerp(14.f, -2.f, Down), AttackAxis.Rotation().Yaw, 0.f);
		Leader->SetWorldRotation(Facing);
		if (Down < .5f) Leader->SetPouncePose(Flight);
		else Leader->SetStrugglePose(Down, SinceHit * 8.f);
		// Fit the wolf's muzzle to the animated near shoulder, including the sideways
		// displacement in the bandit's fall. His feet return to ground as the body settles.
		const FVector Shoulder = Bandit->GetSocketLocation(TEXT("spine_04"));
		const FVector Muzzle = Leader->GetSocketLocation(TEXT("Jaw")) + AttackAxis * 20.;
		FVector Grip = Shoulder - (Muzzle - Leader->GetComponentLocation()) +
			CameraSide * (10. + 4. * Down * Pull);
		Grip.Z = FMath::Max(Grip.Z, Feet.Z);
		const FVector Arc = FVector::UpVector * (FMath::Sin(PI * Flight) * 42.);
		Leader->SetWorldLocation(FMath::Lerp(PounceStart, Grip, Flight) + Arc);
		if (SinceHit >= .36f && SinceHit - Delta < .36f)
			UGameplayStatics::PlaySoundAtLocation(this, Shot, Bandit->GetSocketLocation(TEXT("hand_r")), .63f);
	}

	const FVector FallenHip = Bandit->GetSocketLocation(TEXT("pelvis"));
	const float Prone = FMath::SmoothStep(0.f, 1.f, SinceHit);
	const FVector GuardAt = FMath::Lerp(Feet, FVector(FallenHip.X, FallenHip.Y, Feet.Z), Prone);
	Pack->FlankBandit(Delta, GuardAt, AttackAxis, SinceHit);
	if (AttackViewer.IsValid())
	{
		FrameBanditAttack();
		if (AttackTime >= BanditShotEnd())
		{
			EndBanditShot(true);
			UE_LOG(LogTemp, Display, TEXT("BANDIT TAKEDOWN: returned to Max"));
		}
	}
}

void AFirstLegCrossing::SetFallenPose(float Weight)
{
	const FVector Feet = BanditRoot->GetComponentLocation();
	const FVector TorsoOffset = Bandit->GetSocketLocation(TEXT("spine_04")) - Bandit->GetComponentLocation();
	const FVector NearPouch(-.60 * TorsoOffset.X, -.60 * TorsoOffset.Y, 21. - TorsoOffset.Z);
	Bandit->SetWorldLocation(Feet + (NearPouch + AttackAxis * 42.) * Weight);
}

void AFirstLegCrossing::EndBanditShot(bool bBlend)
{
	if (auto* PC = AttackViewer.Get())
	{
		if (auto* Max = Cast<ACharacter>(AttackPawn.Get()))
			Max->GetCharacterMovement()->SetJumpAllowed(bJumpWasAllowed);
		if (APawn* Pawn = PC->GetPawn())
		{
			const bool bNear = FVector::DistSquared(AttackCamera->GetActorLocation(), Pawn->GetActorLocation()) < FMath::Square(850.);
			PC->SetViewTargetWithBlend(Pawn, bBlend && bNear ? .25f : 0.f, VTBlend_Cubic, 0.f, true);
		}
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
	AttackViewer.Reset();
	AttackPawn.Reset();
}
