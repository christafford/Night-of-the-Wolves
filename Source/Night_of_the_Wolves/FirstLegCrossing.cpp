#include "FirstLegCrossing.h"
#include "DialogueVoiceComponent.h"
#include "WolfPackComponent.h"
#include "WolfMeshComponent.h"
#include "FirstLegCablewayLayout.h"
#include "WolfHeldItem.h"
#include "Components/CapsuleComponent.h"
#include "TwoBoneIK.h"
#include "Night_of_the_WolvesCharacter.h"
#include "WolfTrailGameMode.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/PoseableMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"

using namespace FirstLegLayout;

namespace
{
// Max kicks off the deck. The wheel gathers speed on the grade, then loses
// momentum in the level runout and meets the far stop at rest.
constexpr float PushOffSlope = .74f;
float TrolleySpeed(float TimeFraction)
{
	return FMath::CubicInterpDerivative(0.f, PushOffSlope, 1.f, 0.f, TimeFraction);
}

bool Within(const FVector& A,const FVector& B,float Radius)
{
	return FVector::Dist2D(A,B)<Radius && FMath::Abs(A.Z-B.Z)<235;
}
bool IsAtTrolley(const APawn& Pawn)
{
	const FVector Offset = Pawn.GetActorLocation() - FirstLegCableway::RiderPosition(0);
	const ACharacter* Max = Cast<ACharacter>(&Pawn);
	return Offset.SizeSquared2D() < FMath::Square(103.f) && FMath::Abs(Offset.Z) < 42.f &&
		Max && Max->GetCharacterMovement()->IsMovingOnGround();
}
}

void AFirstLegCrossing::BeginRopePose(ACharacter& Max)
{
	if (IsValid(HangingPose)) return;
	auto* Body = Max.GetMesh();
	HangingPose = NewObject<UPoseableMeshComponent>(&Max);
	HangingPose->SetupAttachment(Body);
	HangingPose->SetSkinnedAssetAndUpdate(Body->GetSkinnedAsset());
	HangingPose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HangingPose->RegisterComponent();
	for (int32 I = 0; I < Body->GetNumMaterials(); ++I)
		HangingPose->SetMaterial(I, Body->GetMaterial(I));
	// The regular locomotion instance keeps running while the sleeves and torso
	// use the rope pose. It is available immediately when Max lets go.
	HangingPose->CopyPoseFromSkeletalComponent(Body);
	if (auto* Player = Cast<ANight_of_the_WolvesCharacter>(&Max))
		Player->SetHangingAppearance(HangingPose);
	if (auto* Held = Max.FindComponentByClass<UWolfHeldItem>())
		Held->SetHiddenInGame(true, true);
	Body->SetVisibility(false);
}

