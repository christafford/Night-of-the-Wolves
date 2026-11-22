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
#if !UE_BUILD_SHIPPING
	TSet<FString> ReviewShots;
#endif
	UPROPERTY() TObjectPtr<class UTexture2D> WolfAttackTexture;
	UPROPERTY() TObjectPtr<class UTexture2D> OpeningTitleTexture;
	void DrawOpeningTitle(float Opacity);
	void CenterText(const FString& Str, float X, float Y, float Size, FLinearColor Color);
	void Wheel(class ANight_of_the_WolvesPlayerController& PC,const struct FWolfInventory& Inventory);
	void Icon(FName Item,FVector2D At,float Size,FLinearColor Color);
	void Ring(FVector2D At,float Radius,FLinearColor Color,float Thickness);
};
