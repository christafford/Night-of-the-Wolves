#include "WolfPackComponent.h"
#include "HuntingWolf.h"
#include "FirstLegCrossingLayout.h"
#include "WolfTrailGameMode.h"
#include "WolfMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<int32> PackDebug(TEXT("wolf.PackDebug"),0,TEXT("Show live pack paths, beliefs and engagement ownership."));
#endif
constexpr float Speeds[] = {.97f,1.04f,1.f,1.09f,.98f};
constexpr float SightHeight = 58.f;
}

UWolfPackComponent::UWolfPackComponent()
{
	static ConstructorHelpers::FObjectFinder<USoundBase> Signal(TEXT("/Game/NightOfTheWolves/FirstLeg/Growl"));
	LeadCall = Signal.Object;
}

void UWolfPackComponent::SpawnMembers()
{
	if (!Members.IsEmpty()) return;
	SignalAttenuation=NewObject<USoundAttenuation>(this);
	SignalAttenuation->Attenuation.AttenuationShapeExtents=FVector(280,0,0);
	SignalAttenuation->Attenuation.FalloffDistance=1400;
	SignalAttenuation->Attenuation.bSpatialize=true;
	SignalAttenuation->Attenuation.bAttenuate=true;
	FActorSpawnParameters Spawn;
	Spawn.Owner = GetOwner();
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 I=0; I<Count; ++I)
	{
		AHuntingWolf* Wolf = GetWorld()->SpawnActor<AHuntingWolf>(FVector::ZeroVector,FRotator::ZeroRotator,Spawn);
		check(Wolf);
		Wolf->Body->SetLightingChannels(true,true,false);
		Wolf->Body->SetVisibility(false);
		Wolf->Body->SetIdentity(I);
		Members.Add(Wolf);
	}
	if (FApp::CanEverRender())
	{
		MoonBounce = NewObject<UPointLightComponent>(GetOwner(), TEXT("WolfMoonBounce"));
		MoonBounce->SetupAttachment(GetOwner()->GetRootComponent());
		MoonBounce->SetLightingChannels(false,true,false);
		MoonBounce->SetIntensityUnits(ELightUnits::Candelas);
		MoonBounce->SetIntensity(3.5f);
		MoonBounce->SetLightColor(FLinearColor(.72f,.84f,1.f));
		MoonBounce->SetSourceRadius(32.f);
		MoonBounce->SetSoftSourceRadius(54.f);
		MoonBounce->SetAttenuationRadius(1050.f);
		MoonBounce->SetCastShadows(false);
		MoonBounce->SetVolumetricScatteringIntensity(0.f);
		MoonBounce->SetVisibility(false);
		MoonBounce->RegisterComponent();
	}
	ResetAttempt();
}

void UWolfPackComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	for (AHuntingWolf* Wolf : Members)
		if (IsValid(Wolf)) Wolf->Destroy();
	Members.Empty();
	Super::EndPlay(Reason);
}

void UWolfPackComponent::ResetAttempt()
{
	Nav.Reset(GetWorld());
	Nav.SetCabinAllowed(false);
	Tactics.Reset();
	bPlaced = bCanCapture = bThreshold = bPlayedSignal = false;
	Movement = EMovement::Hunt;
	Attacker = INDEX_NONE;
	BreachProgress = -1.f;
	for (int32 I=0; I<Members.Num(); ++I)
	{
		Walkers[I] = {};
		GetMember(I)->ResetGait(I*.27f);
		GetMember(I)->SetHuntGesture(0,0);
		GetMember(I)->SetVisibility(false);
	}
	if (MoonBounce) MoonBounce->SetVisibility(false);
}

void UWolfPackComponent::PlaceAtStart(const FVector& PressureFeet, FVector Heading)
{
	if (Members.Num()!=Count || bPlaced) return;
	Heading = Heading.GetSafeNormal2D();
	const FVector Side(-Heading.Y, Heading.X, 0);
	// Authored formation only for the initial placement of a restored attempt.
	const FVector2D Offsets[] = {{-92,-73},{0,0},{-145,70},{-235,-63},{-304,31}};
	FVector Starts[Count];
	for (int32 I=0; I<Count; ++I)
	{
		const FVector Preferred=PressureFeet+Heading*Offsets[I].X+Side*Offsets[I].Y;
		bool bFound=false;
		for (int32 Attempt=0; Attempt<25 && !bFound; ++Attempt)
		{
			const int32 Ring=(Attempt+7)/8;
			const FVector Offset=Attempt==0 ? FVector::ZeroVector :
				Heading.RotateAngleAxis(45.*((Attempt-1)%8),FVector::UpVector)*(Ring*65.);
			if (!Nav.ProjectFeet(Preferred+Offset,Starts[I])) continue;
			bFound=true;
			for (int32 Previous=0; Previous<I; ++Previous)
				if (FVector::DistSquared2D(Starts[I],Starts[Previous])<FMath::Square(66.f)) bFound=false;
		}
		if (!bFound)
		{
			UE_LOG(LogTemp,Error,TEXT("No supported checkpoint placement for %s near %s"),Tactics.IdentityName(I),*Preferred.ToCompactString());
			return;
		}
	}
	for (int32 I=0; I<Count; ++I) Pose(I,Starts[I],Heading,0,0,false,0,true);
	bPlaced = true;
}