void AFirstLegCrossing::UpdateRopePose(ACharacter& Max, float CastProgress)
{
	if (!HangingPose) return;
	using namespace FirstLegCableway;
	const bool bCasting = Phase == EPhase::Casting;
	const FVector Center = Max.GetActorLocation();

	auto PlaceLimb = [this](FName Upper, FName Lower, FName End, const FVector& BendToward, const FVector& Target, float Weight)
	{
		constexpr auto World = EBoneSpaces::WorldSpace;
		FTransform A = HangingPose->GetBoneTransformByName(Upper, World);
		FTransform B = HangingPose->GetBoneTransformByName(Lower, World);
		FTransform C = HangingPose->GetBoneTransformByName(End, World);
		const FVector Goal = FMath::Lerp(C.GetLocation(), Target, double(Weight));
		AnimationCore::SolveTwoBoneIK(A, B, C, BendToward, Goal, false, 1., 1.);
		HangingPose->SetBoneRotationByName(Upper, A.Rotator(), World);
		HangingPose->SetBoneRotationByName(Lower, B.Rotator(), World);
	};
	if (bCasting)
		HangingPose->CopyPoseFromSkeletalComponent(Max.GetMesh());
	for (bool bLeft : {true, false})
	{
		const float SideSign = bLeft ? -1.f : 1.f;
		// Max keeps the coil at his left hip and swings his right arm from a
		// low backswing into an underhand toss. Neither IK target reaches for the iron.
		const float Swing = FMath::SmoothStep(.14f, .32f, CastProgress);
		const float Settle = FMath::SmoothStep(.52f, .95f, CastProgress);
		const FVector Back = Center - Axis * 26. + Side * 31. + FVector(0,0,-12);
		const FVector Release = Center + Axis * 44. + Side * 21. + FVector(0,0,73);
		const FVector Coil = Center + Axis * 17. - Side * 25. + FVector(0,0,16);
		const FVector CastingHand = FMath::Lerp(FMath::Lerp(Back, Release, Swing),
			Center + Axis * 21. + Side * 26. + FVector(0,0,22), Settle);
		const FVector Target = bCasting ? (bLeft ? Coil : CastingHand) : GripPosition(RideFraction, bLeft);
		const FVector Elbow = Center + Side * SideSign * 45.f - Axis * 12.f + FVector(0,0,42);
		PlaceLimb(bLeft ? TEXT("upperarm_l") : TEXT("upperarm_r"), bLeft ? TEXT("lowerarm_l") : TEXT("lowerarm_r"),
			bLeft ? TEXT("hand_l") : TEXT("hand_r"), Elbow, Target,
			bCasting ? FMath::SmoothStep(0.f, .11f, CastProgress) : 1.f);
		if (!bCasting)
		{
			const FVector Knee = Center + Axis * 46.f + Side * SideSign * 15.f - FVector(0,0,30);
			const FVector Foot = Center - Axis * (bLeft ? 29.f : 38.f) + Side * SideSign * 12.f -
				FVector(0,0, bLeft ? 64.f : 58.f);
			PlaceLimb(bLeft ? TEXT("thigh_l") : TEXT("thigh_r"), bLeft ? TEXT("calf_l") : TEXT("calf_r"),
				bLeft ? TEXT("foot_l") : TEXT("foot_r"), Knee, Foot, 1.f);
		}
	}
	HangingPose->RefreshBoneTransforms();
}

void AFirstLegCrossing::EndRopePose(ACharacter& Max)
{
	if (!IsValid(HangingPose)) return;
	Max.GetMesh()->SetVisibility(true);
	if (auto* Player = Cast<ANight_of_the_WolvesCharacter>(&Max))
		Player->SetHangingAppearance(nullptr);
	if (auto* Held = Max.FindComponentByClass<UWolfHeldItem>())
		Held->SetHiddenInGame(false, true);
	HangingPose->DestroyComponent();
	HangingPose = nullptr;
}

void AFirstLegCrossing::UpdateCableView(APawn& Pawn, float Delta)
{
	const bool bHandling = Phase == EPhase::Casting || Phase == EPhase::Riding;
	const bool bNearCarriage = Phase == EPhase::Ready || Phase == EPhase::Rigged;
	const double StageDistance = FVector::Dist2D(Pawn.GetActorLocation(), FirstLegCableway::NearStop);
	const double Framing = bHandling ? 1. : bNearCarriage ? 1.-FMath::SmoothStep(225., 540., StageDistance) : 0.;
	const bool bFramed = Framing > 0.;
	if (!CableBoom.IsValid())
	{
		if (!bFramed) return;
		CableBoom = Pawn.FindComponentByClass<USpringArmComponent>();
		if (!CableBoom.IsValid()) return;
		PreviousBoomLength = CableBoom->TargetArmLength;
		PreviousBoomOffset = CableBoom->TargetOffset;
		bPreviousBoomCollision = CableBoom->bDoCollisionTest;
	}
	// As Max walks up the ramp, include the bare hook before the player chooses
	// a tool. The camera keeps its orbit controls while the rope is in use.
	const FVector Aim = FirstLegCableway::Axis * 38. + FirstLegCableway::Side * 38. + FVector(0,0,165);
	const FVector Offset = PreviousBoomOffset + Aim * Framing;
	const float Length = FMath::Lerp(PreviousBoomLength, 640.f, Framing);
	CableBoom->bDoCollisionTest = bFramed ? false : bPreviousBoomCollision;
	CableBoom->TargetOffset = FMath::VInterpTo(CableBoom->TargetOffset, Offset, Delta, 11.f);
	CableBoom->TargetArmLength = FMath::FInterpTo(CableBoom->TargetArmLength, Length, Delta, 11.f);
	if (!bFramed && CableBoom->TargetOffset.Equals(PreviousBoomOffset,.3f) &&
		FMath::IsNearlyEqual(CableBoom->TargetArmLength,PreviousBoomLength,.3f))
		RestoreCableView();
}

