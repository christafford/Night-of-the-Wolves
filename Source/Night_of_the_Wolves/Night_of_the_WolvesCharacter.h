// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Night_of_the_WolvesCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimInstance;
class UGroomBindingAsset;
class UGroomComponent;
class USkeletalMesh;
class UPoseableMeshComponent;
class USkinnedMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FInputActionValue;
enum class EVisibilityBasedAnimTickOption : uint8;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ANight_of_the_WolvesCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, Category="Max")
	TObjectPtr<USkeletalMeshComponent> Face;

	UPROPERTY(VisibleAnywhere, Category="Max")
	TObjectPtr<USkeletalMeshComponent> Outfit;

	UPROPERTY(VisibleAnywhere, Category="Max")
	TObjectPtr<UStaticMeshComponent> Hair;

	UPROPERTY(VisibleAnywhere, Category="Max")
	TObjectPtr<UGroomComponent> Fringe;

	UPROPERTY(VisibleAnywhere, Category="Max")
	TObjectPtr<UGroomComponent> Brows;

	UPROPERTY(EditDefaultsOnly, Category="Max")
	TObjectPtr<UGroomBindingAsset> MaxFringeBinding;

	UPROPERTY(EditDefaultsOnly, Category="Max")
	TObjectPtr<UGroomBindingAsset> MaxBrowBinding;

	UPROPERTY(EditDefaultsOnly, Category="Max")
	TObjectPtr<USkeletalMesh> MaxBody;

	UPROPERTY(EditDefaultsOnly, Category="Max")
	TObjectPtr<USkeletalMesh> MaxHead;

	UPROPERTY(EditDefaultsOnly, Category="Max")
	TObjectPtr<USkeletalMesh> MaxOutfit;

	UPROPERTY(EditDefaultsOnly, Category="Max")
	TObjectPtr<UStaticMesh> MaxHair;

	UPROPERTY(EditDefaultsOnly, Category="Max")
	TSubclassOf<UAnimInstance> MaxLocomotion;

	UPROPERTY(EditDefaultsOnly, Category="Max")
	TSubclassOf<UAnimInstance> MaxFaceAnimation;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

public:

	/** Constructor */
	ANight_of_the_WolvesCharacter();	
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;

protected:

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual bool CanJumpInternal_Implementation() const override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Keep the clothes and head aligned with Max's grip during the cable crossing. */
	void SetHangingAppearance(USkinnedMeshComponent* HangingBody);
	void BeginDigAppearance(USkinnedMeshComponent* WorkingBody);
	void FollowDigHead(const FTransform& Head);
	void EndDigAppearance();
	UPoseableMeshComponent* BeginWorkingPose();
	void EndWorkingPose(UPoseableMeshComponent* Pose);

	/** Contact is a point on the outside face of the closed door, in world space. */
	void PoseDoorKnock(const FVector& Contact, const FVector& OutwardNormal, float Fraction);
	void EndDoorKnock();
	const UPoseableMeshComponent* GetKnockBody() const { return KnockBody; }
	FVector KnockingKnuckle() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> KnockBody;
	TArray<FTransform> KnockStance;
	bool bKnockLeft = false;
	bool bWorkingBodyWasVisible = true;
	EVisibilityBasedAnimTickOption WorkingBodyTick;
	FTransform RestFaceRelative;
	FTransform FaceFromHead;
public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

