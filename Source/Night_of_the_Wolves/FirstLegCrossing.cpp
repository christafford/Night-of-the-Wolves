#include "FirstLegCrossing.h"
#include "DialogueVoiceComponent.h"
#include "WolfPackComponent.h"
#include "FirstLegCrossingLayout.h"
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
bool Within(const FVector& A,const FVector& B,float Radius)
{
	return FVector::Dist2D(A,B)<Radius && FMath::Abs(A.Z-B.Z)<235;
}
}

void AFirstLegCrossing::TraversalMovement(APawn& Pawn,bool bActive)
{
	ACharacter* Character=Cast<ACharacter>(&Pawn);
	if (!Character) return;
	auto* Move=Character->GetCharacterMovement();
	Move->StopMovementImmediately();
	Move->SetMovementMode(bActive ? MOVE_None : MOVE_Falling);
	if (auto* Boom=Pawn.FindComponentByClass<USpringArmComponent>())
	{
		if (bActive)
		{
			bPreviousBoomCollision=Boom->bDoCollisionTest;
			PreviousBoomLength=Boom->TargetArmLength;
			PreviousBoomOffset=Boom->TargetOffset;
			// Look over the bank from above throughout the fixed diagonal arc.
			Boom->bDoCollisionTest=false;
			Boom->TargetArmLength=340;
			Boom->TargetOffset+=FVector(0,0,110);
		}
		else
		{
			Boom->bDoCollisionTest=bPreviousBoomCollision;
			Boom->TargetArmLength=PreviousBoomLength;
			Boom->TargetOffset=PreviousBoomOffset;
		}
	}
	auto* Body=Character->GetMesh();
	auto* Max=Cast<ANight_of_the_WolvesCharacter>(Character);
	if (!bActive)
	{
		Body->SetVisibility(true);
		if (Max) Max->SetHangingAppearance(nullptr);
		if (IsValid(HangingPose)) HangingPose->DestroyComponent();
		HangingPose=nullptr;
		return;
	}
	// Pose both hands around the rope. Keep the regular locomotion animation
	// instance intact so the landing immediately resumes normal movement.
	HangingPose=NewObject<UPoseableMeshComponent>(Character);
	HangingPose->SetupAttachment(Body);
	HangingPose->SetSkinnedAssetAndUpdate(Body->GetSkinnedAsset());
	HangingPose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HangingPose->RegisterComponent();
	for(int32 i=0;i<Body->GetNumMaterials();++i) HangingPose->SetMaterial(i,Body->GetMaterial(i));
	HangingPose->CopyPoseFromSkeletalComponent(Body);
	const auto Space=EBoneSpaces::WorldSpace;
	auto AimBone=[&](FName Bone,FName Child,const FVector& Goal)
	{
		const FVector Root=HangingPose->GetBoneLocationByName(Bone,Space);
		const FVector Dir=(HangingPose->GetBoneLocationByName(Child,Space)-Root).GetSafeNormal();
		const FQuat Delta=FQuat::FindBetweenNormals(Dir,(Goal-Root).GetSafeNormal());
		const FQuat Rot=HangingPose->GetBoneTransformByName(Bone,Space).GetRotation();
		HangingPose->SetBoneRotationByName(Bone,(Delta*Rot).Rotator(),Space);
	};
	for(bool bLeft : {true,false})
	{
		const FVector Grip=Pawn.GetActorLocation()+Pawn.GetActorForwardVector()*12+FVector(0,0,bLeft?98:109);
		const FVector Elbow=Grip+Pawn.GetActorRightVector()*(bLeft?-19:19)-FVector(0,0,19);
		const FName Upper=bLeft?TEXT("upperarm_l"):TEXT("upperarm_r");
		const FName Fore=bLeft?TEXT("lowerarm_l"):TEXT("lowerarm_r");
		AimBone(Upper,Fore,Elbow);
		AimBone(Fore,bLeft?TEXT("hand_l"):TEXT("hand_r"),Grip);
	}
	HangingPose->RefreshBoneTransforms();
	if (Max) Max->SetHangingAppearance(HangingPose);
	Body->SetVisibility(false);
}

