#include "FirstLegCrossing.h"
#include "FirstLegCablewayLayout.h"
#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"

using namespace FirstLegCableway;

void AFirstLegCrossing::BuildCableway()
{
	const FRotator Yaw = Axis.Rotation();
	const FVector Head(NearStop.X, NearStop.Y, 0);
	const FVector Tail(FarStop.X, FarStop.Y, 0);
	const double Span = FVector::Dist2D(Head, Tail);
	auto* HeadStation = Part(SkylineHead, nullptr);
	auto* TailStation = Part(SkylineTail, nullptr);
	HeadStation->SetWorldLocationAndRotation(Head, Yaw);
	TailStation->SetWorldLocationAndRotation(Tail, Yaw);

	// Match the solid walk to the cross-laid planks. A smooth underlying ramp
	// keeps the capsule out of the seams between the uphill treads.
	auto Deck = [this](FVector Start, FVector End, double Width)
	{
		const FVector D = End-Start;
		Stamp(Cube, nullptr, (Start+End)*.5 - FVector(0,0,4.2),
			{D.Size()/100., Width/100., .084}, FRotationMatrix::MakeFromX(D).Rotator(), true, false, TEXT("TimberFootsteps"));
	};
	auto Board = [](FVector Bank, double Along, double Z) { return Bank + Axis*Along + FVector(0,0,Z); };
	Deck(Board(Head,-268,49), Board(Head,-68,123), 158.);
	Deck(Board(Head,-68,123), Board(Head,52,123), 158.);
	const double Lip = (CatchDeckStart-1)*Span-7.;
	Deck(Board(Tail,Lip,CatchDeckHeight(Lip)), Board(Tail,76,CatchDeckHeight(76)), 186.);
	Deck(Board(Tail,76,CatchDeckHeight(76)), Board(Tail,147,CatchDeckHeight(147)), 186.);

	const double StartGrade = (FarStop.Z-NearStop.Z-32.)/Span;
	const double EndGrade = (FarStop.Z-NearStop.Z+32.)/Span;
	const FVector NearSaddle = Board(Head, -145., NearStop.Z-145.*StartGrade);
	const FVector FarSaddle = Board(Tail, 246., FarStop.Z+246.*EndGrade);
	LogBeam(NearSaddle, NearStop, 1.55f, SkylineSteel);
	FVector Last = NearStop;
	for (int32 I = 1; I <= 24; ++I)
	{
		const FVector Next = WireContact(static_cast<float>(I)/24.f);
		LogBeam(Last, Next, 1.55f, SkylineSteel);
		Last = Next;
	}
	LogBeam(FarStop, FarSaddle, 1.55f, SkylineSteel);
	for (int32 Sign : {-1,1})
	{
		const FVector Saddle = Sign == -1 ? NearSaddle : FarSaddle;
		const FVector Deadman = Saddle + Axis*(Sign*208.) - Side*172.;
		LogBeam(Saddle, FVector(Deadman.X, Deadman.Y, 44), 1.65f, SkylineSteel);
		const FVector Bumper = (Sign == -1 ? NearStop : FarStop) + Axis*(Sign*39.) + FVector(0,0,39.*Sign*(Sign == -1 ? StartGrade : EndGrade));
		LogBeam(Bumper, Bumper+Axis*(Sign*20.), 5.9f, BlackMat);
		for (int32 I=0; I<6; ++I)
		{
			const FVector Collar = Bumper + Axis*(Sign*(2.3+I*3.));
			LogBeam(Collar, Collar+Axis*1.3, 6.7f, SkylineSteel);
		}
	}

	// Raking moonlight separates the wheel, sling and downriver landing from the
	// conifers behind them. A broad source avoids a hard circle on the timber.
	auto MoonPatch = [this](const FVector& P, float Cd, float Radius)
	{
		auto* Sky = NewObject<UPointLightComponent>(this);
		Sky->SetupAttachment(RootComponent);
		Sky->SetRelativeLocation(P);
		Sky->SetIntensityUnits(ELightUnits::Candelas);
		Sky->SetIntensity(Cd);
		Sky->SetAttenuationRadius(Radius);
		Sky->SetSourceRadius(72.f);
		Sky->SetSoftSourceRadius(104.f);
		Sky->SetLightColor(FLinearColor(.68,.81,1.));
		Sky->SetCastShadows(false);
		Sky->SetVolumetricScatteringIntensity(0);
		AddInstanceComponent(Sky);
		Sky->RegisterComponent();
	};
	MoonPatch(NearStop - Axis*113. + Side*115. + FVector(0,0,74), 2.25f, 620.f);
	MoonPatch(FarStop - Axis*47. - Side*94. + FVector(0,0,56), 1.8f, 560.f);

	Ironwork = Part(SkylineCarriage, nullptr);
	TrolleyWheel = Part(SkylineSheave, nullptr);
	Cord = Part(SkylineSling, nullptr);
	CastBight = Part(SkylineBight, nullptr);
	for (int32 I = 0; I < 8; ++I)
		CastTail.Add(Part(Pole, CastBight->GetMaterial(8)));
	SetTrolleyPose(0.f,0.f);
}

