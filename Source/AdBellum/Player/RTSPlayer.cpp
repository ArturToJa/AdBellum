// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSPlayer.h"
#include "RTS_HUD.h"
#include "Kismet/KismetMathLibrary.h"
#include "AdBellumPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

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

	CalculateHeightAboveLandscape();
	CalculateSpeedMultiplier();
}

void ARTSPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    // If smooth scrolling is active, interpolate toward target
    if (bIsScrolling && bEnableSmoothScroll)
    {
        FVector CurrentLocation = GetActorLocation();
        FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, ScrollTargetLocation, DeltaTime, ScrollInterpSpeed * CalculatedSpeedMultiplier);

        float HeightDifference = NewLocation.Z - CurrentLocation.Z;
        CurrentHeightAboveLandscape += HeightDifference;
		CalculateSpeedMultiplier();
        // clamp height
        NewLocation.Z = FMath::Clamp(NewLocation.Z, MinCameraHeight, MaxCameraHeight);

        // If we're very close to the target, snap and finish
        const float StopDistSq = FMath::Square(4.0f);
        if (FVector::DistSquared(NewLocation, ScrollTargetLocation) <= StopDistSq)
        {
            SetActorLocation(ScrollTargetLocation);
            // ensure height and multiplier are correct at final location
            CalculateHeightAboveLandscape();
            CalculateSpeedMultiplier();
            bIsScrolling = false;
        }
        else
        {
            SetActorLocation(NewLocation);
        }


        // stop scrolling if reached height limits
        if (NewLocation.Z <= MinCameraHeight || NewLocation.Z >= MaxCameraHeight)
        {
            bIsScrolling = false;
        }
    }

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

		// MovementDirection uses calculated multiplier and delta time to be framerate independent
		FVector MovementDirection = CameraForward * MouseDelta.X + CameraRight * MouseDelta.Y;
		if (!MovementDirection.IsNearlyZero())
		{
			MovementDirection = MovementDirection.GetSafeNormal() * CalculatedSpeedMultiplier * CalculatedSpeedMultiplier * DeltaTime * MovementDirection.Size();
			AddActorWorldOffset(MovementDirection, true);
		}

		if (bCameraRotationEnabled || bCameraMouseRotationEnabled)
		{
			PlayerController->SetMouseLocation(MouseRotationX, MouseRotationY);
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
		// Move actor only on the XY plane; keep Z unchanged
		FVector Forward = UKismetMathLibrary::GetForwardVector(GetControlRotation());
		Forward.Z = 0.0f;
		Forward.Normalize();

		// Use CalculatedSpeedMultiplier instead of magic constant, include DeltaTime by using AddMovementInput
		AddMovementInput(Forward, Value * BaseSpeedMultiplier * CalculatedSpeedMultiplier);
	}
}

void ARTSPlayer::RightMovementAction_Implementation(float Value)
{
	if (IsLocallyControlled())
	{
		// Move actor only on the XY plane; keep Z unchanged
		FVector Right = UKismetMathLibrary::GetRightVector(GetControlRotation());
		Right.Z = 0.0f;
		Right.Normalize();

		AddMovementInput(Right, Value * BaseSpeedMultiplier * CalculatedSpeedMultiplier);
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
		if (bCameraMouseRotationEnabled)
		{
			FVector Pivot = MousePivotPoint;

			FVector CamLoc = GetActorLocation();
			FRotator CamRot = GetControlRotation();

			FVector Offset = CamLoc - Pivot;

			float YawDelta = Value * 3.0f;

			FRotator OrbitRot(0.f, YawDelta, 0.f);

			FVector RotatedOffset =
				OrbitRot.RotateVector(Offset);

			FVector NewLoc =
				Pivot + RotatedOffset;

			SetActorLocation(NewLoc);

			FQuat DeltaQuat =
				OrbitRot.Quaternion();

			FQuat DesiredQuat =
				DeltaQuat * CamRot.Quaternion();

			FRotator DesiredRot =
				DesiredQuat.Rotator();

			FRotator DeltaRot =
				(DesiredRot - CamRot).GetNormalized();

			GetController()->SetControlRotation(DesiredRot);
		}
	}
}

void ARTSPlayer::CameraRotateAction_Implementation(bool Value)
{
	if (IsLocallyControlled())
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		float ViewportScale = GEngine->GameViewport->GetDPIScale();
		FVector2D ViewportScaled = ViewportSize / ViewportScale;
		MouseRotationX = ViewportScaled.X / 2;
		MouseRotationY = ViewportScaled.Y / 2;
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

void ARTSPlayer::CalculateHeightAboveLandscape()
{
	// Trace directly downwards from camera to find landscape height below
	FVector Start = GetActorLocation();

    FVector End = Start - FVector(0.0f, 0.0f, MaxCameraHeight);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        CurrentHeightAboveLandscape = Start.Z - Hit.ImpactPoint.Z;
    }
    else
    {
        // no hit
        CurrentHeightAboveLandscape = FLT_MAX;
    }
}