void AFirstLegCrossing::RestoreCableView()
{
	if (!CableBoom.IsValid()) return;
	CableBoom->bDoCollisionTest = bPreviousBoomCollision;
	CableBoom->TargetArmLength = PreviousBoomLength;
	CableBoom->TargetOffset = PreviousBoomOffset;
	CableBoom.Reset();
}

void AFirstLegCrossing::TraversalMovement(APawn& Pawn, bool bActive)
{
	auto* Max = Cast<ACharacter>(&Pawn);
	if (!Max) return;
	Max->StopJumping();
	Max->ConsumeMovementInputVector();
	auto* Move = Max->GetCharacterMovement();
	Move->StopMovementImmediately();
	Move->SetMovementMode(bActive ? MOVE_None : MOVE_Falling);
	if (bActive)
	{
		BeginRopePose(*Max);
		UpdateRopePose(*Max, 1.f);
	}
	else EndRopePose(*Max);
}

bool AFirstLegCrossing::IsTraversalLocked() const
{
	return IsWatchingCreekPack() || AttackViewer.IsValid() || Phase==EPhase::Riding || Phase==EPhase::Landing || Phase==EPhase::Missed;
}

void AFirstLegCrossing::StageCue(const FString& Message,float Seconds)
{
	Cue=Message;
	CueTime=Seconds;
}

void AFirstLegCrossing::QueueSpeech(USoundBase* Line)
{
	if (Line) PendingSpeech.Add(Line);
}

void AFirstLegCrossing::NoticeGold()
{
	if (bGoldDemandSpoken) return;
	bGoldDemandSpoken = true;
	Speech->SetMood(0.f, 1.f);
	QueueSpeech(GoldVoice);
}

void AFirstLegCrossing::InterruptEncounter()
{
	EndCreekLookback();
	if (Phase == EPhase::Casting && Cord) SetTrolleyPose(0.f, 0.f);
	RestoreCableView();
	if (WheelAudio) WheelAudio->Stop();
	if (HardwareAudio) HardwareAudio->Stop();
	if (IsValid(HangingPose))
	{
		if (auto* Max = Cast<ACharacter>(HangingPose->GetOwner()))
		{
			if (Phase == EPhase::Riding) TraversalMovement(*Max, false);
			else EndRopePose(*Max);
		}
	}
	PendingSpeech.Reset();
	EndBanditShot(false);
	if (Speech) Speech->ResetPerformance();
}

void AFirstLegCrossing::MoveBandit(const FVector& Position,const FVector& Facing)
{
	BanditRoot->SetWorldLocation(Position);
	const FVector D=Facing-Position;
	BanditRoot->SetWorldRotation(FRotator(0,D.Rotation().Yaw,0));
}

