#include "FirstLegCrossing.h"
#include "FirstLegCrossingLayout.h"
#include "WolfMeshComponent.h"
#include "WolfPackComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
struct FRoadPoint
{
	double Along = 0.;
	double FromCenter = TNumericLimits<double>::Max();
};

// Distance along the bending road puts the pack's approach on the correct
// side of Max even when he moves across the clearing before throwing.
FRoadPoint ProjectToRoad(FVector Point)
{
	using namespace FirstLegLayout;
	FRoadPoint Nearest;
	double Along = 0.;
	Point.Z = 0.;
	for (int32 I=1; I<UE_ARRAY_COUNT(FarRoad); ++I)
	{
		const FVector A(FarRoad[I-1].X,FarRoad[I-1].Y,0.);
		const FVector B(FarRoad[I].X,FarRoad[I].Y,0.);
		const FVector Foot = FMath::ClosestPointOnSegment(Point,A,B);
		const double Across = FVector::Dist(Point,Foot);
		if (Across < Nearest.FromCenter)
			Nearest = {Along + FVector::Dist(A,Foot), Across};
		Along += FVector::Dist(A,B);
	}
	return Nearest;
}
}

void AFirstLegCrossing::ThrowPouch(const APawn& Pawn)
{
	const FRotator Aim = Pawn.GetControlRotation();
	const FVector Forward = FRotator(0.,Aim.Yaw,0.).Vector();
	TossOrigin = Pawn.GetActorLocation();
	TossVelocity = Forward * (GoldThrowSpeed * FMath::Max(.42, Aim.Vector().Size2D()));
	TossVelocity.Z = 210. + GoldThrowSpeed*.7*Aim.Vector().Z;
	Bag->SetWorldLocation(TossOrigin + Forward*24. + FVector::UpVector*24.);
	Bag->SetVisibility(true);
	Phase = EPhase::Diversion;
	DiversionTime = AimTime = 0.f;
	bPouchLanded = bWolfIntercept = false;
}

bool AFirstLegCrossing::IsOnPackApproach() const
{
	// A close throw in front of a wolf can also intercept a flanker which
	// has moved off the tread. Swimming wolves cannot reach the loose bag.
	for (int32 I=0; I<5; ++I)
	{
		const UWolfMeshComponent* Wolf = Pack->GetMember(I);
		if (!Wolf) continue;
		const FVector Feet = Wolf->GetComponentLocation();
		if (Feet.X>12100. && FVector::Dist2D(Feet,GoldLanding)<190. &&
			FVector::Dist2D(TossOrigin,Feet)>FVector::Dist2D(Feet,GoldLanding)+100.)
			return true;
	}
	const FRoadPoint Loot = ProjectToRoad(GoldLanding);
	const FRoadPoint Thrower = ProjectToRoad(TossOrigin);
	return Loot.FromCenter <= PackInterceptWidth && Loot.Along + 145. < Thrower.Along;
}

void AFirstLegCrossing::AdvancePouch(const APawn& Pawn, float Delta)
{
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ThrownGold),false,&Pawn);
	// Use the same collision as Max's feet so the bag lands on the actual
	// rocks and tread. Only the thrower is ignored; the road belongs to this actor.
	while (Delta>KINDA_SMALL_NUMBER && !bPouchLanded)
	{
		const float Step = FMath::Min(Delta,1.f/45.f);
		Delta -= Step;
		const FVector Start = Bag->GetComponentLocation();
		const FVector Gravity(0.,0.,GetWorld()->GetGravityZ());
		const FVector End = Start + TossVelocity*Step + .5*Gravity*Step*Step;
		TossVelocity += Gravity*Step;
		FHitResult Hit;
		if (!GetWorld()->SweepSingleByChannel(Hit,Start,End,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(8.f),Query))
		{
			Bag->SetWorldLocation(End);
			if (End.Z > -500.) continue;
			// Thrown over an edge. No one can collect it there.
			bPouchLanded = true;
			GoldLanding = End;
			Bag->SetVisibility(false);
			return;
		}
		if (Hit.Normal.Z < .52 && !Hit.bStartPenetrating)
		{
			Bag->SetWorldLocation(Hit.Location+Hit.Normal*2.);
			TossVelocity = FVector::VectorPlaneProject(TossVelocity,Hit.Normal)*.55;
			continue;
		}
		bPouchLanded = true;
		GoldLanding = Hit.Location - FVector::UpVector*8.;
		Bag->SetWorldLocation(GoldLanding + FVector::UpVector*10.);
		bWolfIntercept = IsOnPackApproach();
		UGameplayStatics::PlaySoundAtLocation(this,Coins,GoldLanding,.85f);
		for (int32 I=0; I<SpilledCoins.Num(); ++I)
		{
			SpilledCoins[I]->SetWorldLocation(GoldLanding+FVector(9*I-34,20*FMath::Sin(I*2.2),1));
			SpilledCoins[I]->SetVisibility(true);
		}
		UE_LOG(LogTemp,Display,TEXT("BANDIT pouch landed at %s, from %s, pack intercept %d"),*GoldLanding.ToString(),*TossOrigin.ToString(),bWolfIntercept);
	}
}
