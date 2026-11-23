#include "WolfPackComponent.h"
#include "WolfMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Misc/App.h"

void UWolfPackComponent::SpawnMembers()
{
	if (!Wolves.IsEmpty()) return;
	if (FApp::CanEverRender() && MoonBounceCandelas > 0.f)
	{
		// A broad patch of reflected sky across the clearing gives the brow, eyes and
		// muzzle a soft cross-light. Restrict it to the wolves' direct-light channel.
		MoonBounce = NewObject<UPointLightComponent>(GetOwner(), TEXT("WolfMoonBounce"));
		MoonBounce->SetupAttachment(GetOwner()->GetRootComponent());
		MoonBounce->SetMobility(EComponentMobility::Movable);
		MoonBounce->SetLightingChannels(false,true,false);
		MoonBounce->SetIntensityUnits(ELightUnits::Candelas);
		MoonBounce->SetIntensity(MoonBounceCandelas);
		MoonBounce->SetLightColor(FLinearColor(.72f,.84f,1.f));
		MoonBounce->SetSourceRadius(32.f);
		MoonBounce->SetSoftSourceRadius(54.f);
		MoonBounce->SetAttenuationRadius(1050.f);
		MoonBounce->SetCastShadows(false);
		MoonBounce->SetIndirectLightingIntensity(0.f);
		MoonBounce->SetVolumetricScatteringIntensity(0.f);
		MoonBounce->SetAffectTranslucentLighting(false);
		MoonBounce->SetVisibility(false);
		MoonBounce->RegisterComponent();
	}
	for (int32 I = 0; I < 3; ++I)
	{
		const FName Label(I == 0 ? TEXT("PackLeader") : I == 1 ? TEXT("LeftFlanker") : TEXT("RightFlanker"));
		auto* Wolf = NewObject<UWolfMeshComponent>(GetOwner(), Label);
		Wolf->SetupAttachment(GetOwner()->GetRootComponent());
		Wolf->SetLightingChannels(true,true,false);
		Wolf->SetRelativeScale3D(FVector(I == 0 ? 1.08f : I == 1 ? .97f : 1.00f));
		Wolf->SetVisibility(false);
		Wolf->RegisterComponent();
		Wolf->SetScalarParameterValueOnMaterials(TEXT("CoatValue"), I == 0 ? 1.19f : I == 1 ? .82f : .96f);
		Wolf->ResetGait(.27f*I);
		Wolves.Add(Wolf);
	}
}

void UWolfPackComponent::PlaceMoonBounce(const FVector& Ground)
{
	if (!MoonBounce) return;
	// A fixed world-space offset keeps the reflection steady when Max turns to look.
	MoonBounce->SetWorldLocation(Ground + FVector(40.,-65.,195.));
	MoonBounce->SetVisibility(true);
}

void UWolfPackComponent::Hide()
{
	if (!bPlaced) return;
	if (MoonBounce) MoonBounce->SetVisibility(false);
	bPlaced = bGuarding = bCovering = false;
	QuarryVelocity = FVector::ZeroVector;
	for (int32 I=0; I<Wolves.Num(); ++I)
	{
		Wolves[I]->SetVisibility(false);
		Wolves[I]->ResetGait(I*.27f);
	}
}

UWolfMeshComponent* UWolfPackComponent::GetLeadWolf() const
{
	return bPlaced && !Wolves.IsEmpty() && Wolves[0]->IsVisible() ? Wolves[0].Get() : nullptr;
}

bool UWolfPackComponent::TraceSolidTerrain(FHitResult& Hit, const FVector& From, const FVector& To) const
{
	// Forest generation volumes are WorldStatic too. Only physical terrain and
	// obstacles that block a pawn should affect where a wolf puts its feet.
	FCollisionResponseParams Solid(ECR_Ignore);
	Solid.CollisionResponse.SetResponse(ECC_WorldStatic,ECR_Block);
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(WolfTerrain),false,GetOwner());
	return GetWorld()->LineTraceSingleByChannel(Hit,From,To,ECC_Pawn,Params,Solid);
}

FVector UWolfPackComponent::FeetOnGround(FVector P) const
{
	FHitResult Hit;
	if (TraceSolidTerrain(Hit,P+FVector(0,0,55),P-FVector(0,0,100))
		&& !Hit.bStartPenetrating && Hit.ImpactNormal.Z>.65 && Hit.ImpactPoint.Z <= P.Z+23.)
	{
		P.Z = Hit.ImpactPoint.Z;
	}
	return P;
}