void UWolfPackComponent::RunOpening(USplineComponent& Trail, float AlphaDistance, float Destination, float Dt, bool bRevealLeader)
{
	// Four wolves run ahead of the lens. As Max comes into view they check their
	// speed, and the silver wolf overtakes on the centre line. These same five
	// pawns continue into Hunt from their final foot positions.
	if (!bPlaced) return;
	constexpr float Lead[] = {0,640,522,408,307};
	constexpr float AtHandoff[] = {0,-157,-235,-320,-396};
	constexpr float RunningLanes[] = {0,-38,42,-127,128};
	constexpr float FinalLanes[] = {0,-56,68,-75,65};
	const float Join = FMath::SmoothStep(Destination-1720.f,Destination,AlphaDistance);
	for (int32 I=0; I<Count; ++I)
	{
		const float Along = AlphaDistance + FMath::Lerp(Lead[I],AtHandoff[I],Join);
		const FVector Center = Trail.GetLocationAtDistanceAlongSpline(Along,ESplineCoordinateSpace::World);
		const FVector Ahead = Trail.GetLocationAtDistanceAlongSpline(Along+80.f,ESplineCoordinateSpace::World);
		const FVector Heading = (Ahead-Center).GetSafeNormal2D();
		const FVector Side(-Heading.Y,Heading.X,0);
		const float Lane=FMath::Lerp(RunningLanes[I],FinalLanes[I],Join);
		FVector Feet;
		// Draw the outside lanes inward where the trail brushes a trunk.
		if (Nav.ProjectFeet(Center + Side*Lane,Feet) || Nav.ProjectFeet(Center + Side*Lane*.5f,Feet)
			|| Nav.ProjectFeet(Center,Feet))
			Pose(I,Feet,Heading,Dt,.5f,false,0.f,Dt==0.f);
		if (I==0) GetMember(I)->SetVisibility(bRevealLeader);
	}
	PlaceMoonBounce(GetMember(0)->GetComponentLocation());
}

void UWolfPackComponent::PlaceMoonBounce(const FVector& Ground)
{
	if (!MoonBounce) return;
	MoonBounce->SetWorldLocation(Ground + FVector(40.,-65.,195.));
	MoonBounce->SetVisibility(true);
}

UWolfMeshComponent* UWolfPackComponent::GetMember(int32 Index) const
{
	return Members.IsValidIndex(Index) ? Members[Index]->Body.Get() : nullptr;
}

UWolfMeshComponent* UWolfPackComponent::GetLeadWolf() const { return bPlaced ? GetMember(0) : nullptr; }
UWolfMeshComponent* UWolfPackComponent::GetAttacker() const { return GetMember(Attacker); }

void UWolfPackComponent::BeginMovement(EMovement Next)
{
	if (Movement == Next) return;
	Movement = Next;
	for (auto& Walker : Walkers) Walker = {};
	Attacker = INDEX_NONE;
	bCanCapture = bThreshold = bPlayedSignal = false;
	BreachProgress = -1.f;
	for (int32 I=0; I<Count; ++I) GetMember(I)->SetHuntGesture(0,0);
	Tactics.ReleaseAssignments();
}

bool UWolfPackComponent::ClearSight(const FVector& From,const FVector& To,float Radius) const
{
	FCollisionQueryParams Query(SCENE_QUERY_STAT(PackSight),false);
	FCollisionResponseParams Solid(ECR_Ignore);
	Solid.CollisionResponse.SetResponse(ECC_WorldStatic,ECR_Block);
	Solid.CollisionResponse.SetResponse(ECC_WorldDynamic,ECR_Block);
	if (Radius > 0)
		return !GetWorld()->SweepTestByChannel(From,To,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(Radius),Query,Solid);
	return !GetWorld()->LineTraceTestByChannel(From,To,ECC_Pawn,Query,Solid);
}

void UWolfPackComponent::HearFootfall(const FVector& Feet,float Force)
{
	if (!bPlaced) return;
	if (Movement!=EMovement::Hunt) return;
	float Nearest = BIG_NUMBER;
	int32 Listener = INDEX_NONE;
	for (int32 I=0; I<Count; ++I)
	{
		const FVector Ear = GetMember(I)->GetComponentLocation()+FVector(0,0,SightHeight);
		const float Distance = FVector::Dist(Ear,Feet);
		if (Distance >= Nearest || Distance > 900.f*Force) continue;
		if (Distance > 320.f*Force && !ClearSight(Ear,Feet+FVector(0,0,20))) continue;
		Listener = I;
		Nearest = Distance;
	}
	if (Listener!=INDEX_NONE) Tactics.Hear(Feet, Listener);
}

