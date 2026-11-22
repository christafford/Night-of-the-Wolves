#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WolfTrailHUD.generated.h"

UCLASS()
class NIGHT_OF_THE_WOLVES_API AWolfTrailHUD : public AHUD
{
	GENERATED_BODY()
public:
	AWolfTrailHUD();
	virtual void DrawHUD() override;
private:
	UPROPERTY()
	TObjectPtr<class UTexture2D> WolfAttackTexture;
};
