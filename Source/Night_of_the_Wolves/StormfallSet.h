#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StormfallSet.generated.h"

class UAudioComponent;
class UBoxComponent;
class UCameraComponent;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class NIGHT_OF_THE_WOLVES_API AStormfallSet : public AActor
{
	GENERATED_BODY()
public:
	AStormfallSet();
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;
	void Restore(bool bFallen,bool bRootsCleared);
	void SetStormPose(float SecondsSinceStrike);
	void SetPuzzlePose(float BurnFraction);
private:
	UPROPERTY() TObjectPtr<UCameraComponent> StrikeCamera;
	TWeakObjectPtr<AActor> PreviousView;
	void EndStrikeShot(float Blend);
	void BuildFractureFaces();
	void FrameRunningPack(bool bCut);
	FVector FirstPackCenter=FVector::ZeroVector;
	FVector ReactionOffset=FVector::ZeroVector;
	TArray<int32> FramedWolves;
	UPROPERTY() TObjectPtr<USceneComponent> FallingTree;
	UPROPERTY() TObjectPtr<UBoxComponent> LogBlock;
	UPROPERTY() TObjectPtr<UBoxComponent> RootBlock;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Bolt;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> BurnMarks;
	UPROPERTY() TObjectPtr<UPointLightComponent> Flash;
	UPROPERTY() TObjectPtr<UPointLightComponent> FireLight;
	UPROPERTY() TObjectPtr<UPointLightComponent> EmberLight;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Flames;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Sparks;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> RootStrands;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> RootEmbers;
	UPROPERTY() TObjectPtr<UAudioComponent> Thunder;
	UPROPERTY() TObjectPtr<UAudioComponent> Break;
	UPROPERTY() TObjectPtr<UAudioComponent> Crash;
	UPROPERTY() TObjectPtr<UAudioComponent> Crackle;
	UPROPERTY() TObjectPtr<UAudioComponent> RootsBurn;
	UPROPERTY() TObjectPtr<UAudioComponent> CaveDrips;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HeroFir;
	TArray<FVector> FireAnchors;
	float PreviousStorm=-1.f, Burn=0.f;
	bool bFire=false;
};