void UWolfPackComponent::Pose(int32 I,const FVector& Next,const FVector& Face,float Dt,float Threat,bool bSwimming,float Peer,bool bSnap)
{
	auto* Wolf = GetMember(I);
	const FVector Travel = Next-Wolf->GetComponentLocation();
	FRotator Desired = Wolf->GetComponentRotation();
	if (!Face.IsNearlyZero()) Desired = FRotator(0,Face.Rotation().Yaw,0);
	const FRotator Rotation = bSnap ? Desired : FMath::RInterpConstantTo(Wolf->GetComponentRotation(),Desired,Dt,285.f);
	Wolf->SetWorldLocationAndRotation(Next,Rotation);
	Wolf->SetVisibility(true);
	Wolf->AdvanceGait(Dt,bSnap ? 0.f : Travel.Size2D(),bSwimming,Threat,Peer);
}

void UWolfPackComponent::MoveGround(int32 I,const TArray<FVector>& Points,float Dt,float Speed,float Threat,const FVector& Watch,bool bSeparate)
{
	auto& Walker = Walkers[I];
	const FVector From = GetMember(I)->GetComponentLocation();
	while (Points.IsValidIndex(Walker.Cursor) && FVector::Dist2D(From,Points[Walker.Cursor])<14.f) ++Walker.Cursor;
	FVector Step=FVector::ZeroVector;
	float DesiredSpeed=0.f;
	if (Points.IsValidIndex(Walker.Cursor))
	{
		Step=Points[Walker.Cursor]-From;
		Step.Z=0;
		const float Alignment=FVector::DotProduct(GetMember(I)->GetForwardVector(),Step.GetSafeNormal2D());
		DesiredSpeed=Speed*FMath::GetMappedRangeValueClamped(FVector2D(-.65,.82),FVector2D(.23,1.),Alignment);
		if (Walker.Cursor==Points.Num()-1)
			DesiredSpeed=FMath::Min(DesiredSpeed,float(FMath::Sqrt(2.*1850.*Step.Size2D())));
	}
	Walker.GroundSpeed=FMath::FInterpConstantTo(Walker.GroundSpeed,DesiredSpeed,Dt,DesiredSpeed>Walker.GroundSpeed ? 2090.f : 2500.f);
	Step=Step.GetClampedToMaxSize(Walker.GroundSpeed*Dt);
	FVector Avoid = FVector::ZeroVector;
	if (bSeparate)
	{
		for (int32 J=0; J<Count; ++J)
		{
			if (J==I) continue;
			const FVector Away = From-GetMember(J)->GetComponentLocation();
			const double Gap = Away.Size2D();
			if (Gap<78. && Gap>1.) Avoid += Away.GetSafeNormal2D()*(78.-Gap)*4.5*Dt;
		}
	}
	// Recheck personal space between plans if Max turns into a support wolf.
	// Use the pack's fresh sighting, so an unseen runner cannot steer wolves.
	if (Movement==EMovement::Hunt && !Tactics.CanAttack(I) && Tactics.Knowledge==EWolfKnowledge::Sight && Tactics.BeliefAge<.23f)
	{
		const FVector Away=From-Tactics.LastKnown;
		const float Radius=Tuning.StandOff*.68f;
		if (FMath::Abs(Away.Z)<105.f && Away.SizeSquared2D()<FMath::Square(Radius))
		{
			const FVector Out=Away.GetSafeNormal2D(SMALL_NUMBER,-GetMember(I)->GetForwardVector());
			const float Inward=FVector::DotProduct(Step,Out);
			if (Inward<0.f) Step-=Out*Inward;
			Avoid+=Out*FMath::Min<double>(Speed*Dt,(Radius-Away.Size2D())*8.f*Dt);
		}
	}
	auto Supported = [&](const FVector& Delta, FVector& Ground)
	{
		return !Delta.IsNearlyZero(.04) && Nav.CanTraverse(From, From+Delta, &Ground);
	};
	FVector Next = From;
	if (!Supported((Step+Avoid).GetClampedToMaxSize2D(Speed*Dt),Next) && !Avoid.IsNearlyZero())
	{
		Next = From;
		Supported(Step,Next);
	}
	const FVector Motion = Next-From;
	if (!Step.IsNearlyZero(1.) && Motion.Size2D()<.2f) Walker.BlockTime+=Dt;
	else Walker.BlockTime=0.f;
	const FVector Facing = Motion.Size2D()>.1 ? Motion : Watch-From;
	Pose(I,Next,Facing,Dt,Threat);
}

void UWolfPackComponent::FollowMark(int32 I,const FVector& Mark,const FVector& Watch,float Dt,float Speed,float Threat)
{
	auto& Walker=Walkers[I];
	Walker.RetryIn-=Dt;
	if (Walker.RetryIn<=0 && (Walker.ScriptPath.IsEmpty() || Walker.BlockTime>.24f || !Walker.ScriptGoal.Equals(Mark,42.f)))
	{
		FWolfNavigationPath Result;
		Walker.ScriptPath.Reset();
		if (Nav.FindPath(GetMember(I)->GetComponentLocation(), Mark, Result))
			Walker.ScriptPath = MoveTemp(Result.Points);
		Walker.Cursor=0;
		Walker.ScriptGoal=Mark;
		Walker.RetryIn=.36f+.035f*I;
		Walker.BlockTime=0.f;
	}
	MoveGround(I,Walker.ScriptPath,Dt,Speed,Threat,Watch);
}

