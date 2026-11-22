// Copyright Epic Games, Inc. All Rights Reserved.


#include "Night_of_the_WolvesPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "WolfTrailGameMode.h"
#include "Components/InputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Night_of_the_Wolves.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ANight_of_the_WolvesPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogNight_of_the_Wolves, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ANight_of_the_WolvesPlayerController::BuildInputStack(TArray<UInputComponent*>& InputStack)
{
	// This covers the pawn's Enhanced Input actions and our raw menu key bindings,
	// including the first frame before the fresh-start/checkpoint decision.
	const auto* Mode = GetWorld()->GetAuthGameMode<AWolfTrailGameMode>();
	if (!Mode || !Mode->IsOpeningCinematicPlaying()) Super::BuildInputStack(InputStack);
}

void ANight_of_the_WolvesPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// The template Blueprint retains its Enhanced Input movement and camera mappings.
	for (FKey Key : {EKeys::Tab, EKeys::Gamepad_LeftShoulder, EKeys::Escape, EKeys::Gamepad_Special_Right,
		EKeys::E, EKeys::Gamepad_FaceButton_Left, EKeys::F, EKeys::Gamepad_RightTrigger,
		EKeys::LeftMouseButton, EKeys::RightMouseButton, EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom,
		EKeys::Gamepad_FaceButton_Right, EKeys::Left, EKeys::Right, EKeys::Up, EKeys::Down,
		EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Down,
		EKeys::PageUp, EKeys::PageDown, EKeys::MouseScrollUp, EKeys::MouseScrollDown,
		EKeys::Gamepad_RightShoulder, EKeys::R, EKeys::N, EKeys::Gamepad_FaceButton_Top,
		EKeys::LeftShift, EKeys::Gamepad_LeftThumbstick})
	{
		FInputKeyBinding& Binding = InputComponent->BindKey(Key, IE_Pressed, this, &ANight_of_the_WolvesPlayerController::MenuKey);
		Binding.bExecuteWhenPaused = true;
		Binding.bConsumeInput = false;
	}
	for (FKey Key : {EKeys::LeftShift, EKeys::Gamepad_LeftThumbstick})
	{
		InputComponent->BindKey(Key, IE_Released, this, &ANight_of_the_WolvesPlayerController::SprintStop).bExecuteWhenPaused = true;
	}

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool ANight_of_the_WolvesPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ANight_of_the_WolvesPlayerController::MenuKey(FKey Key)
{
	if (!GetWorld()->GetAuthGameMode<AWolfTrailGameMode>()) return;
	bUsingPad = Key.IsGamepadKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right) { TogglePause(); return; }
	if (Key == EKeys::Tab || Key == EKeys::Gamepad_LeftShoulder) { ToggleWheel(); return; }
	if (Overlay == EOverlay::ConfirmNew)
	{
		if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			CloseOverlay();
			GetWorld()->GetAuthGameMode<AWolfTrailGameMode>()->NewTrail();
		}
		else if (Key == EKeys::Gamepad_FaceButton_Right) Overlay = EOverlay::Pause;
		return;
	}
	if (Overlay == EOverlay::Pause)
	{
		if (Key == EKeys::N || Key == EKeys::Gamepad_FaceButton_Left) { Overlay = EOverlay::ConfirmNew; return; }
		if (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Top) Retry();
		if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom) CloseOverlay();
		return;
	}
	if (Overlay == EOverlay::Inventory)
	{
		if (Key == EKeys::Enter || Key == EKeys::LeftMouseButton || Key == EKeys::Gamepad_FaceButton_Bottom) Confirm();
		else if (Key == EKeys::RightMouseButton || Key == EKeys::Gamepad_FaceButton_Right) Cancel();
		else if (Key == EKeys::PageUp) PageBack();
		else if (Key == EKeys::PageDown || Key == EKeys::Gamepad_RightShoulder) PageForward();
		else if (Key == EKeys::Left || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::MouseScrollUp) Previous();
		else if (Key == EKeys::Right || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::MouseScrollDown) Next();
		return;
	}
	if (Key == EKeys::E || Key == EKeys::Gamepad_FaceButton_Left) Interact();
	if (Key == EKeys::F || Key == EKeys::Gamepad_RightTrigger) UseItem();
	if (Key == EKeys::LeftShift || Key == EKeys::Gamepad_LeftThumbstick) SprintStart();
}

