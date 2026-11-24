#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WolfCottageSet.generated.h"

class AWolfTrailRoute;
class UWolfPackComponent;
class UWolfMeshComponent;
class UStaticMeshComponent;
class USoundBase;
class USoundAttenuation;
class UMaterialInterface;

// Physical puzzle pieces and reusable presentation for the opening and cottage.
UCLASS()
class AWolfCottageSet : public AActor
{
	GENERATED_BODY()
public:
	AWolfCottageSet();
	virtual void BeginPlay() override;
	void RestoreProps(bool bHermitCaught);
	void SetPit(bool bDug);
	const FBox& GetPitBounds() const { return PitBounds; }
	FVector PitSurface() const { return FVector(PitBounds.GetCenter().X,PitBounds.GetCenter().Y,PitBounds.Max.Z); }
	void SetShovel(bool bAvailable);
	void SetRope(bool bAvailable);
	void SetGold(bool bAvailable);
	FVector GetRopeLocation() const;
	FVector GetGoldLocation() const;
	void SetBrand(bool bAvailable);
	void Door(bool bOpen,bool bBarred);
	void SwingDoor(float Fraction);
	void Animate(float Time,float DigTime,float BarHealth,bool bBarred);
	void TrailPack(AWolfTrailRoute* Route,float Lead);
	void YardPack(const AWolfTrailRoute* Route,float Separation,bool bBurning);
	void DoorPack(const FVector& Door, bool bRepelled, bool bExitExposed=false, bool bBreached=false);
	UWolfMeshComponent* GetLeadWolf() const;
	void Impact(const FVector& Location,float Volume,float Pitch);
	void HideWindowBarrel();
private:
	UPROPERTY() TObjectPtr<UStaticMesh> Cube;
	UPROPERTY() TObjectPtr<UStaticMesh> Ball;
	UPROPERTY() TObjectPtr<UStaticMesh> Tube;
	UPROPERTY(Transient) TObjectPtr<UWolfPackComponent> Pack;
	UPROPERTY() TObjectPtr<UMaterialInterface> Masonry;
	UPROPERTY() TObjectPtr<UMaterialInterface> Bark;
	UPROPERTY() TObjectPtr<UMaterialInterface> Soil;
	UPROPERTY() TObjectPtr<UMaterialInterface> Fire;
	UPROPERTY() TObjectPtr<UMaterialInterface> ShapeMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> Gold;
	UPROPERTY() TObjectPtr<UMaterialInterface> Cloth;
	UPROPERTY() TObjectPtr<UMaterialInterface> Iron;
	UPROPERTY() TObjectPtr<UStaticMesh> RopeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> PouchMesh;
	UPROPERTY() TObjectPtr<USoundBase> ImpactSound;
	UPROPERTY() TObjectPtr<USoundBase> GrowlSound;
	UPROPERTY() TObjectPtr<USoundBase> FireSound;
	UPROPERTY() TObjectPtr<USoundAttenuation> Nearby;
	UPROPERTY() TArray<TObjectPtr<AActor>> DoorActors;
	UPROPERTY() TArray<TObjectPtr<AActor>> PitCover;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> YardShovel;
	UPROPERTY() TArray<TObjectPtr<AActor>> OldHermit;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> RopeCoil;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> GoldSack;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Brand;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> WeakPlanks;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Bar;
	UPROPERTY() TObjectPtr<USceneComponent> WindowWarning;
	float NextGrowl=0;
	FBox PitBounds = FBox(ForceInit);
	bool bPitOpen=false;
	UStaticMeshComponent* Piece(UStaticMesh* Mesh,UMaterialInterface* Mat,FVector Position,FVector Scale,FRotator Rotation=FRotator::ZeroRotator,USceneComponent* Parent=nullptr,bool bSolid=false);
	UStaticMeshComponent* Stick(FVector From,FVector To,float Width,UMaterialInterface* Mat,USceneComponent* Parent=nullptr,bool bSolid=false);
	void Growl(const FVector& Lead, float Threat);
	void CreateYardProps();
	void BuildProps();
	void BuildRoof();
};
