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
	virtual void CameraRotateAction_Implementation(bool Value) override;
	virtual void TriggerAction_Implementation(bool Value) override;
	virtual void AimAction_Implementation(bool Value) override;
	virtual void InteractionAction_Implementation() override;
	virtual void OrderStopAction_Implementation(bool Value) override;
	virtual void OrderPatrolAction_Implementation(bool Value) override;
	virtual void OrderMoveAction_Implementation(bool Value) override;
	virtual void OrderHoldPositionAction_Implementation(bool Value) override;
	virtual void OrderAttackAction_Implementation(bool Value) override;

	void RightMouseButtonPressed();
	void TeleportAboveUnit(AActor* TargetUnit);

	FOrderRightActionDelegate OrderActionDelegate;
	FOrderActionDelegate InteractionActionDelegate;

	TObjectPtr<ARTS_HUD> HUD;

private:
	FVector2D GetMouseDeltas(FVector2D MousePosition, FVector2D ViewportScaled);
	bool bCameraRotationEnabled;

	static float BorderSize;
	static float CameraMoveSpeed;
	UPROPERTY(EditDefaultsOnly ,meta = (AllowPrivateAccess = "true"))
	bool UseBorderCameraMovement;
};
