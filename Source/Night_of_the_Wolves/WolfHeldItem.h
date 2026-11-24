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
	void UpdateSpadePose(class ANight_of_the_WolvesCharacter& Max, float Fraction);
	void EndDigPose();
	UPROPERTY(Transient) TObjectPtr<class UPoseableMeshComponent> DigBody;
	UPROPERTY() TObjectPtr<class UStaticMesh> ClodMesh;
	UPROPERTY() TObjectPtr<class UMaterialInterface> Soil;
	UPROPERTY(Transient) TArray<TObjectPtr<class UStaticMeshComponent>> Clods;
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