void ARTSPlayer::ScrollAction_Implementation(bool bScrollUp)
{
	// Scroll moves camera toward point under mouse on landscape
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("ScrollAction: %s"), bScrollUp ? TEXT("Up") : TEXT("Down")));

	APlayerController* PC = GetController<APlayerController>();
	if (!PC) return;

    // Prevent scrolling beyond world Z limits
    float CameraZ = GetActorLocation().Z;
    if (CameraZ >= MaxCameraHeight && !bScrollUp) return;
    if (CameraZ <= MinCameraHeight && bScrollUp) return;

	float MouseX, MouseY;
	PC->GetMousePosition(MouseX, MouseY);

	FVector WorldOrigin, WorldDir;
	PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldOrigin, WorldDir);

	FVector TraceStart = WorldOrigin;
	FVector TraceEnd = WorldOrigin + WorldDir * 100000.0f;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		return;
	}

	FVector TargetPoint = Hit.ImpactPoint;
	FVector CameraLocation = GetActorLocation();

	// compute direction on XY plane toward target point
    // compute full 3D direction toward target point (allow vertical movement)
    FVector Dir = TargetPoint - CameraLocation;
    float Dist = Dir.Size();
    if (Dist < KINDA_SMALL_NUMBER) return;
    Dir.Normalize();

	// choose direction sign based on scroll
	float ScrollSign = bScrollUp ? 1.0f : -1.0f;

	FVector DesiredLocation = CameraLocation + Dir * ScrollStep * ScrollSign * CalculatedSpeedMultiplier;

    // clamp new height between min/max
    DesiredLocation.Z = FMath::Clamp(DesiredLocation.Z, MinCameraHeight, MaxCameraHeight);

    // If smooth scrolling enabled, set scroll target and let Tick() interpolate
    if (bEnableSmoothScroll)
    {
        ScrollTargetLocation = DesiredLocation;
        bIsScrolling = true;
    }
    else
    {
        SetActorLocation(DesiredLocation);
        // Recalculate current height above landscape and speed multiplier
        CalculateHeightAboveLandscape();
        CalculateSpeedMultiplier();
    }
}

void ARTSPlayer::CameraMouseRotateAction_Implementation(bool bScrollUp)
{
	if (bScrollUp)
	{
		bCameraMouseRotationEnabled = true;
		APlayerController* PlayerController = GetController<APlayerController>();
		PlayerController->SetShowMouseCursor(false);
		if (PlayerController->GetMousePosition(MouseRotationX, MouseRotationY))
		{
			FVector MousePositionInWorld;
			FVector MouseDirectionInWorld;

			PlayerController->DeprojectScreenPositionToWorld(MouseRotationX, MouseRotationY, MousePositionInWorld, MouseDirectionInWorld);

			FVector2D ScaledMouseRotation = ConvertToPlatformPixels(MouseRotationX, MouseRotationY);
			MouseRotationX = ScaledMouseRotation.X;
			MouseRotationY = ScaledMouseRotation.Y;

			FHitResult Hit;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);
			FVector TraceEnd = MousePositionInWorld + MouseDirectionInWorld * 100000.0f;
			if (GetWorld()->LineTraceSingleByChannel(Hit, MousePositionInWorld, TraceEnd, ECC_Visibility, Params))
			{
				MousePivotPoint = Hit.ImpactPoint;
			}
		}
	}
	else
	{
		bCameraMouseRotationEnabled = false;
		APlayerController* PlayerController = GetController<APlayerController>();
		PlayerController->SetShowMouseCursor(true);
	}
}

void ARTSPlayer::CalculateSpeedMultiplier() {
	float SpeedLimitDifference = MaxSpeedScale - MinSpeedScale;
	float HeightLimitDifference = MaxCameraHeight - MinCameraHeight;
	float NewScale = MinSpeedScale;
	if (HeightLimitDifference > KINDA_SMALL_NUMBER)
	{
		NewScale = MinSpeedScale + ((CurrentHeightAboveLandscape - MinCameraHeight) / HeightLimitDifference) * SpeedLimitDifference;
	}
	NewScale = FMath::Clamp(NewScale, MinSpeedScale, MaxSpeedScale);
	CalculatedSpeedMultiplier = NewScale;
}

FVector2D ARTSPlayer::ConvertToPlatformPixels(float MouseX, float MouseY)
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return FVector2D(MouseX, MouseY);
	}
	float DPIScale = GEngine->GameViewport->GetDPIScale();
	// UI coords * DPI -> platform (native) pixels
	return FVector2D(MouseX * DPIScale, MouseY * DPIScale);
}