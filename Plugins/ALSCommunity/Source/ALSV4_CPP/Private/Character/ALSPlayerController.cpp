// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community


#include "Character/ALSPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "AI/ALSAIController.h"
#include "Character/ALSCharacter.h"
#include "Character/ALSPlayerCameraManager.h"
#include "Components/ALSDebugComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

void AALSPlayerController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);
	PossessedCharacter = NewPawn;
	if (!IsRunningDedicatedServer())
	{
		// Servers want to setup camera only in listen servers.
		SetupCamera();
	}

	SetupInputs();
}

void AALSPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	PossessedCharacter = GetPawn();
	SetupCamera();
	SetupInputs();
}

void AALSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->ClearActionEventBindings();
		EnhancedInputComponent->ClearActionValueBindings();
		EnhancedInputComponent->ClearDebugKeyBindings();

		BindActions(DefaultInputMappingContext);
		BindActions(DebugInputMappingContext);
	}
	else
	{
		UE_LOG(LogTemp, Fatal, TEXT("ALS Community requires Enhanced Input System to be activated in project settings to function properly"));
	}
}

void AALSPlayerController::BindActions(UInputMappingContext* Context)
{
	if (Context)
	{
		const TArray<FEnhancedActionKeyMapping>& Mappings = Context->GetMappings();
		UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
		if (EnhancedInputComponent)
		{
			// There may be more than one keymapping assigned to one action. So, first filter duplicate action entries to prevent multiple delegate bindings
			TSet<const UInputAction*> UniqueActions;
			for (const FEnhancedActionKeyMapping& Keymapping : Mappings)
			{
				UniqueActions.Add(Keymapping.Action);
			}
			for (const UInputAction* UniqueAction : UniqueActions)
			{
				EnhancedInputComponent->BindAction(UniqueAction, ETriggerEvent::Triggered, Cast<UObject>(this), UniqueAction->GetFName());

				if (UniqueAction->GetFName().ToString().Contains("ForwardMovementAction")) {
					EnhancedInputComponent->BindAction(UniqueAction, ETriggerEvent::Completed, Cast<UObject>(this), "HandleForwardMovementActionCompleted");
					EnhancedInputComponent->BindAction(UniqueAction, ETriggerEvent::Canceled, Cast<UObject>(this), "HandleForwardMovementActionCompleted");
				}
				else if (UniqueAction->GetFName().ToString().Contains("RightMovementAction")) {
					EnhancedInputComponent->BindAction(UniqueAction, ETriggerEvent::Completed, Cast<UObject>(this), "HandleRightMovementActionCompleted");
					EnhancedInputComponent->BindAction(UniqueAction, ETriggerEvent::Canceled, Cast<UObject>(this), "HandleRightMovementActionCompleted");
				}
				else if (UniqueAction->GetFName().ToString().Contains("CameraUpAction")) {
					EnhancedInputComponent->BindAction(UniqueAction, ETriggerEvent::Completed, Cast<UObject>(this), "HandleCameraActionCompleted");
				}
				else if (UniqueAction->GetFName().ToString().Contains("CameraRightAction")) {
					EnhancedInputComponent->BindAction(UniqueAction, ETriggerEvent::Completed, Cast<UObject>(this), "HandleCameraActionCompleted");
				}
				else if (UniqueAction->GetFName().ToString().Contains("AimAction")) {
					EnhancedInputComponent->BindAction(UniqueAction, ETriggerEvent::Completed, Cast<UObject>(this), "HandleAimActionCompleted");
				}
				else if (UniqueAction->GetFName().ToString().Contains("TriggerAction")) {
					EnhancedInputComponent->BindAction(UniqueAction, ETriggerEvent::Completed, Cast<UObject>(this), "HandleTriggerActionCompleted");
				}
			}
		}
	}
}

void AALSPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AALSPlayerController, PossessedCharacter);
}