FVector UWolfPackComponent::AvoidObstacle(FVector From, FVector Toward, float Side) const
{
	const FVector Travel = (Toward-From).GetSafeNormal2D();
	const FVector Snout = From+FVector(0,0,42);
	const double LookAhead = FMath::Min(105., FVector::Dist2D(From,Toward));
	FHitResult Hit;
	if (LookAhead > 20. && TraceSolidTerrain(Hit,Snout,Snout+Travel*LookAhead)
		&& !Hit.bStartPenetrating && Hit.ImpactNormal.Z < .55)
	{
		const FVector Normal = Hit.ImpactNormal.GetSafeNormal2D();
		FVector Tangent(-Normal.Y,Normal.X,0);
		const double Dot = FVector::DotProduct(Tangent,Travel);
		Tangent *= FMath::Abs(Dot)>.12 ? FMath::Sign(Dot) : Side;
		return From + Tangent*95. + Normal*18.;
	}
	return Toward;
}

void UWolfPackComponent::Step(int32 I, const FVector& Goal, const FVector& Face, float Dt, float Threat,
	bool bSwimming, bool bSnap, bool bFaceTarget, bool bGuard)
{
	UWolfMeshComponent* Wolf = Wolves[I];
	const FVector Last = Wolf->GetComponentLocation();
	FVector Next = Goal;
	if (!bSnap)
	{
		const FVector Waypoint = I>0 && !bGuard && !bSwimming ? AvoidObstacle(Last,Goal,I==1?-1.f:1.f) : Goal;
		// The pressure guide can change abruptly on leaving shelter. Even the
		// leader has to cover that separation on foot when he yields to the fire.
		const float SpeedLimit = bGuard ? 380.f : I==0 ? 920.f : FlankSpeed;
		Next = FMath::VInterpConstantTo(Last,Waypoint,Dt,SpeedLimit);
	}
	// Probe from the authored route elevation every frame. Overlapping foliage bounds
	// must never become a staircase that lifts the flankers into the canopy.
	if (!bSwimming)
	{
		Next.Z = Goal.Z;
		Next = FeetOnGround(Next);
	}
	const FVector Delta = Next-Last;
	const double Travel = bSnap ? 0. : Delta.Size2D();
	FVector Facing = Face.GetSafeNormal2D();
	if (!bFaceTarget && !bSnap && Travel>Dt*10.)
	{
		const FVector StepHeading = Delta.GetSafeNormal2D();
		Facing = FMath::Lerp(StepHeading,Facing,Threat*.42f).GetSafeNormal2D();
	}
	const FRotator GoalRotation(0,Facing.Rotation().Yaw,0);
	const FRotator Rotation = bSnap ? GoalRotation : FMath::RInterpConstantTo(Wolf->GetComponentRotation(),GoalRotation,Dt,280.f);
	Wolf->SetWorldLocationAndRotation(Next,Rotation);
	Wolf->SetVisibility(true);
	Wolf->AdvanceGait(Dt,Travel,bSwimming,Threat);
}