void UWolfPackComponent::Hunt(float Dt,FVector Guide,FVector Heading,float HoldDistance,EWolfGroundEncounter Terrain,bool bPermitAttack)
{
	if (!bPlaced) return;
	BeginMovement(EMovement::Hunt);
	Nav.SetCabinAllowed(false);
	bCanCapture=bPermitAttack;
	FWolfTacticFrame Frame;
	Frame.Forward=Heading.GetSafeNormal2D();
	Frame.TrailReference=Guide;
	Frame.bAllowFlank=Terrain!=EWolfGroundEncounter::Trail;
	Frame.bAllowIntercept=Frame.bAllowFlank;
	Frame.bAllowEngagement=bPermitAttack;
	Frame.PressureLead=HoldDistance;
	for (int32 I=0; I<Count; ++I)
	{
		Frame.Feet[I]=GetMember(I)->GetComponentLocation();
		Frame.Speed[I]=Speeds[I]*PursuitSpeed;
		Frame.bBlocked[I]=Walkers[I].BlockTime>.20f;
	}
	const APawn* Max=UGameplayStatics::GetPlayerPawn(this,0);
	auto Sense=[&](int32 I,FVector& Seen)
	{
		if (!Max) return false;
		const FVector Eyes=Frame.Feet[I]+FVector(0,0,SightHeight);
		const FVector Body=Max->GetActorLocation();
		const FVector Offset=Body-Eyes;
		if (Offset.SizeSquared()>FMath::Square(SightRange)) return false;
		if (Offset.Size2D()>430.f && FVector::DotProduct(GetMember(I)->GetForwardVector(),Offset.GetSafeNormal2D())<-.22f) return false;
		if (!ClearSight(Eyes,Body)) return false;
		Seen=Body-FVector(0,0,Max->GetSimpleCollisionHalfHeight());
		return true;
	};
	auto Find=[&](int32 I,const FVector& From,const FVector& To,bool bAlternate,FWolfTacticRoute& Out)
	{
		FWolfNavigationPath Result;
		if (!Nav.FindPath(From,To,Result,bAlternate)) return false;
		Out.Points=MoveTemp(Result.Points); Out.Length=Result.Length; Out.bAlternate=Result.bAlternate; return true;
	};
	Tactics.Tick(Dt,Frame,Tuning,Sense,Find);
	for (int32 I=0; I<Count; ++I)
	{
		const auto& Plan=Tactics.Wolves[I];
		auto& Walker=Walkers[I];
		if (Walker.Revision!=Plan.Revision)
		{
			Walker.Cursor=0; Walker.Revision=Plan.Revision; Walker.BlockTime=0;
		}
		const bool bSignal=Tactics.CueTime>0 && I==Tactics.CueWolf;
		const FVector Watch=Tactics.BeliefAge<Tuning.MemorySeconds ? Tactics.LastKnown : Plan.Destination;
		FVector SignalToward=Watch;
		if (bSignal && Tactics.CueReceiver!=INDEX_NONE)
		{
			const auto& Receiver=Tactics.Wolves[Tactics.CueReceiver];
			if (!Receiver.Route.Points.IsEmpty())
			{
				// Indicate the shoulder or branch the responding wolf is taking.
				const int32 Next=FMath::Clamp(Walkers[Tactics.CueReceiver].Cursor,0,Receiver.Route.Points.Num()-1);
				SignalToward=Receiver.Route.Points[Next];
			}
		}
		const float Side=FVector::DotProduct(GetMember(I)->GetRightVector(),(SignalToward-Frame.Feet[I]).GetSafeNormal2D());
		const float Sniff = Plan.Assignment==EWolfAssignment::Track || Plan.Assignment==EWolfAssignment::Search ? 1.f : 0.f;
		GetMember(I)->SetHuntGesture(bSignal ? (Side<0 ? -.9f : .9f) : 0.f, Sniff);
		const float Threat=Plan.bEngaging ? .98f : Plan.Assignment==EWolfAssignment::Flank ? .62f : .30f;
		MoveGround(I,Plan.Route.Points,Dt,Frame.Speed[I],Threat,Watch);
	}
	if (Tactics.CueTime>0 && !bPlayedSignal && LeadCall && Tactics.CueWolf!=INDEX_NONE)
		UGameplayStatics::PlaySoundAtLocation(this,LeadCall,Frame.Feet[Tactics.CueWolf],.40f,.83f,0.f,SignalAttenuation);
	bPlayedSignal=Tactics.CueTime>0;
	PlaceMoonBounce(GetMember(Tactics.PressureWolf)->GetComponentLocation());
	Debug(Max);
}

