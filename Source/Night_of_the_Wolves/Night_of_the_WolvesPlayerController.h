// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Night_of_the_WolvesPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ANight_of_the_WolvesPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	enum class EOverlay { None, Inventory, Pause, ConfirmNew };
	static constexpr int32 SlotsPerPage = 6;
	EOverlay GetOverlay() const { return Overlay; }
	int32 WheelPage() const { return Selection / SlotsPerPage; }
	int32 Highlight() const { return Selection; }
	bool UsingPad() const { return bUsingPad; }
	const FString& ItemNotice() const { return Notice; }
	float NoticeUntil = 0.f;
	void UpdatePointerSelection();
#if !UE_BUILD_SHIPPING
	void RequestTestEquip(FName Item);
	bool bTestSelecting = false;
#endif


private:
	EOverlay Overlay = EOverlay::None;
	int32 Selection = 0;
	bool bUsingPad = false;
	bool bTrackCursor = false;
	FVector2D LastMouse = FVector2D::ZeroVector;
	float NormalSpeed = 0.f;
	FString Notice;
	void MenuKey(FKey Key);
	void PageForward();
	void PageBack();
	void Previous();
	void Next();
	void Confirm();
	void Cancel();
	void ToggleWheel();
	void TogglePause();
	void Interact();
	void UseItem();
	void SprintStart();
	void SprintStop();
	void Retry();
	void OpenOverlay(EOverlay NewOverlay);
	void CloseOverlay();
	void ShiftSelection(int32 Change);
	void ChangePage(int32 Direction);
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void UpdateHiddenComponents(const FVector& ViewLocation, TSet<FPrimitiveComponentId>& HiddenComponents) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;
	virtual void BuildInputStack(TArray<UInputComponent*>& InputStack) override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};