void AFirstLegCrossing::Restore(int32 Checkpoint, const FVector& SavedPouchSite)
{
	InterruptEncounter();
	Phase=Checkpoint>=5 ? EPhase::Escaped : Checkpoint==4 ? EPhase::FarBank :
		  Checkpoint==3 ? EPhase::Ready : EPhase::Approach;
	ActionTime=BankTime=PackTime=DiversionTime=AimTime=CueTime=0;
	AttackTime=-1;
	RideFraction = Checkpoint >= 4 ? 1.f : 0.f;
	RideTime = RideFraction * TrolleyRideSeconds;
	bWolfIntercept=bPouchLanded=bGoldDemandSpoken=bBanditStruck=false;
	TimeUntilBanditFires=-1.f;
	Cue.Empty();
	if (!Cord) return;
	Speech->SetVolumeMultiplier(.72f);
	Bandit->SetRelativeLocation(FVector::ZeroVector);
	Bandit->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	BanditFace->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	MoveBandit(BanditMark+FVector(0,230,0),BanditMark-FVector(500,0,0));
	Bandit->PlayAnimation(AimAnim,true);
	SetTrolleyPose(RideFraction, Checkpoint >= 4 ? 1.f : 0.f);
	UpdateCableRoll(FirstLegCableway::HookSaddle(RideFraction), 0.f);
	GoldLanding=Checkpoint>=5 && !SavedPouchSite.IsZero() ? SavedPouchSite : GoldPatch;
	if (Checkpoint>=5)
	{
		Bag->SetWorldLocation(GoldLanding+FVector(0,0,12));
		MoveBandit(GoldLanding,GoldLanding-FVector(200,0,0));
		Bandit->PlayAnimation(DeathAnim,false);
		Bandit->SetPosition(DeathAnim->GetPlayLength()-.02f,false);
		Bandit->TickAnimation(0.f, false);
		Bandit->RefreshBoneTransforms();
		AttackAxis = (GoldLanding - FVector(12780,-930,50)).GetSafeNormal2D();
		SetFallenPose(1.f);
	}
	Bag->SetVisibility(Checkpoint>=5);
	for (int32 I=0; I<SpilledCoins.Num(); ++I)
	{
		SpilledCoins[I]->SetWorldLocation(GoldLanding+FVector(9*I-34,20*FMath::Sin(I*2.2),0));
		SpilledCoins[I]->SetVisibility(Checkpoint>=5);
	}
	HideCreekWakes();
	if (Checkpoint==4) PlaceCreekPack(CreekHandoff,0.f,true);
#if !UE_BUILD_SHIPPING
	TestTime=0; TestWaypoint=0;
#endif
}

UWolfMeshComponent* AFirstLegCrossing::GetLeadWolf() const { return Pack->GetLeadWolf(); }

void AFirstLegCrossing::UpdatePack(float Delta,float HoldDistance,bool bPermitAttack)
{
	const bool bFar=Phase>=EPhase::FarBank;
	const FVector From=Pack->GetMember(1)->GetComponentLocation();
	FVector Guide=Takeoff-FVector(0,0,94);
	if (bFar)
	{
		Guide=FarRoad[UE_ARRAY_COUNT(FarRoad)-1];
		for (const FVector& Bend : FarRoad)
			if (Bend.X>From.X+330.f) { Guide=Bend; break; }
	}
	const FVector Heading=(Guide-From).GetSafeNormal2D(SMALL_NUMBER,FVector::ForwardVector);
	Pack->Hunt(Delta,Guide,Heading,HoldDistance,bFar ? EWolfGroundEncounter::FarBank : EWolfGroundEncounter::NearBank,bPermitAttack);
}

bool AFirstLegCrossing::IsInsideWater(const FVector& P) const
{
	float Offset=StreamBend(P.Y);
	return P.X>9800+Offset && P.X<10900+Offset && P.Z < -120;
}

