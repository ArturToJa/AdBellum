// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/RTSPlayer.h"
#include "Player/RTS_HUD.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/AdBellumPlayerController.h"

float ARTSPlayer::BorderSize = 10.0f;
float ARTSPlayer::CameraMoveSpeed = 160.0f;

ARTSPlayer::ARTSPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	bCameraRotationEnabled = false;
	SetReplicates(true);
	SetActorTickEnabled(false);
}

void ARTSPlayer::BeginPlay()
{
	Super::BeginPlay();
	if (AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(GetOwner()))
	{
		PC->PlayerCameraManager = Cast<AALSPlayerCameraManager>(UGameplayStatics::GetPlayerCameraManager(GetWorld(), PC->PlayerIndex));
	}
}

void ARTSPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (IsLocallyControlled() && UseBorderCameraMovement)
	{
		APlayerController* PlayerController = GetController<APlayerController>();

		float MouseX;
		float MouseY;
		PlayerController->GetMousePosition(MouseX, MouseY);

		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		float ViewportScale = GEngine->GameViewport->GetDPIScale();
		FVector2D ViewportScaled = ViewportSize / ViewportScale;

		FVector2D MouseDelta = GetMouseDeltas(FVector2D(MouseX, MouseY), ViewportScaled);

		FVector CameraForward = UKismetMathLibrary::GetForwardVector(GetControlRotation());
		FVector CameraRight = UKismetMathLibrary::GetRightVector(GetControlRotation());
		CameraForward.Z = 0;
		CameraRight.Z = 0;
		CameraForward.Normalize();
		CameraRight.Normalize();
		FVector MovementDirection = CameraForward * MouseDelta.X + CameraRight * MouseDelta.Y;
		FVector NormalizedDirection = MovementDirection;
		NormalizedDirection.Normalize();
		AddActorWorldOffset(MovementDirection);

		if (bCameraRotationEnabled)
		{
			int32 CenterX = ViewportScaled.X / 2;
			int32 CenterY = ViewportScaled.Y / 2;
			PlayerController->SetMouseLocation(CenterX, CenterY);
		}
	}
}

void ARTSPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ARTSPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	SetActorTickEnabled(true);
}

void ARTSPlayer::UnPossessed()
{
	Super::UnPossessed();
	bCameraRotationEnabled = false;
	SetActorTickEnabled(false);
}

void ARTSPlayer::ForwardMovementAction_Implementation(float Value)
{
	if (IsLocallyControlled())
	{
		FVector RotationForwardVector = UKismetMathLibrary::GetForwardVector(GetControlRotation());
		AddMovementInput(RotationForwardVector, Value * 150.0f);
	}
}

void ARTSPlayer::RightMovementAction_Implementation(float Value)
{
	if (IsLocallyControlled())
	{
		FVector RotationRightVector = UKismetMathLibrary::GetRightVector(GetControlRotation());
		AddMovementInput(RotationRightVector, Value * 150.0f);
	}
}

void ARTSPlayer::CameraUpAction_Implementation(float Value)
{
	if (IsLocallyControlled())
	{
		if (bCameraRotationEnabled)
		{
			AddControllerPitchInput(Value * 3.0f);
		}
	}
}

void ARTSPlayer::CameraRightAction_Implementation(float Value)
{
	if (IsLocallyControlled())
	{
		if (bCameraRotationEnabled)
		{
			AddControllerYawInput(Value * 3.0f);
		}
	}
}

void ARTSPlayer::CameraRotateAction_Implementation(bool Value)
{
	if (IsLocallyControlled())
	{
		bCameraRotationEnabled = Value;
		APlayerController* PlayerController = GetController<APlayerController>();
		PlayerController->SetShowMouseCursor(!Value);
	}
}

void ARTSPlayer::TriggerAction_Implementation(bool Value)
{
	if (Value)
	{
		if (HUD)
		{
			HUD->SelectionModeStart();
		}
	}
	else
	{
		if (HUD)
		{
			HUD->SelectionModeEnd();
		}
	}
}

void ARTSPlayer::AimAction_Implementation(bool Value)
{
	OrderActionDelegate.ExecuteIfBound(Value);
}

void ARTSPlayer::InteractionAction_Implementation()
{
	InteractionActionDelegate.ExecuteIfBound(OrderEnum::Interact);
}

void ARTSPlayer::OrderStopAction_Implementation(bool Value)
{
	InteractionActionDelegate.ExecuteIfBound(OrderEnum::Stop);
}

void ARTSPlayer::OrderPatrolAction_Implementation(bool Value)
{
	InteractionActionDelegate.ExecuteIfBound(OrderEnum::Patrol);
}

void ARTSPlayer::OrderMoveAction_Implementation(bool Value)
{
	InteractionActionDelegate.ExecuteIfBound(OrderEnum::Move);
}

void ARTSPlayer::OrderHoldPositionAction_Implementation(bool Value)
{
	InteractionActionDelegate.ExecuteIfBound(OrderEnum::HoldPosition);
}

void ARTSPlayer::OrderAttackAction_Implementation(bool Value)
{
	InteractionActionDelegate.ExecuteIfBound(OrderEnum::Attack);
}

FVector2D ARTSPlayer::GetMouseDeltas(FVector2D MousePosition, FVector2D ViewportScaled)
{
	FVector2D Delta(0.0f, 0.0f);

	if (MousePosition.X <= BorderSize)
	{
		Delta.Y = CameraMoveSpeed * -1.0f;
	}
	else if (MousePosition.X >= ViewportScaled.X - BorderSize)
	{
		Delta.Y = CameraMoveSpeed;
	}

	if (MousePosition.Y <= BorderSize)
	{
		Delta.X = CameraMoveSpeed;
	}
	else if (MousePosition.Y >= ViewportScaled.Y - BorderSize)
	{
		Delta.X = CameraMoveSpeed * -1.0f;
	}

	return Delta;
}

void ARTSPlayer::RightMouseButtonPressed()
{

}

void ARTSPlayer::TeleportAboveUnit(AActor* TargetUnit)
{
	if (!TargetUnit) return;
	FVector UnitLocation = TargetUnit->GetActorLocation();
	FVector CameraLocation = GetActorLocation();
	float DesiredAngle = 45.0f; // Desired angle (in degrees)
	float HeightOffset = CameraLocation.Z - UnitLocation.Z;
	float DistanceFromUnit = HeightOffset / FMath::Tan(FMath::DegreesToRadians(DesiredAngle));

	FVector Direction = FVector(UnitLocation.X - CameraLocation.X, UnitLocation.Y - CameraLocation.Y, 0.0f);
	Direction.Normalize();

	FVector NewLocation = UnitLocation - DistanceFromUnit * Direction;
	NewLocation.Z = CameraLocation.Z;
	SetActorLocation(NewLocation);

	FRotator LookAtRotation = (UnitLocation - NewLocation).Rotation();
	FRotator NewRotation;
	NewRotation.Pitch = -DesiredAngle;
	NewRotation.Yaw = LookAtRotation.Yaw;
	NewRotation.Roll = 0.0f;
	SetActorRotation(NewRotation);
}