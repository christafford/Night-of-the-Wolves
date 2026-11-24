#pragma once

#include "StormfallLayout.h"

// Feet or deck positions, in the same local space as the root passage. +Y goes deeper into the hill.
namespace HighWaterLayout
{
inline constexpr float BasinZ = 26.f;
inline constexpr float LowWater = 16.f;
inline constexpr float HighWater = 248.f;
inline constexpr float ShelfZ = 264.f;
inline constexpr float LandingZ = 532.f;
inline const FVector Entrance(550, 930, 23);
inline const FVector Shelf(535, 1710, ShelfZ);
inline const FVector LowerFork(246, 1910, ShelfZ);
inline const FVector Trough(472, 2280, ShelfZ);
inline const FVector StepsTop(528, 2010, ShelfZ);
inline const FVector StepsBottom(1210, 2010, BasinZ);
inline const FVector Pin(1120, 2320, BasinZ);
inline const FVector LooseFloat(994, 2285, BasinZ + 14);
inline const FVector WrongFloat(1332, 2420, BasinZ + 14);
inline const FVector Guides(1010, 2710, BasinZ + 14);
inline const FVector Board(835, 2710, ShelfZ);
inline const FVector UpperForkLow(1100, 2710, BasinZ + 14);
inline const FVector LowerClimb(1070, 2710, HighWater + 12);
inline const FVector Landing(1290, 2710, LandingZ);
inline const FVector Exit(1535, 3290, LandingZ + 10);
// The long surveyed gallery sits below the bandit's wooded ridge. Check the
// whole crown: a fir planted outside the rock can still occlude the ladder.
inline bool IntersectsSurveyCut(const FBox& WorldBounds)
{
	const FTransform Frame(StormfallLayout::Facing, StormfallLayout::Mouth);
	static const FBox Cleft = FBox(FVector(330,895,0), FVector(750,1717,614)).TransformBy(Frame);
	static const FBox Chamber = FBox(FVector(130,1708,0), FVector(1710,3680,898)).TransformBy(Frame);
	return Cleft.Intersect(WorldBounds) || Chamber.Intersect(WorldBounds);
}
inline FVector World(const FVector& Local) { return StormfallLayout::WorldPoint(Local); }
}
