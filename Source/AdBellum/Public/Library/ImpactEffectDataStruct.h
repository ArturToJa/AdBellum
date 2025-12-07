// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ImpactEffectDataStruct.generated.h"

class UNiagaraSystem;
class USoundBase;

USTRUCT(BlueprintType)
struct FImpactEffectDataStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundBase* ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNiagaraSystem* ImpactEffect;
};

USTRUCT(BlueprintType)
struct FImpactEffectLibrary
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TEnumAsByte<EPhysicalSurface>, FImpactEffectDataStruct> ImpactLibrary;

	FImpactEffectDataStruct GetImpactEffectData(EPhysicalSurface SurfaceType) const
	{
		TEnumAsByte<EPhysicalSurface> SurfaceEnum = SurfaceType;
		return ImpactLibrary[SurfaceEnum];
	}

};