// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "Vehicle.generated.h"

UINTERFACE(MinimalAPI)
class UVehicle : public UInterface
{
	GENERATED_BODY()
};


class ADBELLUM_API IVehicle
{
	GENERATED_BODY()
public:

	/**
	* Return array of transforms for HandIK - 0 is left, 1 is right
	* @param Role StationaryRole
	* @return Return array of transforms for HandIK - 0 is left, 1 is right
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IVehicle")
	TArray<FTransform> GetHandsTransform(EALSStationaryRole Role);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IVehicle")
	float GetSteeringValue();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IVehicle")
	void SetTurretYaw(float value);

	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category = "IVehicle")
	float GetTurretYaw();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IVehicle")
	void SetTurretPitch(float value);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IVehicle")
	float GetTurretPitch();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IVehicle")
	void CalibrateADS(FVector TargetPoint);

};