void UWolfPackComponent::GuardDoor(float Dt,FVector Door,bool bRepelled,bool bExitExposed,bool bBreached)
{
	if (!bPlaced) return;
	BeginMovement(EMovement::Guard);
	Tactics.AdvanceWithoutSight(Dt,Tuning);
	DoorFeet=FVector(Door.X,Door.Y,0);
	bCanCapture=!bRepelled && (bExitExposed || bBreached);
	bThreshold=true;
	Nav.SetCabinAllowed(bBreached);
	// The hollow is 190 cm wide in the direction of approach. Both jambs
	// overlook its solid edges. The flame opens up the centre for Max's jump.
	static const FVector Guard[]={{-320,126,0},{-237,-14,0},{-173,-222,0},{-365,-322,0},{-185,275,0}};
	static const FVector Retreat[]={{-415,193,0},{-355,-164,0},{-240,-335,0},{-450,-397,0},{-235,378,0}};
	const APawn* Max=UGameplayStatics::GetPlayerPawn(this,0);
	for (int32 I=0; I<Count; ++I)
	{
		const FVector At=GetMember(I)->GetComponentLocation();
		// Once the bar breaks the pressure wolf can clear the open hollow just
		// as Max did. It must reach the actual lip before getting this traversal.
		if (I==1 && bBreached && !bRepelled && BreachProgress<1.f)
		{
			const FVector Lip=DoorFeet+FVector(-235,0,0);
			if (BreachProgress<0 && FVector::Dist2D(At,Lip)<65.f &&
				ClearSight(At+FVector(0,0,57),DoorFeet+FVector(75,0,61),15.f))
			{
				BreachProgress=0; BreachStart=At;
			}
			if (BreachProgress>=0)
			{
				BreachProgress=FMath::Min(1.f,BreachProgress+Dt/.58f);
				FVector Land=DoorFeet+FVector(79,0,0);
				Nav.ProjectFeet(Land,Land);
				const FVector Foot=FMath::Lerp(BreachStart,Land,BreachProgress)+FVector(0,0,49.f*FMath::Sin(PI*BreachProgress));
				Pose(I,Foot,Land-BreachStart,Dt,.96f);
				continue;
			}
		}
		FVector Goal=DoorFeet+(bRepelled ? Retreat[I] : Guard[I]);
		FVector Watch=DoorFeet+FVector(140,0,20);
		if (I==1 && BreachProgress>=1.f && Max && ClearSight(At+FVector(0,0,SightHeight),Max->GetActorLocation()))
		{
			Watch=Max->GetActorLocation();
			const FVector Feet=Watch-FVector(0,0,Max->GetSimpleCollisionHalfHeight());
			const FVector FromMax=At-Feet;
			const double StandBack=FMath::Min(145.,FromMax.Size2D());
			Goal=Feet+FromMax.GetSafeNormal2D()*StandBack;
		}
		FollowMark(I,Goal,Watch,Dt,bRepelled ? 355.f : PursuitSpeed,.80f);
	}
	PlaceMoonBounce(DoorFeet);
	Debug(Max);
}

void UWolfPackComponent::FaceFire(float Dt, const FVector& BurningBranch)
{
	if (!bPlaced) return;
	if (Movement!=EMovement::FireAlert)
	{
		BeginMovement(EMovement::FireAlert);
		FireClock=0.f;
	}
	FireClock+=Dt;
	Tactics.AdvanceWithoutSight(Dt,Tuning);
	for (int32 I=0; I<Count; ++I)
	{
		auto* Body=GetMember(I);
		const FVector Feet=Body->GetComponentLocation();
		Pose(I,Feet,BurningBranch-Feet,Dt,.15f);
		const float T=FireClock-I*.017f;
		const float Recoil=FMath::SmoothStep(0.f,.11f,T)*(1.f-FMath::SmoothStep(.20f,.72f,T));
		if (Recoil>.01f) Body->SetFireRecoil(Recoil);
	}
	PlaceMoonBounce(GetMember(0)->GetComponentLocation());
}

