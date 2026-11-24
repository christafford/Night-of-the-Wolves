#if !UE_BUILD_SHIPPING
#include "HighWaterChamber.h"
#include "HighWaterLayout.h"
#include "Night_of_the_WolvesCharacter.h"
#include "StormfallSet.h"
#include "WolfTrailGameMode.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "SceneView.h"
#include "TimerManager.h"

namespace
{
struct FCaveOrbitStation
{
	const TCHAR* Name;
	FVector Feet;
};

const FCaveOrbitStation OrbitStations[] = {
	{TEXT("vestibule"), {445, 15, 8}},
	{TEXT("blind cairn"), {610, -45, 14}},
	{TEXT("cairn side wall"), {605, 155, 14}},
	{TEXT("vestibule slabs"), {280, -151, 6}},
	{TEXT("root turn"), {452, 357, 13}},
	{TEXT("cleft west wall"), {410, 702, 19}},
	{TEXT("cleft east wall"), {636, 829, 20}},
	{TEXT("ramp seam"), HighWaterLayout::Entrance},
	{TEXT("low ramp west"), {437, 1220, 112}},
	{TEXT("high ramp east"), {648, 1450, 181}},
	{TEXT("shelf threshold"), HighWaterLayout::Shelf},
	{TEXT("lower torch fork"), HighWaterLayout::LowerFork},
	{TEXT("west shelf near edge"), {224, 1880, 264}},
	{TEXT("west shelf far edge"), {250, 3110, 264}},
	{TEXT("trough"), HighWaterLayout::Trough},
	{TEXT("landing"), HighWaterLayout::Landing},
	{TEXT("upper elbow"), {1478, 2848, 532}},
	{TEXT("upper arch wall"), {1617, 3090, 538}},
	{TEXT("upper exit"), HighWaterLayout::Exit},
	{TEXT("passage end"), {1490, 3580, 554}},
};
constexpr int32 OrbitYawCount = 16;
constexpr double OrbitPitches[] = {-78., -40., 0., 42., 82.};
// Include the exact aim from the packaged blind-cairn reproduction.
constexpr int32 OrbitViewCount = OrbitYawCount * UE_ARRAY_COUNT(OrbitPitches) + 1;

// Only a sight line through the authored mouth can actually reach the forest.
bool LooksOutOfCaveMouth(const FVector& From, const FVector& Direction)
{
	const FVector Local = StormfallLayout::LocalPoint(From);
	const FVector Ray = StormfallLayout::Facing.UnrotateVector(Direction);
	if (Ray.X >= -UE_SMALL_NUMBER) return false;
	const double T = (-82. - Local.X) / Ray.X;
	if (T < 0.) return false;
	const FVector AtMouth = Local + T * Ray;
	return FMath::Abs(AtMouth.Y + 6.) <= 255. && AtMouth.Z >= -20. && AtMouth.Z <= 346.;
}

UStaticMeshComponent* IntersectedCaveMesh(const TArray<UStaticMeshComponent*>& Shells, const FVector& A, const FVector& B)
{
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(CaveOrbitTest), true);
	FHitResult Hit;
	for (UStaticMeshComponent* Shell : Shells)
	{
		if (Shell->LineTraceComponent(Hit, A, B, Params)) return Shell;
	}
	return nullptr;
}