void AALSPlayerController::SetupInputs()
{
	if (PossessedCharacter)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			FModifyContextOptions Options;
			Options.bForceImmediately = 1;
			Subsystem->AddMappingContext(DefaultInputMappingContext, 1, Options);
			UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
			if (DebugComp)
			{
				// Do only if we have debug component
				Subsystem->AddMappingContext(DebugInputMappingContext, 0, Options);
			}
		}
	}
}

void AALSPlayerController::SetupCamera()
{
	// Call "OnPossess" in Player Camera Manager when possessing a pawn
	AALSPlayerCameraManager* CastedMgr = Cast<AALSPlayerCameraManager>(PlayerCameraManager);
	if (PossessedCharacter && CastedMgr)
	{
		CastedMgr->OnPossess(PossessedCharacter);
	}
}

void AALSPlayerController::ForwardMovementAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_ForwardMovementAction(PossessedCharacter, Value.GetMagnitude());
		//PossessedCharacter->ForwardMovementAction(Value.GetMagnitude());
	}
}

void AALSPlayerController::RightMovementAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_RightMovementAction(PossessedCharacter, Value.GetMagnitude());
		//PossessedCharacter->RightMovementAction(Value.GetMagnitude());
	}
}

void AALSPlayerController::CameraUpAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter)
	{
		//float LookAxis = Value.GetMagnitude();

		//UE_LOG(
		//	LogTemp,
		//	Warning,
		//	TEXT("LOOK INPUT: Camera UP Action=%f "),
		//	LookAxis
		//);
		
		IALSInputInterface::Execute_CameraUpAction(PossessedCharacter, Value.GetMagnitude());
		//PossessedCharacter->CameraUpAction(Value.GetMagnitude());
	}
}

void AALSPlayerController::CameraRightAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter)
	{
		//float LookAxis = Value.GetMagnitude();

		//UE_LOG(
		//	LogTemp,
		//	Warning,
		//	TEXT("LOOK INPUT: Camera RIGHT Action=%f "),
		//	LookAxis
		//);

		IALSInputInterface::Execute_CameraRightAction(PossessedCharacter, Value.GetMagnitude());
		//PossessedCharacter->CameraRightAction(Value.GetMagnitude());
	}
}

void AALSPlayerController::JumpAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_JumpAction(PossessedCharacter, Value.Get<bool>());
		//PossessedCharacter->JumpAction(Value.Get<bool>());
	}
}

void AALSPlayerController::SprintAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_SprintAction(PossessedCharacter, Value.Get<bool>());
		//PossessedCharacter->SprintAction(Value.Get<bool>());
	}
}

void AALSPlayerController::AimAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_AimAction(PossessedCharacter, Value.Get<bool>());
		//PossessedCharacter->AimAction(Value.Get<bool>());
	}
}

void AALSPlayerController::CameraTapAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_CameraTapAction(PossessedCharacter);
		//PossessedCharacter->CameraTapAction();
	}
}

void AALSPlayerController::CameraHeldAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_CameraHeldAction(PossessedCharacter);
		//PossessedCharacter->CameraHeldAction();
	}
}

void AALSPlayerController::StanceAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter && Value.Get<bool>())
	{
		IALSInputInterface::Execute_StanceAction(PossessedCharacter);
		//PossessedCharacter->StanceAction();
	}
}

void AALSPlayerController::WalkAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter && Value.Get<bool>())
	{
		IALSInputInterface::Execute_WalkAction(PossessedCharacter);
		//PossessedCharacter->WalkAction();
	}
}

void AALSPlayerController::RagdollAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter && Value.Get<bool>())
	{
		IALSInputInterface::Execute_RagdollAction(PossessedCharacter);
		//PossessedCharacter->RagdollAction();
	}
}

void AALSPlayerController::InteractionAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter && Value.Get<bool>())
	{
		IALSInputInterface::Execute_InteractionAction(PossessedCharacter);
		//PossessedCharacter->LookingDirectionAction();
	}
}

