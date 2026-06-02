// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Character/ALSInputInterface.h"
#include "OrderSystem/OrderType.h"
#include "RTSPlayer.generated.h"

DECLARE_DELEGATE_OneParam(FOrderRightActionDelegate, bool)
DECLARE_DELEGATE_OneParam(FOrderActionDelegate, OrderEnum)

class ARTS_HUD;

UCLASS()
class ADBELLUM_API ARTSPlayer : public APawn, public IALSInputInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARTSPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	//ALSInputInterface
	virtual void ForwardMovementAction_Implementation(float Value) override;
	virtual void RightMovementAction_Implementation(float Value) override;
	virtual void CameraUpAction_Implementation(float Value) override;
	virtual void CameraRightAction_Implementation(float Value) override;
	virtual void CameraFreeRotateAction_Implementation(bool Value) override;
	virtual void TriggerAction_Implementation(bool Value) override;
	virtual void AimAction_Implementation(bool Value) override;
	virtual void InteractionAction_Implementation() override;
	virtual void OrderStopAction_Implementation(bool Value) override;
	virtual void OrderPatrolAction_Implementation(bool Value) override;
	virtual void OrderMoveAction_Implementation(bool Value) override;
	virtual void OrderHoldPositionAction_Implementation(bool Value) override;
	virtual void OrderAttackAction_Implementation(bool Value) override;
	virtual void ScrollAction_Implementation(bool bScrollUp) override;
	virtual void CameraMouseRotateAction_Implementation(bool Value) override;

	void RightMouseButtonPressed();
	void TeleportAboveUnit(AActor* TargetUnit);

	FOrderRightActionDelegate OrderActionDelegate;
	FOrderActionDelegate InteractionActionDelegate;

	TObjectPtr<ARTS_HUD> HUD;

private:
	FVector2D GetMouseDeltas(FVector2D MousePosition, FVector2D ViewportScaled);
	bool bFreeCameraRotationEnabled;
	bool bCameraMouseRotationEnabled;
	bool bDirectionalInputEnabled;

	static float BorderSize;
	static float CameraMoveSpeed;
	// movement multiplier used by WSAD/axis camera movement (replaces magic 150.0f)
	// Base speed; runtime calculated multiplier will be derived from this and camera height
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    float BaseSpeedMultiplier = 150.0f;

	// Calculated runtime speed multiplier (based on height)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float CalculatedSpeedMultiplier = 150.0f;

	// Reference height used to scale speed; calculated scale = Height / SpeedScaleReferenceHeight
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float SpeedScaleReferenceHeight = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float MinSpeedScale = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float MaxSpeedScale = 3.0f;

	// Min / Max camera height (world Z). Camera will be clamped to these limits.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float MinCameraHeight = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float MaxCameraHeight = 3000.0f;

	// Scroll step scale used when processing scroll actions
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float ScrollStep = 0.2f;

	UPROPERTY(EditDefaultsOnly ,meta = (AllowPrivateAccess = "true"))
	bool UseBorderCameraMovement;

    float CurrentHeightAboveLandscape = -1.0f;

    // Smooth scroll state
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    bool bEnableSmoothScroll = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    bool bIsScrolling = false;

    // Target location for smooth scrolling
    FVector ScrollTargetLocation;
	float MouseRotationX;
	float MouseRotationY;

	FVector MousePivotPoint;

    // Interp speed (units per second) used when smoothing
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    float ScrollInterpSpeed = 1200.0f;

    void CalculateHeightAboveLandscape();

	void CalculateSpeedMultiplier();

	void CheckCursorVisibility();

	static FVector2D ConvertToPlatformPixels(float MouseX, float MouseY);
};
