#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "DamageEffectCameraModifier.generated.h"

/**
 * Modifier that applies visual damage effects (blur, chromatic aberration) based on values set externally.
 */
UCLASS()
class ADBELLUM_API UDamageEffectCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	UDamageEffectCameraModifier();

	virtual void AddedToCamera(APlayerCameraManager* Camera) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Effects")
	UMaterialInterface* BlurEffectMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* BlurMaterialDynamicInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Effects")
	UMaterialInterface* VignetteEffectMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* VignetteMaterialDynamicInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Effects")
	float BlurStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Effects")
	float BlurDistance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Effects")
	float BlurRadius = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Effects")
	float VignetteStrength = 0.0f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Effects")
	//FLinearColor VignetteColor = FLinearColor(0.15f, 0.0f, 0.0f, 1.0f);

	UFUNCTION(BlueprintCallable)
	void CalculateEffectValues(float HPRatio);

protected:
	/** Override the correct method to modify post-process settings */
	virtual void ModifyPostProcess(float DeltaTime, float& PostProcessBlendWeight, FPostProcessSettings& PostProcessSettings) override;

private:
	float PrevVignetteStrength = 0.f;
	float PrevBlurStrength = 1.f;

	// Speed of smoothing (higher = faster)
	float InterpSpeed = 5.0f;
};