bool UWolfPackComponent::FindFireCover(int32 I, const FVector& BurningBranch)
{
	// Read the actual wolf's place on the winding far road. A continuous run
	// can leave a wolf at the robber, well below the checkpoint formation.
	using namespace FirstLegLayout;
	const FVector From=GetMember(I)->GetComponentLocation();
	const float Lane[] = {-110,70,166,-181,12};
	const float Retreat=FireRetreatDistance*(.85f+.065f*I);
	auto& Walker=Walkers[I];
	const FVector BlockedGoal=Walker.ScriptGoal;
	const bool bStalled=Walker.BlockTime>.24f;
	auto UseRoute=[&](FWolfNavigationPath& Route)
	{
		Walker.ScriptGoal=Route.Points.Last();
		Walker.ScriptPath=MoveTemp(Route.Points);
		Walker.Cursor=0;
		Walker.BlockTime=0.f;
	};
	auto TryCover=[&](FVector Target)
	{
		if (bStalled && FVector::DistSquared2D(Target,BlockedGoal)<FMath::Square(80.f)) return false;
		if (FVector::Dist2D(Target,BurningBranch)<FVector::Dist2D(FireStart[I],BurningBranch)+Retreat*.30f) return false;
		FWolfNavigationPath Route;
		if (!Nav.FindPath(From,Target,Route) || Route.Points.IsEmpty()) return false;
		const double Closest=FVector::Dist2D(From,BurningBranch)-100.;
		if (Route.Points.ContainsByPredicate([&](const FVector& P){return FVector::DistSquared2D(P,BurningBranch)<Closest*Closest;})) return false;
		UseRoute(Route);
		return true;
	};
	if (bStalled)
	{
		// The original long line may brush a boulder as the bank changes slope.
		// Get around its shoulder from the current paws before planning the rest.
		const FVector Out=(From-BurningBranch).GetSafeNormal2D();
		for (float Angle : {-32.f,32.f,-60.f,60.f,-85.f,85.f,-105.f,105.f})
		{
			FVector Via;
			if (!Nav.CanTraverse(From,From+Out.RotateAngleAxis(Angle,FVector::UpVector)*175.,&Via)) continue;
			for (float Pitch : {0.f,-28.f,28.f})
			{
				const FVector End=Via+Out.RotateAngleAxis(Pitch,FVector::UpVector)*420.;
				FWolfNavigationPath Route;
				if (!Nav.FindPathVia(From,Via,End,Route) || Route.Points.IsEmpty()) continue;
				UseRoute(Route);
				return true;
			}
		}
	}
	// A flanker on the shingle or a scout in the trees may have no route
	// back to the tread. Let it break directly away onto its own bank.
	const FVector Away=(From-BurningBranch).GetSafeNormal2D();
	for (float Bearing : {0.f,-25.f,25.f,-48.f,48.f,-65.f,65.f})
	{
		const FVector Direction=Away.RotateAngleAxis(Bearing,FVector::UpVector);
		for (float Fraction : {1.f,.70f,.48f})
		{
			const FVector Goal=FireStart[I]+Direction*(Retreat*Fraction);
			FVector Ground;
			if (Nav.CanTraverse(From,Goal,&Ground) && TryCover(Ground)) return true;
		}
	}
	// Around rocks, let the surface planner work back down the actual road.
	for (float Fraction : {1.f,.73f,.48f})
	{
		const double X=FMath::Clamp(FireStart[I].X-Retreat*Fraction,FarRoad[0].X,FarRoad[UE_ARRAY_COUNT(FarRoad)-1].X);
		int32 Edge=1;
		while (Edge+1<UE_ARRAY_COUNT(FarRoad) && X>FarRoad[Edge].X) ++Edge;
		const FVector A=FarRoad[Edge-1], B=FarRoad[Edge];
		const FVector Center=FMath::Lerp(A,B,(X-A.X)/(B.X-A.X));
		const FVector Side=FVector::CrossProduct(FVector::UpVector,(B-A).GetSafeNormal2D());
		for (float Offset : {Lane[I],0.f,-Lane[I],245.f,-245.f})
			if (TryCover(Center+Side*Offset)) return true;
	}
	for (float Bearing : {0.f,-40.f,40.f,-65.f,65.f})
	{
		const FVector Direction=Away.RotateAngleAxis(Bearing,FVector::UpVector);
		for (float Fraction : {.8f,.52f})
			if (TryCover(FireStart[I]+Direction*(Retreat*Fraction))) return true;
	}
	return false;
}

void UWolfPackComponent::FleeFire(float Dt, const FVector& BurningBranch, bool bAlreadyAtCover)
{
	if (!bPlaced) return;
	if (Movement!=EMovement::Fire)
	{
		BeginMovement(EMovement::Fire);
		FireClock=0.f;
		for (int32 I=0; I<Count; ++I)
		{
			FireStart[I]=GetMember(I)->GetComponentLocation();
			AtFireCover[I]=bAlreadyAtCover;
			FirePlanAge[I]=0.f;
			FirePlanFeet[I]=FireStart[I];
		}
	}
	FireClock+=Dt;
	Tactics.AdvanceWithoutSight(Dt,Tuning);
	for (int32 I=0; I<Count; ++I)
	{
		auto* Body=GetMember(I);
		const FVector Feet=Body->GetComponentLocation();
		auto& Walker=Walkers[I];
		const float Release=.19f+I*.052f;
		if (AtFireCover[I])
		{
			Pose(I,Feet,BurningBranch-Feet,Dt,.12f);
			continue;
		}
		if (FireClock<Release)
		{
			Pose(I,Feet,BurningBranch-Feet,Dt,0.f);
			Body->SetFireRecoil(FMath::SmoothStep(0.f,.075f,FireClock));
			continue;
		}
		if (!Walker.ScriptPath.IsEmpty() && FVector::DistSquared2D(Feet,Walker.ScriptGoal)<FMath::Square(35.f))
		{
			AtFireCover[I]=true;
			continue;
		}
		Walker.RetryIn-=Dt;
		FirePlanAge[I]+=Dt;
		if (FirePlanAge[I]>.44f)
		{
			if (!Walker.ScriptPath.IsEmpty() && FVector::DistSquared2D(Feet,FirePlanFeet[I])<FMath::Square(24.f))
				Walker.BlockTime=.5f;
			FirePlanFeet[I]=Feet;
			FirePlanAge[I]=0.f;
		}
		if ((Walker.ScriptPath.IsEmpty() || Walker.BlockTime>.24f) && Walker.RetryIn<=0.f)
		{
			FindFireCover(I,BurningBranch);
			Walker.RetryIn=.35f;
		}
		// Their lanes already separate the runners: keep every body on the exact
		// path that cleared the rocks, including the wolf coming off the shingle.
		MoveGround(I,Walker.ScriptPath,Dt,Speeds[I]*PursuitSpeed*1.24f,.22f,Feet+(Feet-BurningBranch),false);
	}
	PlaceMoonBounce(GetMember(0)->GetComponentLocation());
}

