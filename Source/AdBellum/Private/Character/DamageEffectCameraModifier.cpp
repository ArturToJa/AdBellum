#include "Character/DamageEffectCameraModifier.h"
#include "Kismet/KismetMathLibrary.h"
#include <Kismet/KismetSystemLibrary.h>

UDamageEffectCameraModifier::UDamageEffectCameraModifier()
{
}

void UDamageEffectCameraModifier::AddedToCamera(APlayerCameraManager* Camera)
{
	if (!BlurEffectMaterial || !VignetteEffectMaterial) return;

	// Create dynamic instance if not already
	if (!BlurMaterialDynamicInstance)
	{
		BlurMaterialDynamicInstance = UMaterialInstanceDynamic::Create(BlurEffectMaterial, this);
	}

	if (!VignetteMaterialDynamicInstance)
	{
		VignetteMaterialDynamicInstance = UMaterialInstanceDynamic::Create(VignetteEffectMaterial, this);
	}
}

void UDamageEffectCameraModifier::ModifyPostProcess(float DeltaTime, float& PostProcessBlendWeight, FPostProcessSettings& PPSettings)
{
	PostProcessBlendWeight = 1.0f; // Ensure the modifier applies

	float TempVignetteStrength = FMath::FInterpTo(PrevVignetteStrength, VignetteStrength, DeltaTime, InterpSpeed);
	float TempBlurStrength = FMath::FInterpTo(PrevBlurStrength, BlurStrength, DeltaTime, InterpSpeed);

	// Store for next frame
	PrevVignetteStrength = TempVignetteStrength;
	PrevBlurStrength = TempBlurStrength;

	BlurMaterialDynamicInstance->SetScalarParameterValue("BlurStrength", TempBlurStrength);
	PPSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, BlurMaterialDynamicInstance));

	VignetteMaterialDynamicInstance->SetScalarParameterValue("VignetteStrength", TempVignetteStrength);
	PPSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, VignetteMaterialDynamicInstance));
}
void UDamageEffectCameraModifier::CalculateEffectValues(float HPRatio)
{
	if (HPRatio == 1.0f)
	{
		VignetteStrength = 0.0f;
		PrevVignetteStrength = 0.0f;
		BlurStrength = 1.0f;
		PrevBlurStrength = 1.0f;
		//BlurMaterialDynamicInstance = nullptr;
		//VignetteMaterialDynamicInstance = nullptr;
	}
	else
	{
		if (HPRatio >= 0.5f)
		{
			VignetteStrength = -10.0f * HPRatio + 10; // Linearly interpolate from 0 to 5.0 between 1.0 and 0.5 HP ratio
		}
		else
		{
			VignetteStrength = 5.0f; // Max vignette strength when HP is below 50%
		}

		//Vignette Color
		//if (HPRatio <= 0.5f)
		//{
		//	VignetteColor.R = HPRatio / 2.0f;
		//}

		// Blur Strength
		if (HPRatio >= 0.25f)
		{
			BlurStrength = 1.0f; // No blur when HP is above 25%
		}
		else if (HPRatio < 0.25f && HPRatio >= 0.1f)
		{
			// Smoothly increase blur strength from 1.0 to 3.0 between 25% and 0% HP
			BlurStrength = -40.0f * (HPRatio / 3.0f) + 13.0f / 3.0f;
		}
		else
		{
			BlurStrength = 3.0f;
		}
	}
	//UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("HP: %.2f | Blur: %.2f | Vignette: %.2f"), HPRatio, BlurStrength, VignetteStrength));
}