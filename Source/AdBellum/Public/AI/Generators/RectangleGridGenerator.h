// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "DataProviders/AIDataProvider.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "RectangleGridGenerator.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Points: Rectangle Grid"))
class ADBELLUM_API URectangleGridGenerator : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_UCLASS_BODY()

	/** half of rectangle's width */
	UPROPERTY(EditDefaultsOnly, Category = Generator, meta = (DisplayName = "GridHalfWidth"))
	FAIDataProviderFloatValue GridHalfWidth;

	/** half of rectangle's height */
	UPROPERTY(EditDefaultsOnly, Category = Generator, meta = (DisplayName = "GridHalfHeight"))
	FAIDataProviderFloatValue GridHalfHeight;

	/** direction of rectangle */
	UPROPERTY(EditDefaultsOnly, Category = Generator, meta = (DisplayName = "GridAngle"))
	FAIDataProviderFloatValue GridAngle;

	/** generation density */
	UPROPERTY(EditDefaultsOnly, Category = Generator)
	FAIDataProviderFloatValue SpaceBetween;

	/** context */
	UPROPERTY(EditDefaultsOnly, Category = Generator)
	TSubclassOf<UEnvQueryContext> GenerateAround;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};
