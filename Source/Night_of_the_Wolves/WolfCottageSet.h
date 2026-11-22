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
	void SetShovel(bool bAvailable);
	void SetSupplies(bool bAvailable);
	void SetBrand(bool bAvailable);
	void Door(bool bOpen,bool bBarred);
	void SwingDoor(float Fraction);
	void Animate(float Time,float DigTime,float BarHealth,bool bBarred);
	void TrailPack(AWolfTrailRoute* Route,float Distance,const FVector& Player);
	void YardPack(const FVector& Target,float Separation,bool bBurning);
	void HidePack();
	void DoorPack(const FVector& Door, bool bRepelled);
	UWolfMeshComponent* GetLeadWolf() const;
	void Impact(const FVector& Location,float Volume,float Pitch);
	void WarnAtWindow();
	void ShoutAtDoor();
	void CurseFromPit();
private:
	UPROPERTY() TObjectPtr<UStaticMesh> Cube;
	UPROPERTY() TObjectPtr<UStaticMesh> Ball;
	UPROPERTY() TObjectPtr<UStaticMesh> Tube;
	UPROPERTY(VisibleAnywhere, Category="Pack") TObjectPtr<UWolfPackComponent> Pack;
	UPROPERTY() TObjectPtr<UMaterialInterface> Masonry;
	UPROPERTY() TObjectPtr<UMaterialInterface> Bark;
	UPROPERTY() TObjectPtr<UMaterialInterface> Soil;
	UPROPERTY() TObjectPtr<UMaterialInterface> Fire;
	UPROPERTY() TObjectPtr<UMaterialInterface> ShapeMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> Gold;
	UPROPERTY() TObjectPtr<UMaterialInterface> Cloth;
	UPROPERTY() TObjectPtr<UMaterialInterface> Iron;
	UPROPERTY() TObjectPtr<UMaterialInterface> RopeMat;
	UPROPERTY() TObjectPtr<USoundBase> ImpactSound;
	UPROPERTY() TObjectPtr<USoundBase> GrowlSound;
	UPROPERTY() TObjectPtr<USoundBase> FireSound;
	UPROPERTY() TObjectPtr<USoundBase> WarningVoice;
	UPROPERTY() TObjectPtr<USoundBase> ShoutVoice;
	UPROPERTY() TObjectPtr<USoundBase> TrappedVoice;
	UPROPERTY() TObjectPtr<class UAudioComponent> VoiceAudio;
	UPROPERTY() TObjectPtr<USoundAttenuation> Nearby;
	UPROPERTY() TArray<TObjectPtr<AActor>> DoorActors;
	UPROPERTY() TArray<TObjectPtr<AActor>> PitCover;
	UPROPERTY() TArray<TObjectPtr<AActor>> Shovel;
	UPROPERTY() TArray<TObjectPtr<AActor>> OldHermit;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> TableGoods;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Brand;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> WeakPlanks;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Bar;
	UPROPERTY() TObjectPtr<USceneComponent> WindowWarning;
	float NextGrowl=0;
	bool bPitOpen=false;
	bool bWarned=false;
	UStaticMeshComponent* Piece(UStaticMesh* Mesh,UMaterialInterface* Mat,FVector Position,FVector Scale,FRotator Rotation=FRotator::ZeroRotator,USceneComponent* Parent=nullptr,bool bSolid=false);
	UStaticMeshComponent* Stick(FVector From,FVector To,float Width,UMaterialInterface* Mat,USceneComponent* Parent=nullptr,bool bSolid=false);
	void Growl(const FVector& Lead, float Threat);
	void BuildProps();
	void BuildRoof();
	void Speak(USoundBase* Line,FVector Source,float Volume);
};