UWolfMeshComponent* UWolfPackComponent::ClaimCapture(const APawn& Max)
{
	if (!bPlaced || !bCanCapture || Attacker!=INDEX_NONE) return nullptr;
	const FVector Body=Max.GetActorLocation();
	const FVector Feet=Body-FVector(0,0,Max.GetSimpleCollisionHalfHeight());
	int32 Selected=INDEX_NONE;
	float Best=BIG_NUMBER;
	for (int32 I=0; I<Count; ++I)
	{
		if (!bThreshold && !Tactics.CanAttack(I)) continue;
		const FVector At=GetMember(I)->GetComponentLocation();
		const float Gap=FVector::Dist2D(At,Feet);
		if (Gap>PounceReach || FMath::Abs(Feet.Z-At.Z)>175.f ||
			!ClearSight(At+FVector(0,0,54),Body,18.f)) continue;
		// A low direct launch corridor permits an open hollow, but the extra
		// body sweep prevents leaping through a closed leaf, jamb or tree trunk.
		if (!ClearSight(At+FVector(0,0,36),Feet+FVector(0,0,53),16.f)) continue;
		const float Cost=Gap-(I==1 ? 55.f : 0.f);
		if (Cost<Best) { Best=Cost; Selected=I; }
	}
	if (Selected==INDEX_NONE) return nullptr;
	if (bThreshold) Tactics.ReserveEncounterAttack(Selected);
	if (!Tactics.CanAttack(Selected)) return nullptr;
	Attacker=Selected;
	bCanCapture=false;
	UE_LOG(LogTemp,Display,TEXT("PACK CAPTURE %s at %.1f cm on a clear approach"),Tactics.IdentityName(Selected),
		FVector::Dist2D(GetMember(Selected)->GetComponentLocation(),Feet));
	return GetMember(Selected);
}

void UWolfPackComponent::ApproachBrokenBridge(float Dt)
{
	BeginMovement(EMovement::Bridge);
	Tactics.AdvanceWithoutSight(Dt,Tuning);
	// Max pushed off on the cable. The direct pursuer checks the loading
	// edge; the leader and scouts approach the broken span from dry land.
	static const FVector Shore[]={{9738,-1509,62},{9725,-1190,123},{9680,-1395,55},{9595,-1645,49},{9530,-1560,52}};
	for (int32 I=0; I<Count; ++I)
		FollowMark(I,Shore[I],Shore[I]+FVector(300,0,0),Dt,PursuitSpeed*Speeds[I],.70f);
	PlaceMoonBounce(GetMember(0)->GetComponentLocation());
	Debug(UGameplayStatics::GetPlayerPawn(this,0));
}

void UWolfPackComponent::FollowCreek(float Dt,const FVector (&Feet)[Count],const FVector (&Heading)[Count],const float (&Peer)[Count],bool bSnap)
{
	BeginMovement(EMovement::Creek);
	Tactics.AdvanceWithoutSight(Dt,Tuning);
	for (int32 I=0; I<Count; ++I)
	{
		const FVector From=GetMember(I)->GetComponentLocation();
		const FVector& Goal=Feet[I];
		if (bSnap)
		{
			Pose(I,Goal,Heading[I],0,0,Goal.Z < -229.,Peer[I],true);
			continue;
		}
		if (FVector::Dist2D(From,Goal)>170.f && From.X<9820. && From.Z>0.)
		{
			// A late scout must still come around the end of the bridge rail.
			const FVector Entry(9630,-2070,42);
			const FVector Approach=Goal.X>9825. && From.Y>-1900. ? Entry : Goal;
			FollowMark(I,Approach,Goal+Heading[I]*160.,Dt,PursuitSpeed*Speeds[I],.24f);
			continue;
		}
		const FVector Next=FMath::VInterpConstantTo(From,Goal,Dt,(From.Z< -229. ? 310.f : PursuitSpeed)*Speeds[I]);
		Pose(I,Next,Heading[I],Dt,0,Next.Z < -229.,Peer[I]);
	}
	bPlaced=true;
	PlaceMoonBounce(GetMember(0)->GetComponentLocation());
	Debug(UGameplayStatics::GetPlayerPawn(this,0));
}

void UWolfPackComponent::PursueDiversion(float Dt,const FVector& Target)
{
	BeginMovement(EMovement::Diversion);
	Tactics.AdvanceWithoutSight(Dt,Tuning);
	const FVector Axis=(Target-GetMember(0)->GetComponentLocation()).GetSafeNormal2D();
	const FVector Across(-Axis.Y,Axis.X,0);
	const FVector2D Stations[]={{-210,0},{-195,95},{-200,-125},{-350,-180},{-390,170}};
	for (int32 I=0; I<Count; ++I)
		FollowMark(I,Target+Axis*Stations[I].X+Across*Stations[I].Y,Target,Dt,PursuitSpeed*Speeds[I],.83f);
	PlaceMoonBounce(GetMember(0)->GetComponentLocation());
}

