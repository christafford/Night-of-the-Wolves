#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WolfTrailSave.generated.h"

class AWolfTrailGameMode;

UCLASS()
class UWolfTrailSave : public USaveGame
{
	GENERATED_BODY()
public:
	static void Resume(AWolfTrailGameMode& Mode);
	static void Write(AWolfTrailGameMode& Mode);
	static void Clear(AWolfTrailGameMode& Mode);
private:
	UPROPERTY() int32 Format = 1;
	UPROPERTY() int32 Stage = 0;
	UPROPERTY() TArray<FName> Bag;
	UPROPERTY() FName Equipment;
	UPROPERTY() bool bLit = false;
	UPROPERTY() FTransform Spawn;
	bool IsUsable() const;
	static bool Enabled(AWolfTrailGameMode& Mode);
};
