#include "Night_of_the_WolvesCharacter.h"
#include "Night_of_the_WolvesPlayerController.h"
#include "StormfallLayout.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "SceneView.h"

namespace
{
bool InCaveCameraArea(const FVector& WorldPosition)
{
	const FVector Local = StormfallLayout::LocalPoint(WorldPosition);
	static const FBox Vestibule(FVector(-125,-345,-100), FVector(965,385,480));
	static const FBox Cleft(FVector(245,150,-100), FVector(830,1775,680));
	static const FBox Chamber(FVector(70,1560,-100), FVector(1830,3760,1000));
	return Vestibule.IsInsideOrOn(Local) || Cleft.IsInsideOrOn(Local) || Chamber.IsInsideOrOn(Local);
}

float LensClearance(FMinimalViewInfo& View, const APlayerController& PC)
{
	int32 Width = 0, Height = 0;
	PC.GetViewportSize(Width, Height);
	if (Width <= 0 || Height <= 0)
	{
		Height = 1080;
		Width = FMath::Max(1, FMath::RoundToInt(Height * View.AspectRatio));
	}
	const FIntRect Rect(0, 0, Width, Height);
	FSceneViewProjectionData Projection;
	Projection.SetViewRectangle(Rect);
	const ULocalPlayer* Player = PC.GetLocalPlayer();
	const auto Axis = (Player ? Player : GetDefault<ULocalPlayer>())->AspectRatioAxisConstraint;
	FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle(View, Axis, Rect, Projection);
	const FMatrix& Matrix = Projection.ProjectionMatrix;
	// Farthest corner of the rendered near plane, including any off-centre projection.
	const double HalfWidth = (1. + FMath::Abs(Matrix.M[2][0])) / Matrix.M[0][0];
	const double HalfHeight = (1. + FMath::Abs(Matrix.M[2][1])) / Matrix.M[1][1];
	return View.GetFinalPerspectiveNearClipPlane() * FMath::Sqrt(1. + HalfWidth*HalfWidth + HalfHeight*HalfHeight) + 2.;
}
}

void ANight_of_the_WolvesCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);
	const FVector Centre = GetActorLocation();
	if (!InCaveCameraArea(Centre) || !FollowCamera->IsActive() || OutResult.ProjectionMode != ECameraProjectionMode::Perspective)
	{
		return;
	}
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	const FCollisionShape Lens = FCollisionShape::MakeSphere(FMath::Max(CameraBoom->ProbeSize, LensClearance(OutResult, *PC)));
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(MaxCaveCamera), true, this);
	UWorld* World = GetWorld();
	const ECollisionChannel Channel = CameraBoom->ProbeChannel;
	const auto MoveLens = [&](const FVector& Start, const FVector& End) -> FVector
	{
		if (Start.Equals(End, .01)) return Start;
		FHitResult Hit;
		if (!World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, Channel, Lens, Query)) return End;
		if (Hit.bStartPenetrating) return Start;
		return Hit.Location + (Start - End).GetSafeNormal() * FMath::Min(1., FVector::Distance(Start, Hit.Location));
	};

	// Start inside Max's movement capsule. The spring arm starts its own sweep at
	// the offset shoulder, which can already be across the rock when Max turns.
	FVector Anchor = Centre;
	if (World->OverlapBlockingTestByChannel(Anchor, FQuat::Identity, Channel, Lens, Query))
	{
		const double Step = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * .45;
		for (double Offset : {-Step, Step})
		{
			const FVector Candidate = Centre + FVector::UpVector * Offset;
			if (!World->OverlapBlockingTestByChannel(Candidate, FQuat::Identity, Channel, Lens, Query))
			{
				Anchor = Candidate;
				break;
			}
		}
	}
	const FVector Shoulder = MoveLens(Anchor, CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset);
	OutResult.Location = MoveLens(Shoulder, OutResult.Location);

	// At full retraction the template socket is near Max's waist. Bring the
	// view up toward eye level as the arm runs out of space against the wall.
	const double Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const double Retraction = 1. - FMath::SmoothStep(Radius, Radius + 32., FVector::Dist2D(Centre, OutResult.Location));
	const double Lift = FMath::Max(0., Centre.Z + BaseEyeHeight - OutResult.Location.Z) * Retraction;
	if (Lift > 0.) OutResult.Location = MoveLens(OutResult.Location, OutResult.Location + FVector::UpVector * Lift);
}

void ANight_of_the_WolvesPlayerController::UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& HiddenComponents)
{
	Super::UpdateHiddenComponents(ViewLocation, HiddenComponents);
	const ACharacter* Max = GetCharacter();
	if (!Max || GetViewTarget() != Max || !InCaveCameraArea(Max->GetActorLocation())) return;
	const UCapsuleComponent* Capsule = Max->GetCapsuleComponent();
	const FVector Offset = ViewLocation - Capsule->GetComponentLocation();
	// When the eye is inside the body, omit Max and his carried mesh from
	// this view. The firebrand's point light still illuminates the cave.
	if (Offset.SizeSquared2D() > FMath::Square(Capsule->GetScaledCapsuleRadius() + 8.) ||
		FMath::Abs(Offset.Z) > Capsule->GetScaledCapsuleHalfHeight() + 8.) return;
	TInlineComponentArray<UPrimitiveComponent*> Parts(Max);
	for (UPrimitiveComponent* Part : Parts)
	{
		if (Part->IsRegistered()) HiddenComponents.Add(Part->GetPrimitiveSceneId());
	}
}