void TestCaveCamera(UWorld* Level)
{
	if (!Level || Level->WorldType != EWorldType::Game) return;
	FTimerHandle Timer;
	Level->GetTimerManager().SetTimer(Timer, [Level, Station=-1, ViewIndex=0, Frames=0, Shells=TArray<UStaticMeshComponent*>(),
		OutdoorEye=FVector::ZeroVector, NormalLength=0.f, NormalTarget=FVector::ZeroVector, NormalSocket=FVector::ZeroVector, Views=0, Samples=0,
		Clipped=0, Leaks=0, ClippedAtStation=0, LeaksAtStation=0, PortalRays=0, Since=Level->GetTimeSeconds(), ViewSize=FIntPoint::ZeroValue]() mutable
	{
		APlayerController* PC = Level->GetFirstPlayerController();
		auto* Max = PC ? Cast<ANight_of_the_WolvesCharacter>(PC->GetCharacter()) : nullptr;
		auto* Mode = Level->GetAuthGameMode<AWolfTrailGameMode>();
		auto Fail = [](const TCHAR* Why)
		{
			UE_LOG(LogTemp, Error, TEXT("CAVE CAMERA TEST FAILED: %s"), Why);
			FPlatformMisc::RequestExitWithStatus(false, 1);
		};
		if (!Max || !Mode || !PC->PlayerCameraManager || Level->GetTimeSeconds() - Since < .75f) return;
		if (Mode->Checkpoint < 6 || Mode->IsDead()) { Fail(TEXT("launch from an earned cave checkpoint")); return; }
		USpringArmComponent* Arm = Max->GetCameraBoom();
		const int32 StationCount = UE_ARRAY_COUNT(OrbitStations);
		auto Place = [&](FVector Feet)
		{
			Max->GetCharacterMovement()->StopMovementImmediately();
			Max->SetActorLocation(StormfallLayout::WorldPoint(Feet) + FVector::UpVector *
				(Max->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.), false, nullptr, ETeleportType::TeleportPhysics);
		};
		// Establish the true exterior camera, then return to the same spot after the interior orbits.
		const FVector Exterior(-410, 30, -25);
		if (Station == -1 || Station == StationCount)
		{
			if (Frames++ == 0)
			{
				Place(Exterior);
				PC->SetControlRotation(FRotator(-8, 90, 0));
				return;
			}
			if (Frames < 110) return;
			if (Station == StationCount)
			{
				if (!(PC->PlayerCameraManager->GetCameraLocation() - Max->GetActorLocation()).Equals(OutdoorEye, 5.)
					|| !FMath::IsNearlyEqual(Arm->TargetArmLength, NormalLength) || !Arm->TargetOffset.Equals(NormalTarget, .1)
					|| !Arm->SocketOffset.Equals(NormalSocket, .1)) { Fail(TEXT("outdoor boom settings failed to restore")); return; }
				UE_LOG(LogTemp, Display, TEXT("CAVE CAMERA orbit results: %dx%d, %d views, %d near-plane samples, %d mouth rays, %d rock crossings, %d open sight lines"),
					ViewSize.X, ViewSize.Y, Views, Samples, PortalRays, Clipped, Leaks);
				if (Clipped || Leaks || !PortalRays) { Fail(TEXT("render shell crossed, chamber open or mouth view lost")); return; }
				UE_LOG(LogTemp, Display, TEXT("CAVE CAMERA TEST PASSED: orbit, sight lines and outdoor camera restoration"));
				FPlatformMisc::RequestExit(false);
				return;
			}
			OutdoorEye = PC->PlayerCameraManager->GetCameraLocation() - Max->GetActorLocation();
			NormalLength = Arm->TargetArmLength;
			NormalTarget = Arm->TargetOffset;
			NormalSocket = Arm->SocketOffset;
			// The baked meshes are queried directly so invisible pawn and camera blockers cannot hide a hole.
			for (TActorIterator<AActor> Actor(Level); Actor; ++Actor)
			{
				if (!Actor->IsA<AStormfallSet>() && !Actor->IsA<AHighWaterChamber>()) continue;
				TInlineComponentArray<UStaticMeshComponent*> Meshes(*Actor);
				for (UStaticMeshComponent* Mesh : Meshes)
				{
					const FName Name = Mesh->GetFName();
					if (Name == TEXT("RivenBasaltBluff") || Name == TEXT("SurveyCavern") || Name == TEXT("SM_CreekStoneA")
						|| Name == TEXT("SM_CreekStoneB") || Name == TEXT("SM_CreekStoneC")) Shells.Add(Mesh);
				}
			}
			if (Shells.Num() != 5) { Fail(TEXT("missing cave render meshes")); return; }
			Station = 0;
			Frames = 0;
			return;
		}
		const FCaveOrbitStation& Site = OrbitStations[Station];
		if (Frames++ == 0 && ViewIndex == 0) Place(Site.Feet);
		if (ViewIndex == 0 && Frames < 40) return;
		const int32 FirstFrame = ViewIndex == 0 ? 40 : 1;
		const FRotator Angle = ViewIndex == OrbitViewCount - 1 ? FRotator(32., 120., 0.)
			: FRotator(OrbitPitches[ViewIndex / OrbitYawCount], 360. * (ViewIndex % OrbitYawCount) / OrbitYawCount, 0.);
		if (Frames == FirstFrame) PC->SetControlRotation(Angle);
		if (Frames < FirstFrame + 3) return;

		const FMinimalViewInfo& View = PC->PlayerCameraManager->GetCameraCacheView();
		if (PC->GetViewTarget() != Max || !View.Rotation.Equals(Angle, 1.)) { Fail(TEXT("player orbit did not reach the active camera")); return; }
		const ULocalPlayer* Local = PC->GetLocalPlayer();
		FSceneViewProjectionData Projection;
		if (!Local || !Local->ViewportClient || !Local->GetProjectionData(Local->ViewportClient->Viewport, Projection))
			{ Fail(TEXT("test requires an active game viewport")); return; }
		ViewSize = Projection.GetConstrainedViewRect().Size();
		const FVector Eye = View.Location;
		const FRotationMatrix Axes(View.Rotation);
		const double Near = View.GetFinalPerspectiveNearClipPlane();
		const FVector Centre = Eye + Axes.GetUnitAxis(EAxis::X) * Near;
		const FVector Right = Axes.GetUnitAxis(EAxis::Y) * (Near / Projection.ProjectionMatrix.M[0][0]);
		const FVector Up = Axes.GetUnitAxis(EAxis::Z) * (Near / Projection.ProjectionMatrix.M[1][1]);
		UStaticMeshComponent* Crossed = IntersectedCaveMesh(Shells, Max->GetActorLocation(), Eye);
		++Views;
		for (int32 X = -1; X <= 1; ++X)
		{
			for (int32 Y = -1; Y <= 1; ++Y)
			{
				const FVector Sample = Centre + Right * (X - Projection.ProjectionMatrix.M[2][0])
					+ Up * (Y - Projection.ProjectionMatrix.M[2][1]);
				const FVector Ray = (Sample - Eye).GetSafeNormal();
				++Samples;
				if (const UStaticMeshComponent* Clip = Crossed ? Crossed : IntersectedCaveMesh(Shells, Eye, Sample))
				{
					++Clipped;
					if (++ClippedAtStation <= 2) UE_LOG(LogTemp, Warning, TEXT("CAVE CAMERA clipped at %s, pitch/yaw %.0f/%.0f, grid %d/%d, eye %s, mesh %s, from %s"),
						Site.Name, Angle.Pitch, Angle.Yaw, X, Y, *StormfallLayout::LocalPoint(Eye).ToCompactString(),
						*Clip->GetName(), Crossed ? TEXT("capsule centre") : TEXT("eye"));
				}
				if (!IntersectedCaveMesh(Shells, Sample, Sample + Ray * 8500.))
				{
					if (LooksOutOfCaveMouth(Sample, Ray)) { ++PortalRays; continue; }
					++Leaks;
					if (++LeaksAtStation <= 2) UE_LOG(LogTemp, Warning, TEXT("CAVE CAMERA open at %s, pitch/yaw %.0f/%.0f, grid %d/%d, eye %s, local ray %s"),
						Site.Name, Angle.Pitch, Angle.Yaw, X, Y, *StormfallLayout::LocalPoint(Eye).ToCompactString(),
						*StormfallLayout::Facing.UnrotateVector(Ray).ToCompactString());
				}
			}
		}
		Frames = 0;
		if (++ViewIndex == OrbitViewCount)
		{
			UE_LOG(LogTemp, Display, TEXT("CAVE CAMERA checked %s at feet %s, crossings %d, openings %d, cumulative mouth %d"), Site.Name,
				*StormfallLayout::LocalPoint(Max->GetCharacterMovement()->GetActorFeetLocation()).ToCompactString(), ClippedAtStation, LeaksAtStation, PortalRays);
			ClippedAtStation = LeaksAtStation = 0;
			++Station;
			ViewIndex = 0;
		}
	}, .016f, FTimerManagerTimerParameters{.bLoop=true,.bMaxOncePerFrame=true});
}

FAutoConsoleCommandWithWorld CaveOrbitProbe(TEXT("Wolf.TestCaveCamera"), TEXT("Orbit the active player camera from a cave checkpoint"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&TestCaveCamera));
}
#endif
