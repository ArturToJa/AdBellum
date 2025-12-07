// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ALSPlayerCameraManager.generated.h"


// forward declarations
class UALSDebugComponent;
class AALSBaseCharacter;
class AALSCharacter;
/**
 * Player camera manager class
 */
UCLASS(Blueprintable, BlueprintType)
class ALSV4_CPP_API AALSPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AALSPlayerCameraManager();

	UFUNCTION(BlueprintCallable, Category = "ALS|Camera")
	void OnPossess(APawn* NewCharacter);

	UFUNCTION(BlueprintCallable, Category = "ALS|Camera")
	float GetCameraBehaviorParam(FName CurveName) const;

	/** Implemented debug logic in BP */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Camera")
	void DrawDebugTargets(FVector PivotTargetLocation);

protected:
	virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "ALS|Camera")
	static FVector CalculateAxisIndependentLag(
		FVector CurrentLocation, FVector TargetLocation, FRotator CameraRotation, FVector LagSpeeds, float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "ALS|Camera")
	bool CustomCameraBehavior(float DeltaTime, FVector& Location, FRotator& Rotation, float& FOV);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ALS|Camera")
	TObjectPtr<AALSBaseCharacter> ControlledCharacter = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ALS|Camera")
	TObjectPtr<AALSCharacter> castedNewCharacter = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ALS|Camera")
	TObjectPtr<USkeletalMeshComponent> CameraBehavior = nullptr;

	/** Updates post process effects based on player health */

	void UpdateDamageEffect(float HPRatio);

	UFUNCTION(BlueprintImplementableEvent)
	void ApplyValuesCameraModifier(float HPRatio);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Effects")
	float DesaturationAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Effects")
	float VignetteStrength;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Camera")
	FVector RootLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Camera")
	FTransform SmoothedPivotTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Camera")
	FVector PivotLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Camera")
	FVector TargetCameraLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ALS|Camera")
	FRotator TargetCameraRotation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Camera")
	FRotator DebugViewRotation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Camera")
	FVector DebugViewOffset;

private:
	UPROPERTY()
	TObjectPtr<UALSDebugComponent> ALSDebugComponent = nullptr;
};
