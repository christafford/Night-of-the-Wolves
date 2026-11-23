#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WolfPackComponent.generated.h"

class UWolfMeshComponent;
class UPointLightComponent;

// Encounter code owns how much lead Max has. The pack steers its members into position around
// that moving lead and quarry, so a moving or turning target changes the flankers' intercepts.
UCLASS()
class UWolfPackComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	void SpawnMembers();
	void Hide();
	UWolfMeshComponent* GetLeadWolf() const;
	void Hunt(float Dt, FVector Lead, FVector Heading, FVector Quarry, float Threat, bool bSwimming = false);
	void GuardDoor(float Dt, FVector Door, bool bRepelled);
	void CoverPounce(float Dt, const FVector& PreyFeet);
private:
	UPROPERTY() TArray<TObjectPtr<UWolfMeshComponent>> Wolves;
	UPROPERTY(Transient) TObjectPtr<UPointLightComponent> MoonBounce;
	UPROPERTY(EditDefaultsOnly, Category="Pack|Lighting", meta=(ClampMin="0.0", Units="cd"))
	float MoonBounceCandelas = 3.5f;
	UPROPERTY(EditDefaultsOnly, Category="Pack", meta=(ClampMin="0")) float FlankSpeed = 760.f;
	UPROPERTY(EditDefaultsOnly, Category="Pack", meta=(ClampMin="0")) float HuntWidth = 172.f;
	FVector LastQuarry = FVector::ZeroVector;
	FVector QuarryVelocity = FVector::ZeroVector;
	bool bPlaced = false;
	bool bGuarding = false;
	bool bCovering = false;
	FVector CoverMarks[2];
	void PlaceMoonBounce(const FVector& Ground);
	bool TraceSolidTerrain(FHitResult& Hit, const FVector& From, const FVector& To) const;
	FVector FeetOnGround(FVector Position) const;
	FVector AvoidObstacle(FVector From, FVector Toward, float Side) const;
	void Step(int32 Index, const FVector& Goal, const FVector& Face, float Dt, float Threat,
		bool bSwimming, bool bSnap, bool bFaceTarget, bool bGuard);
};
