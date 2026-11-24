#pragma once

#include "CoreMinimal.h"

namespace MaxDigStroke
{
	// The preparation and recovery at either end leave room to change grip.
	inline float Cycle(float Fraction)
	{
		return 3.f * FMath::Clamp((Fraction-.085f)/.84f, 0.f, 1.f);
	}
	inline constexpr float Plant = .31f;
}
