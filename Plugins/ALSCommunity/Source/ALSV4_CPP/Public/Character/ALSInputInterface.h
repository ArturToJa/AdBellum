// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AlsInputInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UALSInputInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class ALSV4_CPP_API IALSInputInterface
{
	GENERATED_BODY()

		// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void ForwardMovementAction(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void ForwardMovementActionCompleted();

	//simple movement orders when player is gunner/commander
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void ForwardMovementActionOrder(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void ForwardMovementActionOrderCompleted();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void RightMovementAction(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void RightMovementActionCompleted();

	//simple movement orders when player is gunner/commander
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void RightMovementActionOrder(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void RightMovementActionOrderCompleted();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void CameraUpAction(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void GunnerCameraUpAction(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void CameraRightAction(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void GunnerCameraRightAction(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void CameraActionCompleted();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void JumpAction(bool bValue);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void SprintAction(bool bValue);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void AimAction(bool bValue);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void AimActionCompleted();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void CameraTapAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void CameraHeldAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void StanceAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void WalkAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void RagdollAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void InteractionAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void CameraRotateAction(bool bValue);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void TriggerAction(bool bValue);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void GunnerTriggerAction(bool bValue);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void TriggerActionCompleted();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void ScrollAction(bool bValue);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void ReloadAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void CameraMouseRotateAction(bool bValue);

	//RECOIL
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void AddRecoil();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void StopRecoil();

	//WEAPON SELECT
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void PrimarySelectionAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void SecondarySelectionAction();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void ThirdSelectionAction();


	//ORDERS SECTION
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void OrderStopAction(bool Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void OrderPatrolAction(bool Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void OrderMoveAction(bool Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void OrderHoldPositionAction(bool Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void OrderAttackAction(bool Value);

		//will be used for reloading weapon after fire, performing slider action, reload RPG
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Input")
		void NotifyPlayMontage(UAnimMontage* Montage);

};