bool AFirstLegCrossing::TickEncounter(AWolfTrailGameMode& Game, APawn& Pawn, float Delta)
{
	const FVector P=Pawn.GetActorLocation();
	if (Game.IsDead()) return false;
	UpdateCableView(Pawn, Delta);
	if (!IsTraversalLocked() && (IsInsideWater(P) || (P.X>8450 && P.Z<-350)))
	{
		Game.Inventory.Extinguish(); Game.Die(TEXT("Current")); return true;
	}
	// This protects the supply route even if the user jumps off outside the dressed banks.
	if (Phase<=EPhase::Rigged && P.X>10870+StreamBend(P.Y))
	{
		Game.Inventory.Extinguish(); Game.Die(TEXT("Current")); return true;
	}
	if (Phase==EPhase::Approach)
	{
		if (P.X<8950) return false;
		if (!Game.Inventory.Has(TEXT("Rope")) || !Game.Inventory.Has(TEXT("Gold")))
		{
			Game.InteractionPrompt=TEXT("The bridge is gone. A logging cable still crosses the creek.");
			return false;
		}
		if (!Within(P,CreekApproach,510)) return false;
		if (!Game.CommitCheckpoint(3,CreekApproach,FRotator(0,9,0))) return false;
		Phase=EPhase::Ready;
		StageCue(TEXT("A rusted trolley hangs beside the broken bridge."));
	}
	if (P.X >= 4000) Game.InteractionPrompt.Empty();
	Game.ThreatCue.Empty();
	CueTime-=Delta;
	if (IsWatchingCreekPack())
	{
		AdvanceTrolley(Delta);
		TickCreekLookback(Delta);
		return true;
	}
	if (Phase>=EPhase::FarBank)
	{
		AdvanceTrolley(Delta);
		UpdateFarBank(Game,Pawn,Delta);
		if (CueTime>0) Game.ThreatCue=Cue;
		return true;
	}
	const bool bFire=Game.Inventory.Equipped==TEXT("Firebrand") && Game.Inventory.bBurning;
	BankTime+=Delta*(bFire ? .49f : 1.f);
	// The pack can reach the launching stage, but cannot follow the high cable.
	if (Phase==EPhase::Riding || Phase==EPhase::Landing || Phase==EPhase::Missed)
	{
		Pack->ApproachBrokenBridge(Delta);
	}
	else if (BankTime<NearBankSeconds)
	{
		UpdatePack(Delta,FMath::Lerp(920.f,218.f,BankTime/NearBankSeconds),false);
	}
	else
	{
		UpdatePack(Delta,120.f,!IsTraversalLocked());
	}
	if (!IsTraversalLocked() && BankTime>=NearBankSeconds)
	{
		Game.Die(TEXT("Wolves"));
		if (Game.IsDead()) return true;
	}
	Game.ThreatCue=BankTime>NearBankSeconds*.71 ? TEXT("Paws scrape the near bank.") :
		bFire ? TEXT("The pack hangs back from the flame.") : TEXT("Growling behind you.");
	if (CueTime>0) Game.ThreatCue=Cue;
	if (P.X < 4000) return true;
	if (IsTraversalLocked())
	{
		UpdateRide(Game, Pawn, Delta);
		return true;
	}
	const bool bAtHook = IsAtTrolley(Pawn);
	if (Phase == EPhase::Casting)
	{
		auto& Max = *CastChecked<ACharacter>(&Pawn);
		const bool bWalkingAway = ActionTime > .15f && !Max.GetLastMovementInputVector().IsNearlyZero(.2f);
		if (!bAtHook || bWalkingAway || Game.Inventory.Equipped != TEXT("Rope"))
		{
			EndRopePose(Max);
			Phase = EPhase::Ready;
			SetTrolleyPose(0.f, 0.f);
			StageCue(TEXT("The rope slips free. You still have it."));
			return true;
		}
		ActionTime += Delta;
		const float T = FMath::Min(ActionTime / CastRopeSeconds, 1.f);
		// Move below the carriage. Walking or jumping interrupts the cast.
		const FVector Mark = FirstLegCableway::RiderPosition(0.f);
		const FVector Align = FMath::Lerp(CastingStart, Mark, FMath::SmoothStep(0.f, .34f, T));
		Max.SetActorLocation(FVector(Align.X, Align.Y, Max.GetActorLocation().Z), true);
		UpdateRopePose(Max, T);
		ShowRopeCast(T);
		Game.InteractionPrompt = TEXT("Casting the rope…");
		if (T >= 1.f && Game.Inventory.Consume(TEXT("Rope")))
		{
			EndRopePose(Max);
			Phase = EPhase::Rigged;
			SoundCableLatch(FirstLegCableway::HookSaddle(0), false);
			StageCue(TEXT("Your rope hangs from the trolley."));
		}
		return true;
	}
	if (bAtHook)
	{
		Game.InteractionPrompt = Phase == EPhase::Rigged ? TEXT("E  Grip the sling and ride") :
			Game.Inventory.Equipped == TEXT("Rope") ? TEXT("F  Cast rope") :
			TEXT("The trolley hook hangs high above the stage.");
	}
	else
		Game.InteractionPrompt = TEXT("A cable runs from the loading stage to the far bank.");
	return true;
}

bool AFirstLegCrossing::IsAboveCatchDeck() const
{
	return RideFraction >= FirstLegCableway::CatchDeckStart;
}

