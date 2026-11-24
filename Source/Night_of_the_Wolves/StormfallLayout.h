#pragma once

#include "CoreMinimal.h"

namespace StormfallLayout
{
	// Actor-local +X enters the hillside; +Y follows the side cleft behind the roots.
	inline const FVector Mouth(18030,1690,153);
	inline const FRotator Facing(0,90,0);
	inline FVector WorldPoint(const FVector& Local) { return Mouth+Facing.RotateVector(Local); }
	inline FVector LocalPoint(const FVector& World) { return Facing.UnrotateVector(World-Mouth); }

	inline const FVector StrikeTrigger(17445,965,103); // world, center of the approaching tread
	inline const FVector TreeFoot = WorldPoint({-875,-253,-33});
	inline const FVector Relight = WorldPoint({-444,-71,-24}); // glowing snapped branch at hip height above these feet
	inline const FVector Approach = WorldPoint({-346,12,-20});
	inline const FVector CaveEntry = WorldPoint({45,0,1});
	inline const FVector Chamber = WorldPoint({460,5,8});
	inline const FVector Roots = WorldPoint({499,230,11}); // curtain spans local X, normal +Y
	inline const FVector RootStance = WorldPoint({490,131,10});
	inline const FVector Through = WorldPoint({500,390,13});
	inline const FVector Outlet = WorldPoint({535,700,19});
	inline const FRotator OutletFacing(0,177,0);
	inline constexpr float FallBegins = .38f;
	inline constexpr float GroundImpact = 2.16f;
	inline constexpr float ReactionCut = GroundImpact+.17f;
	inline constexpr float Settled = ReactionCut+2.7f;
}
