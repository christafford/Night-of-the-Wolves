#include "DialogueArticulation.h"

#include "Math/UnrealMathUtility.h"
#include "Templates/UnrealTemplate.h"

namespace
{
struct FPackedMouthPose
{
	uint8 JawOpen;
	uint8 LipRound;
	uint8 LipWide;
	uint8 LipClose;
	uint8 LipTeeth;
};

struct FMouthTrack
{
	const FPackedMouthPose* Frames;
	int32 Count;
	float Duration;
};

#include "DialogueArticulationData.inl"
}

float GetDialogueDuration(EDialogueLine Line)
{
	return GetMouthTrack(Line).Duration;
}

FDialogueMouthPose SampleDialogueMouth(EDialogueLine Line, float Seconds)
{
	const FMouthTrack Track = GetMouthTrack(Line);
	if (!(Seconds > 0.f && Seconds < Track.Duration))
	{
		return {};
	}

	const float Frame = Seconds * MouthFramesPerSecond;
	const int32 Index = FMath::Min(FMath::FloorToInt(Frame), Track.Count - 2);
	const float Alpha = Frame - Index;
	const FPackedMouthPose& A = Track.Frames[Index];
	const FPackedMouthPose& B = Track.Frames[Index + 1];
	auto Blend = [Alpha](uint8 Start, uint8 End)
	{
		return (Start + (static_cast<float>(End) - Start) * Alpha) / 255.f;
	};
	return {
		Blend(A.JawOpen, B.JawOpen),
		Blend(A.LipRound, B.LipRound),
		Blend(A.LipWide, B.LipWide),
		Blend(A.LipClose, B.LipClose),
		Blend(A.LipTeeth, B.LipTeeth)
	};
}