void AFirstLegCrossing::BeginDismount(APawn& Pawn)
{
	using namespace FirstLegCableway;
	auto* Max = Cast<ACharacter>(&Pawn);
	if (!Max) return;
	// During the first metre Max is still pushing away from the loading
	// platform. A second key press here must not drop him through the boards.
	if (RideFraction <= .08f) return;
	const float T = FMath::Clamp(RideTime / TrolleyRideSeconds, 0.f, 1.f);
	const float TravelPerSecond = TrolleySpeed(T) / TrolleyRideSeconds;
	const FVector Tangent = FarStop - NearStop - FVector(0, 0, 32.f * (1.f - 2.f * RideFraction));
	Phase = IsAboveCatchDeck() ? EPhase::Landing : EPhase::Missed;
	TraversalMovement(Pawn, false);
	Max->LaunchCharacter(Tangent * TravelPerSecond, true, true);
}

void AFirstLegCrossing::AdvanceTrolley(float Delta)
{
	if (RideTime >= TrolleyRideSeconds) return;
	RideTime = FMath::Min(RideTime + Delta, TrolleyRideSeconds);
	const float T = RideTime / TrolleyRideSeconds;
	RideFraction = FMath::CubicInterp(0.f, PushOffSlope, 1.f, 0.f, T);
	SetTrolleyPose(RideFraction, 1.f);
	UpdateCableRoll(FirstLegCableway::WireContact(RideFraction), TrolleySpeed(T) / 1.36f);
	if (T >= 1.f)
		SoundCableLatch(FirstLegCableway::WireContact(1.f), true);
}

void AFirstLegCrossing::UpdateRide(AWolfTrailGameMode& Game, APawn& Pawn, float Delta)
{
	using namespace FirstLegCableway;
	AdvanceTrolley(Delta);
	if (Phase == EPhase::Riding)
	{
		Pawn.SetActorLocation(RiderPosition(RideFraction));
		if (RideFraction <= .08f)
			Game.InteractionPrompt = TEXT("Pushing off the loading stage…");
		else
			Game.InteractionPrompt = TEXT("E  Let go");
		return;
	}
	// Normal character gravity, inherited trolley velocity and real walkable
	// collision decide where Max lands. A last-moment release can carry him
	// onto the first plank even when he lets go just ahead of the lip.
	auto& Max = *CastChecked<ACharacter>(&Pawn);
	const FVector P = Max.GetActorLocation();
	const FVector StartOfPlanks = RiderPosition(CatchDeckStart);
	const bool bOverLanding = FVector::DotProduct(P - StartOfPlanks, Axis) >= -3.f;
	if (Max.GetCharacterMovement()->IsMovingOnGround() && bOverLanding)
	{
		FinishLanding(Game, Pawn);
	}
	else if (IsInsideWater(P) || P.Z < -160.f || (Max.GetCharacterMovement()->IsMovingOnGround() && !bOverLanding))
	{
		Game.Inventory.Extinguish();
		Game.Die(TEXT("Current"));
	}
}

void AFirstLegCrossing::FinishLanding(AWolfTrailGameMode& Game, APawn& Pawn)
{
	if (!Game.Inventory.Has(TEXT("Gold"))) { Game.Die(TEXT("Current")); return; }
	Phase = EPhase::FarBank;
	PackTime = 0;
	if (Game.CommitCheckpoint(4, Landing, FRotator(0,0,0)))
		BeginCreekLookback(Pawn);
}

