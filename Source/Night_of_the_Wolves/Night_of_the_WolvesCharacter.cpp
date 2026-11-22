// Copyright Epic Games, Inc. All Rights Reserved.

#include "Night_of_the_WolvesCharacter.h"
#include "WolfHeldItem.h"
#include "Animation/AnimInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GroomBlueprintLibrary.h"
#include "GroomComponent.h"
#include "HairStrandsInterface.h"
#include "Night_of_the_Wolves.h"

ANight_of_the_WolvesCharacter::ANight_of_the_WolvesCharacter()
{
	Face = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Max Face"));
	Face->SetupAttachment(GetMesh());
	Outfit = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Max Outfit"));
	Outfit->SetupAttachment(GetMesh());
	Hair = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Max Hair"));
	Hair->SetupAttachment(Face);
	Fringe = CreateDefaultSubobject<UGroomComponent>(TEXT("Max Fringe"));
	Fringe->SetupAttachment(Face);
	Brows = CreateDefaultSubobject<UGroomComponent>(TEXT("Max Brows"));
	Brows->SetupAttachment(Face);
	UPrimitiveComponent* Parts[] = {Face, Outfit, Hair, Fringe, Brows};
	for (UPrimitiveComponent* Part : Parts)
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
	}

	auto* HeldItem = CreateDefaultSubobject<UWolfHeldItem>(TEXT("Held item"));
	HeldItem->SetupAttachment(GetMesh(), TEXT("hand_r"));

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ANight_of_the_WolvesCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!MaxBody) return;

	USkeletalMeshComponent* Body = GetMesh();
	if (Body->GetSkeletalMeshAsset() != MaxBody)
	{
		Body->EmptyOverrideMaterials();
		Body->SetSkeletalMesh(MaxBody);
	}
	Body->SetAnimInstanceClass(MaxLocomotion);
	Face->SetSkeletalMesh(MaxHead);
	Face->SetAnimInstanceClass(MaxFaceAnimation);
	Face->AddTickPrerequisiteComponent(Body);
	Outfit->SetSkeletalMesh(MaxOutfit);
	Outfit->SetLeaderPoseComponent(Body);
	Hair->AttachToComponent(Face, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("head"));
	Hair->SetStaticMesh(MaxHair);

	const bool bStrands = FApp::CanEverRender() && UGroomBlueprintLibrary::IsHairStrandsSupportedInWorld(this)
		&& IsHairStrandsEnabled(EHairStrandsShaderType::Strands);
	auto Bind = [bStrands](UGroomComponent* Component, UGroomBindingAsset* Binding)
	{
		UGroomBindingAsset* Active = bStrands ? Binding : nullptr;
		Component->SetGroomAsset(Active ? Active->GetGroom() : nullptr, Active);
	};
	Bind(Fringe, MaxFringeBinding);
	Bind(Brows, MaxBrowBinding);
	// A newly imported groom can still be compiling on first open in the editor.
	// Select one representation even while the strand resources are being built.
	Hair->SetVisibility(!(bStrands && MaxFringeBinding && MaxFringeBinding->GetGroom()));
}

void ANight_of_the_WolvesCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (MaxBody)
	{
		UE_LOG(LogNight_of_the_Wolves, Display,
			TEXT("Max appearance: body=%s locomotion=%s head=%s faceAnimation=%s outfit=%s hair=%s brows=%s"),
			*GetNameSafe(GetMesh()->GetSkeletalMeshAsset()), *GetNameSafe(GetMesh()->GetAnimInstance()),
			*GetNameSafe(Face->GetSkeletalMeshAsset()), *GetNameSafe(Face->GetAnimInstance()),
			*GetNameSafe(Outfit->GetSkeletalMeshAsset()),
			Fringe->GroomAsset ? *Fringe->GroomAsset->GetName() : *GetNameSafe(Hair->GetStaticMesh()),
			*GetNameSafe(Brows->GroomAsset));
	}
}

void ANight_of_the_WolvesCharacter::SetHangingAppearance(USkinnedMeshComponent* HangingBody)
{
	if (!MaxBody) return;

	// The creek pose only changes the arms. Hold the face at the captured neck
	// pose and skin the sleeves against the same bones that grip the rope.
	Face->bPauseAnims = HangingBody != nullptr;
	Outfit->SetLeaderPoseComponent(HangingBody ? HangingBody : GetMesh());
}

void ANight_of_the_WolvesCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANight_of_the_WolvesCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ANight_of_the_WolvesCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANight_of_the_WolvesCharacter::Look);
	}
	else
	{
		UE_LOG(LogNight_of_the_Wolves, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ANight_of_the_WolvesCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ANight_of_the_WolvesCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ANight_of_the_WolvesCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ANight_of_the_WolvesCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ANight_of_the_WolvesCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ANight_of_the_WolvesCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}
