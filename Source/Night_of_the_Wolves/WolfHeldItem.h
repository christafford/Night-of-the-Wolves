#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WolfHeldItem.generated.h"

UCLASS()
class UWolfHeldItem : public USceneComponent
{
	GENERATED_BODY()
public:
	UWolfHeldItem();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
private:
	UPROPERTY() TMap<FName,TObjectPtr<class UStaticMesh>> Props;
	UPROPERTY() TObjectPtr<class UStaticMeshComponent> Held;
	UPROPERTY() TObjectPtr<class UStaticMeshComponent> Flame;
	UPROPERTY() TObjectPtr<class UPointLightComponent> Glow;
	UPROPERTY() TObjectPtr<UStaticMesh> FlameShape;
	UPROPERTY() TObjectPtr<class UMaterialInterface> EmberMaterial;
	FName Shown;
	bool bWasBurning = false;
	float SnuffRemaining = 0.f;
};