void AFirstLegCrossing::SetTrolleyPose(float FractionOnCable, float SlingFraction)
{
	const float T = FMath::Clamp(FractionOnCable, 0.f, 1.f);
	const float Rigged = FMath::Clamp(SlingFraction, 0.f, 1.f);
	const FVector Contact = WireContact(T);
	const FQuat Facing = Axis.Rotation().Quaternion();
	Ironwork->SetWorldLocationAndRotation(Contact, Facing);
	// 24.75 cm is the pitch radius at the bottom of the groove. The casting sits
	// on top of the 3.1 cm wire. The wheel turns independently inside its cheeks.
	const double Distance = T * FVector::Dist2D(NearStop, FarStop);
	const FQuat Spin(FVector::RightVector, Distance/24.75);
	TrolleyWheel->SetWorldLocationAndRotation(Contact + FVector(0,0,26.3), Facing*Spin);
	Cord->SetWorldLocationAndRotation(HookSaddle(T), Facing);
	Cord->SetWorldScale3D({1.,1.,FMath::Max(.01f,Rigged)});
	Cord->SetVisibility(Rigged > 0.f);
	CastBight->SetVisibility(false);
	for (const auto& Segment : CastTail) Segment->SetVisibility(false);
}

void AFirstLegCrossing::ShowRopeCast(float Progress)
{
	// The lower end pays out of Max's left hand as his right throws an open
	// bight. It clears the upturned horn, drops onto its saddle and pulls taut.
	const FVector Right = HangingPose->GetSocketLocation(TEXT("hand_r"));
	const FVector Left = HangingPose->GetSocketLocation(TEXT("hand_l"));
	constexpr float Release = .32f, Catch = .70f;
	const float Travel = FMath::GetMappedRangeValueClamped(FVector2f(Release, Catch), FVector2f(0, 1), Progress);
	const float Unfurl = FMath::SmoothStep(Catch, .98f, Progress);
	SetTrolleyPose(0.f, Unfurl);
	if (Progress < Release || CastOrigin.IsZero()) CastOrigin = Right;
	const FVector Seat = HookSaddle(0) - Side * 7.;
	const FVector ApexOffset = (FVector::UpVector * 158. - Side * 65.) * (4.f * Travel * (1.f - Travel));
	const FVector Bight = FMath::Lerp(CastOrigin, Seat, Travel) + ApexOffset;
	const float Open = 1.f - FMath::SmoothStep(Catch, .81f, Progress);
	CastBight->SetWorldLocationAndRotation(Bight, Axis.Rotation());
	CastBight->SetWorldScale3D(FVector(FMath::Max(.01f,Open)));
	CastBight->SetVisibility(Open > .01f);

	const FVector Top = FMath::Lerp(Bight - Side * (15. * Open), GripCenter(0), Unfurl);
	const FVector Bottom = FMath::Lerp(Left, GripCenter(0), FMath::SmoothStep(.77f, .98f, Progress));
	FVector Prev = Top;
	for (int32 I = 0; I < CastTail.Num(); ++I)
	{
		const float Along = float(I+1) / CastTail.Num();
		const float Slack = 4.f * Along * (1.f-Along) * (1.f-Unfurl);
		const FVector Next = FMath::Lerp(Top, Bottom, Along) + (Side*11. - FVector::UpVector*32.) * Slack;
		const FVector Span = Next - Prev;
		auto* Strand = CastTail[I].Get();
		Strand->SetVisibility(Span.SizeSquared() > 1.);
		if (Strand->IsVisible())
		{
			Strand->SetWorldLocationAndRotation((Prev+Next)*.5, FRotationMatrix::MakeFromZ(Span).ToQuat());
			Strand->SetWorldScale3D(FVector(.029, .029, Span.Size()/100.));
		}
		Prev = Next;
	}
}
