#include "WolfTrailGameMode.h"
#include "StormfallLayout.h"
#include "HighWaterChamber.h"
#include "StormfallSet.h"
#include "WolfPackComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/AudioComponent.h"

namespace
{
using namespace StormfallLayout;

FVector FeetOf(const APawn& Pawn)
{
	return Pawn.GetActorLocation() - FVector::UpVector*Pawn.GetSimpleCollisionHalfHeight();
}

bool ByBurningWood(const FVector& Feet)
{
	return FVector::DistSquared2D(Feet, Relight) < FMath::Square(165.) && FMath::Abs(Feet.Z-Relight.Z)<100.;
}

bool AtRootFace(const FVector& Feet)
{
	const FVector Local = LocalPoint(Feet);
	return FMath::Abs(Local.X-499.)<120. && Local.Y>=45. && Local.Y<245. && FMath::Abs(Local.Z-11.)<85.;
}
}

bool AWolfTrailGameMode::AreCaveRootsClear() const
{
	return RootBurnTime >= RootBurnSeconds;
}

void AWolfTrailGameMode::ResetStormfall(int32 Id)
{
	const bool bCleared = Id >= 6;
	StrikeTime = bCleared ? StormfallLayout::Settled : -1.f;
	BendTime = 0.f;
	ForestLeak = -1.f;
	RootBurnTime = bCleared ? RootBurnSeconds : -1.f;
	bTreeFlame = bInsideCave = bThroughRoots = bCleared;
	Storm->Restore(bCleared,bCleared);
}

void AWolfTrailGameMode::StormAction(APawn& Max, bool bUseItem)
{
	using namespace StormfallLayout;
	const FVector Feet = FeetOf(Max);
	if (StrikeTime >= GroundImpact && !SurveyChamber->HasMountedTorch() && ByBurningWood(Feet))
	{
		// The original torch normally comes from the cabin. A save from an
		// earlier build can legitimately lack it; there is wood here to take.
		const bool bPickUp = !bUseItem && !Inventory.Has(TEXT("Firebrand"));
		const bool bLight = bUseItem && Inventory.Equipped==TEXT("Firebrand");
		if ((bLight || bPickUp) && !HasTreeFlame())
		{
			Inventory.Ignite();
			bTreeFlame = true;
			Say(TEXT("Flame catches on the blackened wood."));
		}
		return;
	}
	if (bUseItem && bInsideCave && AtRootFace(Feet) && RootBurnTime<0.f && HasTreeFlame())
	{
		RootBurnTime = 0.f;
		Storm->SetPuzzlePose(0.f);
		Say(TEXT("A thread of orange crawls up the roots."));
	}
}

void AWolfTrailGameMode::UpdateStormfall(float Dt, APawn& Max)
{
	using namespace StormfallLayout;
	const FVector Feet = FeetOf(Max);
	if (Feet.Z < -380.) { Die(TEXT("The ground falls away")); return; }
	BendTime += Dt;
	// Crossing the bend line works even if Max leaves the center of the tread.
	if (StrikeTime < 0 && Feet.X >= StrikeTrigger.X)
	{
		StrikeTime = 0.f;
		Inventory.Extinguish();
		Storm->SetStormPose(0.f);
		UE_LOG(LogTemp,Display,TEXT("STORMFALL lightning at bend"));
	}
	if (StrikeTime < 0)
	{
		const FVector Heading=FVector(1,.25,0).GetSafeNormal();
		const bool bExposed=BendTime>5.f;
		HuntingPack->Hunt(Dt,Feet,Heading,bExposed ? 100.f : 520.f,EWolfGroundEncounter::FarBank,bExposed);
		if (bExposed) PresentWolfCapture();
		return;
	}
	if (StrikeTime < Settled)
	{
		StrikeTime=FMath::Min(StrikeTime+Dt,Settled);
		Storm->SetStormPose(StrikeTime);
	}
	// Lightning arrests the hunt. As soon as the wood catches, the same
	// five animals break downhill and stay in cover while Max investigates.
	if (StrikeTime < GroundImpact)
		HuntingPack->FaceFire(Dt,Relight);
	else
		HuntingPack->FleeFire(Dt,Relight,Checkpoint>=6);
	if (RootBurnTime>=0 && !AreCaveRootsClear())
	{
		RootBurnTime=FMath::Min(RootBurnSeconds,RootBurnTime+Dt);
		Storm->SetPuzzlePose(RootBurnTime/RootBurnSeconds);
	}

	const FVector Local=LocalPoint(Feet);
	// The outdoor loops are 2D. Let the rock muffle them so a knock from
	// the pool carries, and bring them back if Max retraces his steps.
	if (Checkpoint >= 6)
	{
		const float Leak = 1.f - .96f*FMath::SmoothStep(480.f,1450.f,float(Local.Y));
		if (!FMath::IsNearlyEqual(Leak,ForestLeak,.01f))
		{
			if (HowlLoop) HowlLoop->SetVolumeMultiplier(WolfHowlVolume*Leak);
			if (WindLoop) WindLoop->SetVolumeMultiplier(ForestWindVolume*Leak);
			ForestLeak=Leak;
		}
	}
	const bool bFloorHeight=Local.Z>-65. && Local.Z<95.;
	if (bFloorHeight && Local.X>-28. && Local.X<160. && FMath::Abs(Local.Y)<155.)
		bInsideCave=true;
	if (Local.X< -45.) bInsideCave=false;
	if (bInsideCave && AreCaveRootsClear() && bFloorHeight && Local.X>330. && Local.X<690. && Local.Y>300. && Local.Y<465.)
		bThroughRoots=true;
	if (Checkpoint==5 && bThroughRoots && bFloorHeight && FVector::DistSquared2D(Feet,Outlet)<FMath::Square(115.))
	{
		if (CommitCheckpoint(6,Outlet+FVector(0,0,Max.GetSimpleCollisionHalfHeight()+3.f),OutletFacing))
			Say(TEXT("Cool air moves up from below."),4.f);
	}

	if (StrikeTime>=GroundImpact && !SurveyChamber->HasMountedTorch() && ByBurningWood(Feet))
	{
		if (!Inventory.Has(TEXT("Firebrand"))) InteractionPrompt=TEXT("E  Take burning branch");
		else if (Inventory.Equipped==TEXT("Firebrand") && !HasTreeFlame()) InteractionPrompt=TEXT("F  Light firebrand");
		else InteractionPrompt=TEXT("The split heartwood glows.");
	}
	else if (bInsideCave && AtRootFace(Feet) && RootBurnTime<0)
		InteractionPrompt=HasTreeFlame() ? TEXT("F  Touch flame to roots") : TEXT("Something dry brushes your sleeve.");
}
