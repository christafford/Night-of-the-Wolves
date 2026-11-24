#pragma once

#include "CoreMinimal.h"

class UAudioComponent;
class UPointLightComponent;
class UStaticMeshComponent;

// All components below are native default subobjects of the chamber.
struct FHighWaterScenery
{
	UStaticMeshComponent* Pool = nullptr;
	UStaticMeshComponent* Float = nullptr;
	UStaticMeshComponent* Trough = nullptr;
	UStaticMeshComponent* Timber = nullptr;
	UStaticMeshComponent* Stream = nullptr;
	UStaticMeshComponent* Spill = nullptr;
	UStaticMeshComponent* Bypass = nullptr;
	UStaticMeshComponent* Brand = nullptr;
	UStaticMeshComponent* Flame = nullptr;
	UPointLightComponent* TorchLight = nullptr;
	UPointLightComponent* ReflectedFire = nullptr;
	UAudioComponent* Spring = nullptr;
	UAudioComponent* Seep = nullptr;
	UAudioComponent* Tap = nullptr;
	UAudioComponent* Grind = nullptr;
	float TapIn = 1.4f;
	bool bTurning = false;
	bool bRolling = false;
};