void UWolfPackComponent::CoverPounce(float Dt,const FVector& PreyFeet)
{
	if (Attacker==INDEX_NONE) return;
	// The capture camera animates the reserved member. Others hold their
	// actual ground; head turns and breathing continue through the impact.
	for (int32 I=0; I<Count; ++I)
		if (I!=Attacker)
		{
			const FVector From=GetMember(I)->GetComponentLocation();
			Pose(I,From,PreyFeet-From,Dt,.86f);
		}
}

void UWolfPackComponent::FlankBandit(float Dt,FVector Target,FVector Axis,float SinceHit)
{
	const FVector Across(-Axis.Y,Axis.X,0);
	for (int32 I=1; I<Count; ++I)
	{
		const float Side=I%2 ? -1.f : 1.f;
		const bool Close=I<=2;
		const float Spring=FMath::SmoothStep(I==1 ? -.15f : .26f,I==1 ? .74f : 1.24f,SinceHit);
		const float Offset=Close ? FMath::Lerp(185.f,76.f,Spring) : 240.f+I*12.f;
		FollowMark(I,Target+Across*(Side*Offset)+Axis*(Close ? 10.f*Side : -133.f),Target,Dt,460.f,.94f);
		if (Close && Spring>.85f) GetMember(I)->SetStrugglePose(Spring,SinceHit*8.f+I*2.f);
	}
}

void UWolfPackComponent::Debug(const APawn* Max)
{
#if !UE_BUILD_SHIPPING
	if (PackDebug.GetValueOnGameThread()==0 || !GEngine) return;
	UWorld* World=GetWorld();
	static const FColor Colors[]={FColor::Silver,FColor::Red,FColor::Cyan,FColor::Orange,FColor::Green};
	const TCHAR* Source=Tactics.Knowledge==EWolfKnowledge::Sight ? TEXT("sight") : Tactics.Knowledge==EWolfKnowledge::Footfall ? TEXT("heard step") :
		TEXT("unknown");
	if (Max) DrawDebugSphere(World,Max->GetActorLocation(),35,12,FColor::Magenta,false,0);
	if (Tactics.Knowledge!=EWolfKnowledge::None)
		DrawDebugSphere(World,Tactics.LastKnown+FVector(0,0,20),32,12,FColor::Yellow,false,0);
	if (Tactics.Wolves[3].Assignment==EWolfAssignment::Intercept)
		DrawDebugSphere(World,Tactics.PredictedTarget+FVector(0,0,25),37,12,FColor::Orange,false,0);
	for (int32 I=0; I<Count; ++I)
	{
		const auto& Wolf=Tactics.Wolves[I];
		const auto& Walker=Walkers[I];
		const bool bGround=Movement==EMovement::Hunt;
		const auto& Points=bGround ? Wolf.Route.Points : Walker.ScriptPath;
		const FVector Position=GetMember(I)->GetComponentLocation();
		FVector From=Position+FVector(0,0,18);
		for (int32 J=Walker.Cursor;J<Points.Num();++J)
		{
			const FVector To=Points[J]+FVector(0,0,18);
			DrawDebugLine(World,From,To,Colors[I],false,0,0,2);
			From=To;
		}
		const FVector Destination=bGround ? Wolf.Destination : Walker.ScriptGoal;
		DrawDebugPoint(World,Destination+FVector(0,0,18),9,Colors[I],false,0);
		const TCHAR* Task=bGround ? Tactics.AssignmentName(Wolf.Assignment) : Movement==EMovement::Guard ? TEXT("guard door") :
			Movement==EMovement::Bridge ? TEXT("check broken bridge") : Movement==EMovement::Creek ? TEXT("cross creek") : Movement==EMovement::FireAlert ? TEXT("startle at the lightning") : Movement==EMovement::Fire ? TEXT("retreat from the burning tree") : TEXT("gold diversion");
		const TCHAR* Status=Walker.BlockTime>.20f ? TEXT("blocked") : Points.IsValidIndex(Walker.Cursor) ? TEXT("following") : TEXT("settled");
		const FString Label=FString::Printf(TEXT("%s | %s | %s %d/%d -> %.0f %.0f | %s %.1fs | %s | %s"),
			Tactics.IdentityName(I),Task,Status,Walker.Cursor,Points.Num(),Destination.X,Destination.Y,Source,
			FMath::Min(999.f,Tactics.BeliefAge),Tactics.CanAttack(I) ? TEXT("commit") : TEXT("hold"),*Wolf.Reason);
		GEngine->AddOnScreenDebugMessage(12501+I,.2f,Colors[I],Label);
		DrawDebugString(World,Position+FVector(0,0,108),Tactics.IdentityName(I),nullptr,Colors[I],0.f);
	}
#endif
}