bool AFirstLegCrossing::TryAction(AWolfTrailGameMode& Game,APawn& Pawn,bool bItemUse)
{
	if (Game.IsDead() || Phase == EPhase::Approach) return false;
	if (IsWatchingCreekPack()) return true;
	if (Phase <= EPhase::Rigged && Pawn.GetActorLocation().X < 4000) return false;
	if (Phase == EPhase::Riding)
	{
		if (!bItemUse) BeginDismount(Pawn);
		return true;
	}
	if (Phase == EPhase::Ready && IsAtTrolley(Pawn))
	{
		if (bItemUse && Game.Inventory.Equipped == TEXT("Rope") && Game.Inventory.Has(TEXT("Gold")))
		{
			auto& Max = *CastChecked<ACharacter>(&Pawn);
			Max.GetCharacterMovement()->StopMovementImmediately();
			CastingStart = Max.GetActorLocation();
			Max.SetActorRotation(FirstLegCableway::Axis.Rotation());
			Phase = EPhase::Casting;
			CastOrigin = FVector::ZeroVector;
			ActionTime = 0.f;
			BeginRopePose(Max);
		}
		else StageCue(TEXT("The bare hook is out of reach."));
		return true;
	}
	if (Phase == EPhase::Rigged && !bItemUse && IsAtTrolley(Pawn))
	{
		Phase = EPhase::Riding;
		RideTime = RideFraction = 0.f;
		Pawn.SetActorLocation(FirstLegCableway::RiderPosition(0.f));
		const float Yaw = FirstLegCableway::Axis.Rotation().Yaw;
		Pawn.SetActorRotation(FRotator(0, Yaw, 0));
		TraversalMovement(Pawn, true);
		if (AController* Controller = Pawn.GetController())
			Controller->SetControlRotation(FRotator(-9, Yaw - 32.f, 0));
		StartCableRoll(FirstLegCableway::WireContact(0.f));
		return true;
	}
	if (Phase==EPhase::Demand && bItemUse)
	{
		if (Game.Inventory.Equipped!=TEXT("Gold"))
		{
			StageCue(TEXT("His gun stays raised. He gestures at your bag.")); return true;
		}
		NoticeGold();
		if (Game.Inventory.Consume(TEXT("Gold")))
		{
			ThrowPouch(Pawn);
		}
		return true;
	}
	return true;
}