void ANight_of_the_WolvesPlayerController::OpenOverlay(EOverlay NewOverlay)
{
	auto* Mode = GetWorld()->GetAuthGameMode<AWolfTrailGameMode>();
	if (!Mode || (NewOverlay == EOverlay::Inventory && (Mode->IsDead() || Mode->IsTraversalLocked()))) return;
	if (!SetPause(true)) return;
	Overlay = NewOverlay;
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	SprintStop();
	if (NewOverlay == EOverlay::Inventory)
	{
		Selection = Mode->Inventory.Items.Find(Mode->Inventory.Equipped) + 1;
		int32 W, H;
		GetViewportSize(W, H);
		SetMouseLocation(W/2, H/2);
		LastMouse = FVector2D(W/2, H/2);
		bTrackCursor = false;
		bShowMouseCursor = !bUsingPad;
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
}

void ANight_of_the_WolvesPlayerController::CloseOverlay()
{
	if (Overlay == EOverlay::None) return;
	Overlay = EOverlay::None;
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	SetPause(false);
}

void ANight_of_the_WolvesPlayerController::ToggleWheel()
{
	if (Overlay == EOverlay::None) OpenOverlay(EOverlay::Inventory);
	else if (Overlay == EOverlay::Inventory) Cancel();
}

void ANight_of_the_WolvesPlayerController::TogglePause()
{
	if (Overlay == EOverlay::ConfirmNew) Overlay = EOverlay::Pause;
	else if (Overlay != EOverlay::None) CloseOverlay();
	else OpenOverlay(EOverlay::Pause);
}

void ANight_of_the_WolvesPlayerController::ShiftSelection(int32 Change)
{
	const auto& Items = GetWorld()->GetAuthGameMode<AWolfTrailGameMode>()->Inventory.Items;
	const int32 Count = Items.Num()+1;
	Selection = (Selection + Change + Count) % Count;
	bTrackCursor = false;
}

void ANight_of_the_WolvesPlayerController::Previous() { ShiftSelection(-1); }
void ANight_of_the_WolvesPlayerController::Next() { ShiftSelection(1); }

void ANight_of_the_WolvesPlayerController::ChangePage(int32 Direction)
{
	int32 Count = GetWorld()->GetAuthGameMode<AWolfTrailGameMode>()->Inventory.Items.Num()+1;
	int32 Pages = FMath::DivideAndRoundUp(Count, SlotsPerPage);
	Selection = ((WheelPage()+Direction+Pages)%Pages)*SlotsPerPage;
	bTrackCursor = false;
}
void ANight_of_the_WolvesPlayerController::PageBack() { ChangePage(-1); }
void ANight_of_the_WolvesPlayerController::PageForward() { ChangePage(1); }

void ANight_of_the_WolvesPlayerController::UpdatePointerSelection()
{
	if (Overlay != EOverlay::Inventory || bUsingPad) return;
	float MouseX, MouseY;
	if (!GetMousePosition(MouseX, MouseY)) return;
	const FVector2D Mouse(MouseX, MouseY);
	if (!Mouse.Equals(LastMouse, 1.)) bTrackCursor = true;
	LastMouse = Mouse;
	int32 W, H; GetViewportSize(W, H);
	const FVector2D Offset = Mouse - FVector2D(W*0.5, H*0.5);
	if (!bTrackCursor || Offset.Size() < H*.085) return;
	int32 Start = WheelPage()*SlotsPerPage;
	const int32 N = FMath::Min(SlotsPerPage, GetWorld()->GetAuthGameMode<AWolfTrailGameMode>()->Inventory.Items.Num()+1-Start);
	double Angle = FMath::Atan2(Offset.X, -Offset.Y);
	if (Angle < 0.) Angle += UE_TWO_PI;
	Selection = Start + (FMath::RoundToInt(Angle*N/UE_TWO_PI)%N);
}

void ANight_of_the_WolvesPlayerController::Confirm()
{
	UpdatePointerSelection();
	auto& Inv = GetWorld()->GetAuthGameMode<AWolfTrailGameMode>()->Inventory;
	const bool WasLit = Inv.bBurning;
	if (Selection == 0 || Inv.Items.IsValidIndex(Selection-1))
	{
		Inv.Equip(Selection == 0 ? NAME_None : Inv.Items[Selection-1]);
		if (WasLit && !Inv.bBurning)
		{
			Notice = TEXT("The ember goes dark.");
			NoticeUntil = GetWorld()->GetTimeSeconds()+2.f;
		}
	}
	CloseOverlay();
}

void ANight_of_the_WolvesPlayerController::Cancel() { CloseOverlay(); }
void ANight_of_the_WolvesPlayerController::Retry()
{
	CloseOverlay();
	GetWorld()->GetAuthGameMode<AWolfTrailGameMode>()->RetryCheckpoint();
}
void ANight_of_the_WolvesPlayerController::Interact() { GetWorld()->GetAuthGameMode<AWolfTrailGameMode>()->Interact(); }
void ANight_of_the_WolvesPlayerController::UseItem() { GetWorld()->GetAuthGameMode<AWolfTrailGameMode>()->UseEquipped(); }

void ANight_of_the_WolvesPlayerController::SprintStart()
{
	if (ACharacter* Character = GetCharacter())
	{
		auto* Movement = Character->GetCharacterMovement();
		if (NormalSpeed <= 0.f) NormalSpeed = Movement->MaxWalkSpeed;
		Movement->MaxWalkSpeed = NormalSpeed*1.35f;
	}
}

void ANight_of_the_WolvesPlayerController::SprintStop()
{
	if (NormalSpeed > 0.f && GetCharacter()) GetCharacter()->GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
	NormalSpeed = 0.f;
}
