#pragma once
#include "CoreMinimal.h"

// The water runs across the player's route. Both banks bend along the same channel.
namespace FirstLegLayout
{
	inline const FVector CreekApproach(9400, -1200, 144);
	inline const FVector Takeoff(9740, -1190, 144);
	inline const FVector Landing(11150, -700, 144);
	inline const FVector Anchor(10445, -945, 1595);
	inline const FVector BanditMark(14550, 135, 50);
	// Fixed clearing behind the demand line, on the approach side of the bend.
	inline const FVector GoldPatch(13400, -790, 57);
	inline const FVector Escape(17100, 800, 145);
	inline const FVector Road[] = {{8420,-1180,50},{9450,-1200,50},{9810,-1200,50}};
	inline const FVector FarRoad[] = {{10970,-715,50},{11900,-800,50},{12800,-980,45},{13450,-770,47},
		{14050,-300,50},{14700,230,50},{15600,640,55},{16400,710,80},{17200,880,95},{18600,1350,130}};
	inline float StreamBend(float Y)
	{
		const float T = (Y + 1100.f) / 1650.f;
		return 150.f * FMath::Sin(T) + 35.f * FMath::Sin(2.4f*T);
	}
}
