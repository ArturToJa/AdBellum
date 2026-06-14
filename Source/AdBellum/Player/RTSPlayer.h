// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Character/ALSInputInterface.h"
#include "OrderSystem/OrderType.h"

#include <GameFramework/FloatingPawnMovement.h>

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFloatingPawnMovement* FloatingMovement;

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

    // Playable area defined by a box component in the level (assign in BP or editor)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|PlayableArea", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UBoxComponent> PlayableAreaBox;

    // How much to expand bottom corners outward (120% = 1.2)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|PlayableArea", meta = (AllowPrivateAccess = "true"))
    float PlayableCornerExpandPercent = 1.2f;

    // Cached expanded bottom corners (world space)
    TArray<FVector> ExpandedPlayableCorners;

    // Recompute corners and max camera height from the box component
    void InitializePlayableAreaFromBox();

    // Last known transform of the playable area box to detect changes
    FTransform LastPlayableBoxTransform;
    // Playable area reference plane Z (bottom of box)
    float PlayableAreaZ = 0.0f;

    // Returns true if camera at CandidateLocation would see outside the expanded playable corners
    bool IsPositionInsideArea(const FVector2D& CandidateLocation) const;

	FVector2D BoxCornerMinMin;
	FVector2D BoxCornerMaxMax;

    // Interp speed (units per second) used when smoothing
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    float ScrollInterpSpeed = 1200.0f;

    void CalculateHeightAboveLandscape();

	void CalculateSpeedMultiplier();

	void CheckCursorVisibility();

	static FVector2D ConvertToPlatformPixels(float MouseX, float MouseY);

	FVector PredictPawnStopLocation_Linear(float TimeHorizon, FVector ControlAcceleration) const
	{
		if (!FloatingMovement) return GetActorLocation();

		FVector Velocity = FloatingMovement->Velocity;
		/*if (Velocity.Length() <= 0.0f) 
		{
			const float NewMaxSpeed = FloatingMovement->GetMaxSpeed();
			Velocity += ControlAcceleration * FMath::Abs(FloatingMovement->Acceleration) * TimeHorizon;
			Velocity = Velocity.GetClampedToMaxSize(NewMaxSpeed);
		}*/

		// simple forward projection
		return GetActorLocation() + Velocity * TimeHorizon;
	}

	FVector PredictPawnStopLocation_FrictionApprox(
		float DeltaTimeStep,
		float MaxTime) const
	{
		if (!FloatingMovement) return GetActorLocation();

		FVector Location = GetActorLocation();
		FVector Velocity = FloatingMovement->Velocity;

		const float Deceleration = FloatingMovement->Deceleration;

		float Time = 0.f;

		while (Time < MaxTime && Velocity.SizeSquared() > 1.f)
		{
			Location += Velocity * DeltaTimeStep;

			// approximate UE damping behavior
			float Decay = FMath::Clamp(1.f - Deceleration * DeltaTimeStep, 0.f, 1.f);
			Velocity *= Decay;

			Time += DeltaTimeStep;
		}

		return Location;
	}

	FVector PredictPawnStopLocation_Simulated(
		float DeltaTimeStep,
		float MaxTime) const
	{
		if (!FloatingMovement) return GetActorLocation();

		FVector Location = GetActorLocation();
		FVector Velocity = FloatingMovement->Velocity;

		const float MaxSpeed = FloatingMovement->MaxSpeed;
		const float Accel = FloatingMovement->Acceleration;
		const float Deceleration = FloatingMovement->Deceleration;

		float Time = 0.f;

		while (Time < MaxTime)
		{
			if (Velocity.SizeSquared() < 1.f)
				break;

			// simulate acceleration = none (no input)
			// apply friction braking
			float DecelerationFactor = FMath::Clamp(1.f - Deceleration * DeltaTimeStep, 0.f, 1.f);
			Velocity *= DecelerationFactor;

			// clamp max speed (UE behavior)
			Velocity = Velocity.GetClampedToMaxSize(MaxSpeed);

			Location += Velocity * DeltaTimeStep;
			Time += DeltaTimeStep;
		}

		return Location;
	}
};