void UWolfPackComponent::Hunt(float Dt, FVector Lead, FVector Heading, FVector Quarry, float Threat, bool bSwimming)
{
	if (Wolves.IsEmpty()) return;
	PlaceMoonBounce(FVector(Quarry.X,Quarry.Y,Lead.Z));
	Threat = FMath::Clamp(Threat,0.f,1.f);
	Heading = Heading.GetSafeNormal2D();
	if (Heading.IsNearlyZero()) Heading = bPlaced ? Wolves[0]->GetForwardVector() : FVector::ForwardVector;
	const bool bSnap = !bPlaced || FVector::DistSquared2D(Lead,Wolves[0]->GetComponentLocation())>FMath::Square(1500.f);
	if (bSnap || bGuarding) QuarryVelocity = FVector::ZeroVector;
	else if (Dt>SMALL_NUMBER)
	{
		const FVector Measured = ((Quarry-LastQuarry)/Dt).GetClampedToMaxSize2D(650.);
		QuarryVelocity = FMath::VInterpTo(QuarryVelocity,Measured,Dt,5.f);
	}
	LastQuarry = Quarry;
	const FVector Intercept = Quarry + QuarryVelocity*.24f;
	FVector AttackBearing = (Intercept-Lead).GetSafeNormal2D();
	if (AttackBearing.IsNearlyZero()) AttackBearing=Heading;
	const FVector LeftRight(-Heading.Y,Heading.X,0);
	const FVector Around(-AttackBearing.Y, AttackBearing.X, 0);
	for (int32 I=0; I<Wolves.Num(); ++I)
	{
		FVector Goal=Lead;
		if (I>0)
		{
			const double Side = I==1 ? -1. : 1.;
			const FVector Column = Lead-Heading*(I==1 ? 118.:165.) + LeftRight*Side*(I==1 ? 67.:77.);
			// Spread into two independent intercepts as the leader commits. For a distant
			// quarry, keep the skirmish line anchored to the pack's actual location.
			const FVector Center = Lead + (Intercept-Lead).GetClampedToMaxSize2D(180.);
			const FVector InterceptSide = Center + Around*Side*(HuntWidth + (I==1 ? 0.f : 15.f)) - AttackBearing*43.;
			Goal = FMath::Lerp(Column,InterceptSide,FMath::SmoothStep(.05f,.95f,Threat));
			Goal.Z = Lead.Z;
		}
		const FVector Aim = FMath::Lerp(Heading,(Quarry-Goal).GetSafeNormal2D(),Threat);
		Step(I,Goal,Aim,Dt,Threat,bSwimming,bSnap,Threat>.77f && !bSwimming,false);
	}
	bPlaced = true;
	bGuarding = bCovering = false;
}

void UWolfPackComponent::GuardDoor(float Dt, FVector Door, bool bRepelled)
{
	if (Wolves.IsEmpty()) return;
	// The hollow runs from the threshold to the yard lip 190 cm out and is
	// 320 cm wide. The leader faces the door across it; each flanker holds
	// a solid side. Their backing paths leave Max a clear landing in the middle.
	static const FVector Guard[] = {{-270,35,0},{-145,-235,0},{-156,244,0}};
	static const FVector Retreat[] = {{-335,248,0},{-230,-315,0},{-223,370,0}};
	const FVector HingeFeet(Door.X,Door.Y,0);
	PlaceMoonBounce(HingeFeet);
	for (int32 I=0; I<Wolves.Num(); ++I)
	{
		const FVector Goal = HingeFeet + (bRepelled ? Retreat[I] : Guard[I]);
		const bool bSnap = !bPlaced || (!bGuarding && FVector::DistSquared2D(Goal,Wolves[I]->GetComponentLocation())>FMath::Square(620.f));
		// Look into the room while holding ground, and continue watching the flame when backing away.
		Step(I,Goal,HingeFeet+FVector(145,0,0)-Goal,Dt,bRepelled?.35f:.92f,false,bSnap,true,true);
	}
	bPlaced = bGuarding = true;
	bCovering = false;
}

void UWolfPackComponent::CoverPounce(float Dt, const FVector& PreyFeet)
{
	if (!bPlaced || Wolves.Num()!=3) return;
	if (!bCovering)
	{
		PlaceMoonBounce(PreyFeet);
		// Claim the shoulders of the kill once. Recomputing these points against
		// the airborne leader would make them swap sides as he passes the target.
		const FVector Axis = Wolves[0]->GetForwardVector().GetSafeNormal2D();
		const FVector Across(-Axis.Y,Axis.X,0);
		for (int32 I=1; I<3; ++I)
		{
			const FVector From = Wolves[I]->GetComponentLocation();
			const FVector Side = Across*(I==1 ? -1.f : 1.f);
			const FVector Ring = PreyFeet - Axis*108. + Side*HuntWidth;
			const FVector Shift = bGuarding ? Side*14. : (Ring-From).GetClampedToMaxSize2D(72.);
			CoverMarks[I-1] = FVector(From.X+Shift.X,From.Y+Shift.Y,From.Z);
		}
		bCovering = true;
	}
	// The capture camera owns the attacking wolf. Only the supporting two
	// complete their last strides, turn toward the impact, and settle to a stand.
	for (int32 I=1; I<3; ++I)
		Step(I,CoverMarks[I-1],PreyFeet-CoverMarks[I-1],Dt,1.f,false,false,true,true);
}
