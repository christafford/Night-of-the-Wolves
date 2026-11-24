#pragma once

#include "FirstLegCrossingLayout.h"

namespace FirstLegCableway
{
	inline const FVector Axis = (FirstLegLayout::Landing - FirstLegLayout::Takeoff).GetSafeNormal2D();
	inline const FVector Side(-Axis.Y, Axis.X, 0);
	inline const FVector NearStop(FirstLegLayout::Takeoff.X, FirstLegLayout::Takeoff.Y, 631.);
	inline const FVector FarStop = FVector(FirstLegLayout::Landing.X, FirstLegLayout::Landing.Y, 581.) - Axis * 215.;
	inline constexpr float CatchDeckStart = .86f;
	inline double CatchDeckHeight(double AlongFromFarStop)
	{
		return FMath::Lerp(60., 48., FMath::Clamp((AlongFromFarStop - 76.) / 71., 0., 1.));
	}

	inline FVector WireContact(float T)
	{
		// The slight dip puts a little more grade at launch and flattens the runout.
		return FMath::Lerp(NearStop, FarStop, T) - FVector(0, 0, 8. * 4. * T * (1. - T));
	}

	inline FVector HookSaddle(float T) { return WireContact(T) - FVector(0, 0, 106); }
	inline FVector GripCenter(float T) { return HookSaddle(T) - FVector(0, 0, 211); }
	inline FVector GripPosition(float T, bool bLeft)
	{
		return GripCenter(T) + Side * (bLeft ? -5. : 5.) + FVector(0, 0, bLeft ? -3. : 3.);
	}
	inline FVector RiderPosition(float T) { return GripCenter(T) - Axis * 12. - FVector(0, 0, 98); }
}