void AFirstLegCrossing::UpdateFarBank(AWolfTrailGameMode& Game,APawn& Pawn,float Delta)
{
	if (Phase==EPhase::Escaped) return;
	// During the cutaway the ordinary escape clock is held. The player always gets
	// the full remaining diversion when the view returns to the road.
	if (AttackViewer.IsValid())
	{
		AttackTime += Delta;
		AdvanceBanditAttack(Delta);
		return;
	}
	// A quick equip and throw can request all three lines before the first one ends.
	if (!PendingSpeech.IsEmpty() && Speech && !Speech->IsDelivering())
	{
		USoundBase* Wave = PendingSpeech[0];
		PendingSpeech.RemoveAt(0);
		const EDialogueLine Line = Wave == DemandVoice ? EDialogueLine::BanditDemand :
			(Wave == GoldVoice ? EDialogueLine::BanditGold : EDialogueLine::BanditStay);
		Speech->PlayLine(Wave, Line);
		const TCHAR* Words = Line == EDialogueLine::BanditDemand ? TEXT("“Hands up. Don't move.”") :
			Line == EDialogueLine::BanditGold ? TEXT("“The bag. Put it down. Slowly.”") : TEXT("“Stay right there.”");
		StageCue(Words, GetDialogueDuration(Line));
	}
	const FVector P=Pawn.GetActorLocation();
	PackTime+=Delta;
	const float Clock=PackTime*26.f/FMath::Max(4.f,PackRouteSeconds);
	// The camera has already shown them entering the channel. They keep swimming
	// during Max's head start and resume hunting from their real exit point.
	const bool bInCreek=Clock<=11.f;
	const bool bDiverted=Phase==EPhase::Diversion && bWolfIntercept && TimeUntilBanditFires<0.f;
	const bool bAtBend=P.X>=Escape.X && FMath::Abs(P.Y-Escape.Y)<1300 && P.Z>20;
	const bool bBackOnTrail=bDiverted && AttackTime>=BanditShotEnd()+DiversionSeconds && !bAtBend;
	if (bInCreek)
		PlaceCreekPack(CreekHandoff+Clock,Delta);
	else if (bDiverted && !bBackOnTrail && AttackTime<0.f)
		Pack->PursueDiversion(Delta,GoldLanding);
	else if (!bDiverted || bBackOnTrail)
	{
		const bool bVulnerable=bBackOnTrail || Clock>=21.f || P.X<12250.f;
		const float Hold=bVulnerable ? 120.f : FMath::GetMappedRangeValueClamped(FVector2D(11,21),FVector2D(900,225),Clock);
		UpdatePack(Delta,Hold,bVulnerable);
	}
	if (!bDiverted && Clock>11)
	{
		Game.Die(TEXT("Wolves"));
		if (Game.IsDead()) return;
	}
	Game.ThreatCue=Clock<9 ? TEXT("Downstream, dark heads break the surface.") : TEXT("Wet paws scramble up the bank behind you.");
	if (Phase==EPhase::FarBank && P.X>13610)
	{
		Phase=EPhase::Demand;
		if (ACharacter* Character=Cast<ACharacter>(&Pawn)) Character->GetCharacterMovement()->StopMovementImmediately();
		Speech->SetMood(.55f, 0.f);
		QueueSpeech(DemandVoice);
	}
	if (Phase==EPhase::Demand)
	{
		const bool bGold=Game.Inventory.Equipped==TEXT("Gold");
		if (bGold) NoticeGold();
		MoveBandit(FMath::VInterpConstantTo(BanditRoot->GetComponentLocation(),BanditMark,Delta,160),P);
		if (P.X>BanditMark.X-320)
		{
			AimTime+=Delta;
			Game.ThreatCue=TEXT("“One more step—” The muzzle follows you.");
			CueTime=0;
			if (AimTime>.67f || P.X>BanditMark.X+20)
			{
				UGameplayStatics::PlaySoundAtLocation(this,Shot,BanditRoot->GetComponentLocation(),.8f);
				Game.Die(TEXT("Bandit")); return;
			}
		}
		else AimTime=0;
		Game.InteractionPrompt=bGold ? TEXT("F  Throw gold") : TEXT("The stranger raises his gun and points at your belongings.");
		return;
	}
	if (Phase!=EPhase::Diversion) return;
	DiversionTime+=Delta;
	if (!bPouchLanded)
	{
		AdvancePouch(Pawn,Delta);
		if (bPouchLanded && bWolfIntercept && TimeUntilBanditFires<0.f)
		{
			Bandit->PlayAnimation(WalkAnim,true);
			QueueSpeech(StayVoice);
		}
	}
	if (AttackTime<0.f)
	{
		const bool bBreakingAway=IsBreakingAway(P);
		// The catch must actually happen. If the wolf cannot reach the
		// robber, he finishes collecting the money and turns back to Max.
		const bool bFinishedCollecting=!bInCreek && DiversionTime>FMath::Max(PackRouteSeconds,18.f);
		if (TimeUntilBanditFires>=0.f || (bPouchLanded && !bWolfIntercept) || bBreakingAway || bFinishedCollecting)
		{
			CoverMax(Game,Pawn,Delta,bBreakingAway);
			return;
		}
		if (!bPouchLanded) return;
		const FVector Goal=GoldLanding+FVector(0,42,-5);
		FVector Position=FMath::VInterpConstantTo(BanditRoot->GetComponentLocation(),Goal,Delta,320);
		if (FVector::DistSquared2D(Position,Goal)>100) MoveBandit(Position,Goal);
		else
		{
			BanditRoot->SetWorldLocation(Goal);
			// The bent-knee part of the landing clip reads as a low stoop.
			const auto* Pose=Bandit->GetSingleNodeInstance();
			if (!Pose || Pose->GetAnimationAsset()!=StoopAnim) Bandit->PlayAnimation(StoopAnim,false);
			Bandit->SetPosition(.13f+.055f*FMath::Sin(DiversionTime*3.f),false);
		}
		UWolfMeshComponent* Leader = Pack->GetLeadWolf();
		if (bWolfIntercept && FVector::Dist2D(Position, Goal) <= 10.f && Clock > 11.f &&
			Leader && FVector::Dist2D(Leader->GetComponentLocation(), Goal) < 525.f &&
			PendingSpeech.IsEmpty() && !Speech->IsDelivering())
		{
			BeginBanditAttack(Pawn);
		}
	}
	else
	{
		AttackTime += Delta;
		if (!bBackOnTrail) AdvanceBanditAttack(Delta);
		if (bBackOnTrail)
		{
			Game.Die(TEXT("Wolves"));
			if (Game.IsDead()) return;
		}
	}
	if (bAtBend && Game.CommitCheckpoint(5,Escape,FRotator(0,14,0)))
	{
		Phase=EPhase::Escaped;
		StageCue(TEXT("A familiar howl rises behind you."),4);
		// Play one short call from the source loop, then release this transient sound.
		if (auto* Call=UGameplayStatics::SpawnSoundAtLocation(this,Howl,GoldLanding,FRotator::ZeroRotator,.3f))
			Call->FadeOut(3.5f,0);
	}
}
