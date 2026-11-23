#pragma once

#include "CoreTypes.h"

enum class EDialogueLine : uint8
{
	None,
	HermitWarning,
	HermitShout,
	HermitPit,
	BanditDemand,
	BanditGold,
	BanditStay,
	HermitFallYell
};

// Normalized articulators. All-zero is the unmodified facial rest pose.
struct FDialogueMouthPose
{
	float JawOpen = 0.f;
	float LipRound = 0.f;
	float LipWide = 0.f;
	float LipClose = 0.f;
	float LipTeeth = 0.f;
};

FDialogueMouthPose SampleDialogueMouth(EDialogueLine Line, float Seconds);
float GetDialogueDuration(EDialogueLine Line);