void AALSPlayerController::ReloadAction(const FInputActionValue& Value) 
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter && Value.Get<bool>())
	{
		IALSInputInterface::Execute_ReloadAction(PossessedCharacter);
		//PossessedCharacter->LookingDirectionAction();
	}
}

void AALSPlayerController::CameraMouseRotateAction(const FInputActionValue& Value)
{
	if (CurrentState != EALSState::None)
	{
		return;
	}
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_CameraMouseRotateAction(PossessedCharacter, Value.Get<bool>());
		//PossessedCharacter->LookingDirectionAction();
	}
}

void AALSPlayerController::DebugToggleHudAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
		if (DebugComp)
		{
			DebugComp->ToggleHud();
		}
	}
}

void AALSPlayerController::DebugToggleDebugViewAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
		if (DebugComp)
		{
			DebugComp->ToggleDebugView();
		}
	}
}

void AALSPlayerController::DebugToggleTracesAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
		if (DebugComp)
		{
			DebugComp->ToggleTraces();
		}
	}
}

void AALSPlayerController::DebugToggleShapesAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
		if (DebugComp)
		{
			DebugComp->ToggleDebugShapes();
		}
	}
}

void AALSPlayerController::DebugToggleLayerColorsAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
		if (DebugComp)
		{
			DebugComp->ToggleLayerColors();
		}
	}
}

void AALSPlayerController::DebugToggleCharacterInfoAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
		if (DebugComp)
		{
			DebugComp->ToggleCharacterInfo();
		}
	}
}

void AALSPlayerController::DebugToggleSlomoAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
		if (DebugComp)
		{
			DebugComp->ToggleSlomo();
		}
	}
}

void AALSPlayerController::DebugFocusedCharacterCycleAction(const FInputActionValue& Value)
{
	if (PossessedCharacter)
	{
		UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
		if (DebugComp)
		{
			DebugComp->FocusedDebugCharacterCycle(Value.GetMagnitude() > 0);
		}
	}
}

void AALSPlayerController::DebugToggleMeshAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
		if (DebugComp)
		{
			DebugComp->ToggleDebugMesh();
		}
	}
}

void AALSPlayerController::DebugOpenOverlayMenuAction(const FInputActionValue& Value)
{
	if (PossessedCharacter)
	{
		UALSDebugComponent* DebugComp = Cast<UALSDebugComponent>(PossessedCharacter->GetComponentByClass(UALSDebugComponent::StaticClass()));
		if (DebugComp)
		{
			DebugComp->OpenOverlayMenu(Value.Get<bool>());
		}
	}
}

void AALSPlayerController::DebugOverlayMenuCycleAction(const FInputActionValue& Value)
{
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_ScrollAction(PossessedCharacter, Value.GetMagnitude() > 0);
	}
}

void AALSPlayerController::HandleForwardMovementActionCompleted(const FInputActionValue& Value) {
	if (PossessedCharacter) {
		IALSInputInterface::Execute_ForwardMovementActionCompleted(PossessedCharacter);
	}
}

void AALSPlayerController::HandleRightMovementActionCompleted(const FInputActionValue& Value) {
	if (PossessedCharacter) {
		IALSInputInterface::Execute_RightMovementActionCompleted(PossessedCharacter);
	}
}

void AALSPlayerController::HandleCameraActionCompleted(const FInputActionValue& Value) {
	if (PossessedCharacter) {
		IALSInputInterface::Execute_CameraActionCompleted(PossessedCharacter);
	}
}

void AALSPlayerController::HandleAimActionCompleted(const FInputActionValue& Value) {
	if (PossessedCharacter) {
		IALSInputInterface::Execute_AimActionCompleted(PossessedCharacter);
	}
}

void AALSPlayerController::HandleTriggerActionCompleted(const FInputActionValue& Value) {
	if (PossessedCharacter) {
		IALSInputInterface::Execute_TriggerActionCompleted(PossessedCharacter);
	}
}