bool AFirstLegCrossing::IsTraversalLocked() const
{
	return Phase==EPhase::Swinging || Phase==EPhase::Landing || Phase==EPhase::Missed;
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

void AFirstLegCrossing::StopSpeech()
{
	PendingSpeech.Reset();
	if (Speech) Speech->ResetPerformance();
}

void AFirstLegCrossing::ShowRope(const FVector& End)
{
	Cord->SetVisibility(true);
	Cord->SetWorldLocation((Anchor+End)*.5);
	Cord->SetWorldRotation(FRotationMatrix::MakeFromZ(Anchor-End).Rotator());
	Cord->SetWorldScale3D({.057,.057,FVector::Distance(Anchor,End)/100});
}

void AFirstLegCrossing::MoveBandit(const FVector& Position,const FVector& Facing)
{
	BanditRoot->SetWorldLocation(Position);
	const FVector D=Facing-Position;
	BanditRoot->SetWorldRotation(FRotator(0,D.Rotation().Yaw,0));
}

void AFirstLegCrossing::Restore(int32 Checkpoint)
{
	Phase=Checkpoint>=5 ? EPhase::Escaped : Checkpoint==4 ? EPhase::FarBank :
		  Checkpoint==3 ? EPhase::Ready : EPhase::Approach;
	ActionTime=BankTime=PackTime=DiversionTime=AimTime=CueTime=0;
	AttackTime=-1;
	bPouchLanded=bReachedBend=bGoldDemandSpoken=false;
	StopSpeech();
	Cue.Empty();
	if (!Cord) return;
	MoveBandit(BanditMark+FVector(0,230,0),BanditMark-FVector(500,0,0));
	Bandit->PlayAnimation(AimAnim,true);
	if (Checkpoint>=4) ShowRope(Takeoff+FVector(0,0,40)); else Cord->SetVisibility(false);
	GoldLanding=GoldPatch;
	if (Checkpoint>=5)
	{
		Bag->SetWorldLocation(GoldLanding+FVector(0,0,12));
		MoveBandit(GoldLanding,GoldLanding-FVector(200,0,0));
		Bandit->PlayAnimation(DeathAnim,false);
		Bandit->SetPosition(DeathAnim->GetPlayLength()-.02f,false);
	}
	Bag->SetVisibility(Checkpoint>=5);
	for (int32 I=0; I<SpilledCoins.Num(); ++I)
	{
		SpilledCoins[I]->SetWorldLocation(GoldLanding+FVector(9*I-34,20*FMath::Sin(I*2.2),0));
		SpilledCoins[I]->SetVisibility(Checkpoint>=5);
	}
	SetRing(false);
	Pack->Hide();
	LastLead=Checkpoint>=4 ? FVector(9660,-1820,50) : FVector(8310,-1180,50);
	AttackOrigin=LastLead;
	if (Checkpoint==3 || Checkpoint==4) UpdatePack(0,LastLead,Checkpoint==3 ? CreekApproach : Landing);
#if !UE_BUILD_SHIPPING
	TestTime=0; TestWaypoint=0;
#endif
}

UWolfMeshComponent* AFirstLegCrossing::GetLeadWolf() const { return Pack->GetLeadWolf(); }

void AFirstLegCrossing::UpdatePack(float Delta,const FVector& Lead,const FVector& Quarry,bool bSwimming)
{
	const FVector D = (Lead-LastLead).GetSafeNormal2D();
	const FVector ToQuarry = (Quarry-Lead).GetSafeNormal2D();
	const FVector Heading = D.IsNearlyZero() ? ToQuarry : D;
	const float Gap = FVector::Dist2D(Quarry,Lead);
	const float Threat = bSwimming ? 0.f : 1.f-FMath::SmoothStep(200.f,775.f,Gap);
	// The rules clock reaches its contact radius first. Behind it the lead wolf
	// crouches a little farther out, with enough space to spring onto Max.
	const FVector VisibleLead = Lead - ToQuarry*Threat*70.f;
	Pack->Hunt(Delta,VisibleLead,Heading,Quarry,Threat,bSwimming);
	LastLead = Lead;
}

void AFirstLegCrossing::SetRing(bool bVisible)
{
	for (int32 I=0;I<TargetRing.Num();++I)
	{
		const float Angle=I*2*PI/TargetRing.Num();
		TargetRing[I]->SetWorldLocation(GoldLanding+FVector(48*FMath::Cos(Angle),48*FMath::Sin(Angle),2));
		TargetRing[I]->SetVisibility(bVisible);
	}
}

bool AFirstLegCrossing::IsAimingAtGold(const APawn& Pawn) const
{
	if (!Within(Pawn.GetActorLocation(),GoldLanding,540)) return false;
	const FVector ToGold=(GoldLanding-Pawn.GetActorLocation()).GetSafeNormal2D();
	const FVector Aim=Pawn.GetControlRotation().Vector().GetSafeNormal2D();
	return FVector::DotProduct(ToGold,Aim)>.58f;
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
	if (!IsTraversalLocked() && (IsInsideWater(P) || (P.X>8450 && P.Z<-350)))
	{
		Game.Inventory.Extinguish(); Game.Die(TEXT("Current")); return true;
	}
	// This protects the supply route even if the user jumps off outside the dressed banks.
	if (Phase<=EPhase::Attached && P.X>10870+StreamBend(P.Y))
	{
		Game.Inventory.Extinguish(); Game.Die(TEXT("Current")); return true;
	}
	if (Phase==EPhase::Approach)
	{
		if (P.X<8950) return false;
		if (!Game.Inventory.Has(TEXT("Rope")) || !Game.Inventory.Has(TEXT("Gold")))
		{
			Game.InteractionPrompt=TEXT("The span is gone. A rope could reach that limb.");
			return false;
		}
		if (!Within(P,CreekApproach,510)) return false;
		if (!Game.CommitCheckpoint(3,CreekApproach,FRotator(0,9,0))) return false;
		Phase=EPhase::Ready;
		StageCue(TEXT("Broken timbers twist in the current."));
	}
	if (P.X >= 4000) Game.InteractionPrompt.Empty();
	Game.ThreatCue.Empty();
	CueTime-=Delta;
	if (Phase>=EPhase::FarBank)
	{
		UpdateFarBank(Game,Pawn,Delta);
		if (CueTime>0) Game.ThreatCue=Cue;
		return true;
	}
	const bool bFire=Game.Inventory.Equipped==TEXT("Firebrand") && Game.Inventory.bBurning;
	BankTime+=Delta*(bFire ? .49f : 1.f);
	const FVector PlayerFeet=P-FVector(0,0,94);
	// While Max swings, wolves can reach the lip of the bank and wait there.
	const FVector Hunted=IsTraversalLocked() ? Takeoff-FVector(0,0,94) : PlayerFeet;
	if (BankTime<NearBankSeconds)
	{
		UpdatePack(Delta,FMath::Lerp(FVector(8310,-1180,50),FVector(9540,-1190,50),BankTime/NearBankSeconds),Hunted);
	}
	else
	{
		UpdatePack(Delta,FMath::VInterpConstantTo(LastLead,Hunted,Delta,650),Hunted);
	}
	if (FVector::Dist2D(PlayerFeet,LastLead)<110 && FMath::Abs(PlayerFeet.Z-LastLead.Z)<150)
	{
		Game.Die(TEXT("Wolves"));
		return true;
	}
	Game.ThreatCue=BankTime>NearBankSeconds*.71 ? TEXT("Paws scrape the near bank.") :
		bFire ? TEXT("The pack hangs back from the flame.") : TEXT("Growling behind you.");
	if (CueTime>0) Game.ThreatCue=Cue;
	if (P.X < 4000) return true;
	if (IsTraversalLocked())
	{
		UpdateSwing(Game,Pawn,Delta); return true;
	}
	const bool bInReach=Within(P,Takeoff,310);
	if (Phase==EPhase::Casting)
	{
		if (!bInReach || Game.Inventory.Equipped!=TEXT("Rope"))
		{
			Phase=EPhase::Ready; Cord->SetVisibility(false);
			StageCue(TEXT("The loose rope falls back into your hands.")); return true;
		}
		ActionTime+=Delta;
		const float T=FMath::Min(ActionTime/RopeCastSeconds,1.f);
		const FVector Hand=P+FVector(0,0,45);
		const FVector Tip=FMath::Lerp(Hand,Anchor,T)+FVector(0,0,175*FMath::Sin(PI*T));
		Cord->SetVisibility(true); Cord->SetWorldLocation((Hand+Tip)*.5);
		Cord->SetWorldRotation(FRotationMatrix::MakeFromZ(Tip-Hand).Rotator());
		Cord->SetWorldScale3D({.057,.057,FVector::Distance(Hand,Tip)/100});
		Game.InteractionPrompt=TEXT("Securing the rope…");
		if (T>=1 && Game.Inventory.Consume(TEXT("Rope")))
		{
			Phase=EPhase::Attached; ShowRope(Takeoff+FVector(0,0,40));
			StageCue(TEXT("The rope catches and holds."));
		}
		return true;
	}
	if (bInReach)
		Game.InteractionPrompt=Phase==EPhase::Attached ? TEXT("E  Grip rope and swing") :
			Game.Inventory.Equipped==TEXT("Rope") ? TEXT("F  Cast rope over the limb") : TEXT("A rope could reach the limb above the water.");
	else Game.InteractionPrompt=TEXT("The leaning tree reaches the opposite shelf.");
	return true;
}

void AFirstLegCrossing::UpdateSwing(AWolfTrailGameMode& Game,APawn& Pawn,float Delta)
{
	ActionTime+=Delta;
	if (Phase==EPhase::Swinging)
	{
		// A pendulum in the diagonal plane through the two banks.
		const FVector Axis=(Landing-Takeoff).GetSafeNormal2D();
		const float HalfSpan=FVector::Dist2D(Takeoff,Landing)*.5;
		const float Height=Anchor.Z-Takeoff.Z;
		const float Angle=-FMath::Atan2(HalfSpan,Height)*FMath::Cos(PI*ActionTime/OutwardSwingSeconds);
		const float Radius=FMath::Sqrt(HalfSpan*HalfSpan+Height*Height);
		const FVector Position=Anchor+Axis*(Radius*FMath::Sin(Angle))-FVector(0,0,Radius*FMath::Cos(Angle));
		Pawn.SetActorLocation(Position);
		ShowRope(IsValid(HangingPose) ? HangingPose->GetSocketLocation(TEXT("hand_l")) : Position+FVector(0,0,95));
		const float SwingFraction=ActionTime/OutwardSwingSeconds;
		Game.InteractionPrompt=SwingFraction<.53f ? TEXT("Swing toward the moonlit shelf") :
			SwingFraction<=1.18f ? TEXT("E  Release over the far shelf") : TEXT("The rope swings back to the broken bridge.");
		if (ActionTime>=2*OutwardSwingSeconds)
		{
			Pawn.SetActorLocation(Takeoff); TraversalMovement(Pawn,false);
			Phase=EPhase::Attached; ShowRope(Takeoff+FVector(0,0,40));
			StageCue(TEXT("The swing carries you back."));
		}
		return;
	}
	const float T=FMath::Min(ActionTime/.64f,1.f);
	const bool bLanding=Phase==EPhase::Landing;
	const FVector Target=bLanding ? Landing :
		FVector(FMath::Clamp(ReleaseLocation.X+90.,10010.,10720.),ReleaseLocation.Y,-160);
	ShowRope(FMath::Lerp(ReleaseLocation,Takeoff,T)+FVector(0,0,40));
	Pawn.SetActorLocation(FMath::Lerp(ReleaseLocation,Target,T)+FVector(0,0,bLanding ? 90*FMath::Sin(PI*T) : 0));
	if (T>=1)
	{
		if (bLanding) FinishLanding(Game,Pawn);
		else {Game.Inventory.Extinguish();Game.Die(TEXT("Current"));}
	}
}

void AFirstLegCrossing::FinishLanding(AWolfTrailGameMode& Game,APawn& Pawn)
{
	if (!Game.Inventory.Has(TEXT("Gold"))) {Game.Die(TEXT("Current"));return;}
	TraversalMovement(Pawn,false);
	ShowRope(Takeoff+FVector(0,0,40));
	Phase=EPhase::FarBank; PackTime=0; AttackOrigin=LastLead;
	Game.CommitCheckpoint(4,Landing,FRotator(0,0,0));
	StageCue(TEXT("Behind you, the first wolf tests the water."),3);
}

bool AFirstLegCrossing::TryAction(AWolfTrailGameMode& Game,APawn& Pawn,bool bItemUse)
{
	if (Game.IsDead() || Phase==EPhase::Approach) return false;
	// A player can still return to the fireplace while the pack is behind them.
	if (Phase<=EPhase::Attached && Pawn.GetActorLocation().X < 4000) return false;
	if (Phase==EPhase::Swinging)
	{
		if (bItemUse) return true;
		const float T=ActionTime/OutwardSwingSeconds;
		Phase=(T>=.53f && T<=1.18f) ? EPhase::Landing : EPhase::Missed;
		ReleaseLocation=Pawn.GetActorLocation();
		ActionTime=0;
		return true;
	}
	if (Phase==EPhase::Ready && Within(Pawn.GetActorLocation(),Takeoff,310))
	{
		if (bItemUse && Game.Inventory.Equipped==TEXT("Rope") && Game.Inventory.Has(TEXT("Gold")))
		{
			Phase=EPhase::Casting; ActionTime=0;
		}
		else StageCue(TEXT("The high limb needs a length of rope."));
		return true;
	}
	if (Phase==EPhase::Attached && !bItemUse && Within(Pawn.GetActorLocation(),Takeoff,310))
	{
		Phase=EPhase::Swinging; ActionTime=0;
		Pawn.SetActorRotation(FRotator(0,(Landing-Takeoff).Rotation().Yaw,0));
		TraversalMovement(Pawn,true);
		if (AController* Controller=Pawn.GetController()) Controller->SetControlRotation(FRotator(-16,(Landing-Takeoff).Rotation().Yaw,0));
		return true;
	}
	if (Phase==EPhase::Demand && bItemUse)
	{
		if (Game.Inventory.Equipped!=TEXT("Gold"))
		{
			StageCue(TEXT("His gun stays raised. He gestures at your bag.")); return true;
		}
		NoticeGold();
		if (!IsAimingAtGold(Pawn))
		{
			StageCue(TEXT("Look back at the marked patch to toss the sack.")); return true;
		}
		if (Game.Inventory.Consume(TEXT("Gold")))
		{
			Phase=EPhase::Diversion; DiversionTime=0; TossOrigin=Pawn.GetActorLocation()+FVector(0,0,25);
			Bag->SetWorldLocation(TossOrigin); Bag->SetVisibility(true); SetRing(false);
			Bandit->PlayAnimation(WalkAnim,true);
			QueueSpeech(StayVoice);
		}
		return true;
	}
	return true;
}

void AFirstLegCrossing::UpdateFarBank(AWolfTrailGameMode& Game,APawn& Pawn,float Delta)
{
	if (Phase==EPhase::Escaped) return;
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
	// The same silhouettes approach, wade in, swim downstream and climb onto this bank.
	static const FVector Route[]={{9690,-1830,50},{9780,-1940,12},{10010,-2070,-248},{10780,-2220,-248},
		{11070,-2180,30},{11580,-1020,50},{12780,-930,50},{13420,-765,50}};
	static const float Arrival[]={3.0f,4.5f,6.f,9.f,11.f,14.5f,19.5f,22.f};
	FVector Lead;
	if (Clock<Arrival[UE_ARRAY_COUNT(Arrival)-1])
	{
		int32 I=0;
		while (I+1<UE_ARRAY_COUNT(Arrival) && Clock>=Arrival[I]) ++I;
		const FVector Start=I==0 ? AttackOrigin : Route[I-1];
		const float StartTime=I==0 ? 0 : Arrival[I-1];
		Lead=FMath::Lerp(Start,Route[I],(Clock-StartTime)/(Arrival[I]-StartTime));
	}
	else
	{
		const FVector Goal=Phase==EPhase::Diversion ? GoldLanding : P-FVector(0,0,94);
		Lead=FMath::VInterpConstantTo(LastLead,Goal,Delta,420);
	}

	// If Max waits near the water the pack turns toward him after climbing out.
	if (Clock>11.1f && P.X<12250 && Phase!=EPhase::Diversion)
		Lead=FMath::VInterpConstantTo(LastLead,P-FVector(0,0,94),Delta,570);
	if (Phase==EPhase::Diversion && AttackTime>=0)
	{
		const bool bHunt=DiversionTime>DiversionSeconds && AttackTime>3.f && !bReachedBend;
		Lead=FMath::VInterpConstantTo(LastLead,(bHunt ? P-FVector(0,0,94) : GoldLanding),Delta,690);
	}
	const bool bSeizingBandit = Phase==EPhase::Diversion && (DiversionTime<=DiversionSeconds || AttackTime<3.f);
	UpdatePack(Delta,Lead,bSeizingBandit ? GoldLanding : P,Clock>5 && Clock<10);
	if (Phase!=EPhase::Diversion && Clock>11 && FVector::Dist2D(P,Lead)<120)
	{
		Game.Die(TEXT("Wolves")); return;
	}
	Game.ThreatCue=Clock<5 ? TEXT("The pack gathers at the broken bridge.") :
		Clock<10 ? TEXT("Downstream, dark heads break the surface.") : TEXT("Wet paws scramble up the bank behind you.");
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
		SetRing(bGold);
		Game.InteractionPrompt=bGold ? (IsAimingAtGold(Pawn) ? TEXT("F  Toss the sack onto the moonlit patch") :
			TEXT("Look behind you for the moonlit patch.")) : TEXT("The stranger raises his gun and points at your belongings.");
		return;
	}
	if (Phase!=EPhase::Diversion) return;
	DiversionTime+=Delta;
	if (!bPouchLanded)
	{
		const float T=FMath::Min(DiversionTime/.67f,1.f);
		Bag->SetWorldLocation(FMath::Lerp(TossOrigin,GoldLanding+FVector(0,0,12),T)+FVector(0,0,110*FMath::Sin(PI*T)));
		if (T>=1)
		{
			bPouchLanded=true;
			UGameplayStatics::PlaySoundAtLocation(this,Coins,GoldLanding,.85f);
			for (int32 I=0;I<SpilledCoins.Num();++I)
			{
				SpilledCoins[I]->SetWorldLocation(GoldLanding+FVector(9*I-34,20*FMath::Sin(I*2.2),0));
				SpilledCoins[I]->SetVisibility(true);
			}
		}
	}
	if (AttackTime<0)
	{
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
		if (Clock>11 && FVector::Dist2D(Lead,BanditRoot->GetComponentLocation())<170)
		{
			AttackTime=0;
			StopSpeech();
			Bandit->PlayAnimation(DeathAnim,false);
			UGameplayStatics::PlaySoundAtLocation(this,Shot,BanditRoot->GetComponentLocation(),.8);
			StageCue(TEXT("A cry at the bend. One wild shot."),3.5);
		}
	}
	else
	{
		AttackTime+=Delta;
		if (DiversionTime>DiversionSeconds && AttackTime>3 && FVector::Dist2D(P,Lead)<150)
		{
			Game.Die(TEXT("Wolves")); return;
		}
	}
	bReachedBend=P.X>=Escape.X && FMath::Abs(P.Y-Escape.Y)<1300 && P.Z>20;
	if (bReachedBend && AttackTime>=.4f && Game.CommitCheckpoint(5,Escape,FRotator(0,14,0)))
	{
		Phase=EPhase::Escaped;
		StageCue(TEXT("A familiar howl rises behind you."),4);
		// Play one short call from the source loop, then release this transient sound.
		if (auto* Call=UGameplayStatics::SpawnSoundAtLocation(this,Howl,GoldLanding,FRotator::ZeroRotator,.3f))
			Call->FadeOut(3.5f,0);
	}
}
