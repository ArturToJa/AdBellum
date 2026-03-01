// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Weapon/IWeapon.h"
#include "Orderable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UOrderable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IOrderable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Orderable")
		void Stop(FVector TargetPosition);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Orderable")
		void MoveOrder(FVector TargetPosition);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Orderable")
		void AttackTarget(UObject* TargetObject);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Orderable")
		void AttackLocation(FVector TargetPosition);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Orderable")
		void DoCrouch();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Orderable")
		void DoCrawl();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Orderable")
		void DoStandUp();

};
