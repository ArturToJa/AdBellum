// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Generators/RectangleGridGenerator.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

URectangleGridGenerator::URectangleGridGenerator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	GenerateAround = UEnvQueryContext_Querier::StaticClass();
	GridHalfWidth.DefaultValue = 500.0f;
	GridHalfHeight.DefaultValue = 300.0f;
	GridAngle.DefaultValue = 0.0f;
	SpaceBetween.DefaultValue = 100.0f;
}

void URectangleGridGenerator::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	UObject* BindOwner = QueryInstance.Owner.Get();
	GridHalfWidth.BindData(BindOwner, QueryInstance.QueryID);
	GridHalfHeight.BindData(BindOwner, QueryInstance.QueryID);
	GridAngle.BindData(BindOwner, QueryInstance.QueryID);
	SpaceBetween.BindData(BindOwner, QueryInstance.QueryID);

	float WidthValue = GridHalfWidth.GetValue();
	float HeightValue = GridHalfHeight.GetValue();
	float AngleValue = GridAngle.GetValue();
	float DensityValue = SpaceBetween.GetValue();

	const int32 WidthCount = FPlatformMath::TruncToInt(WidthValue / DensityValue);
	const int32 HalfWidthCount = WidthCount / 2;
	const int32 HeightCount = FPlatformMath::TruncToInt(HeightValue / DensityValue);
	const int32 HalfHeightCount = HeightCount / 2;

	TArray<FVector> ContextLocations;
	QueryInstance.PrepareContext(GenerateAround, ContextLocations);

	TArray<FNavLocation> GridPoints;
	GridPoints.Reserve(WidthCount * HeightCount * ContextLocations.Num());

	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	{
		for (int32 IndexX = 0; IndexX < WidthCount; ++IndexX)
		{
			for (int32 IndexY = 0; IndexY < HeightCount; ++IndexY)
			{
				const FNavLocation TestPoint = FNavLocation(ContextLocations[ContextIndex] - FVector(DensityValue * (IndexX - HalfWidthCount), DensityValue * (IndexY - HalfHeightCount), 0).RotateAngleAxis(AngleValue, FVector::ZAxisVector));
				GridPoints.Add(TestPoint);
			}
		}
	}

	ProjectAndFilterNavPoints(GridPoints, QueryInstance);
	StoreNavPoints(GridPoints, QueryInstance);
}


FText URectangleGridGenerator::GetDescriptionTitle() const
{
	return FText::Format(LOCTEXT("SimpleRectangleGridDescriptionGenerateAroundContext", "{0}: generate around {1}"),
		Super::GetDescriptionTitle(), UEnvQueryTypes::DescribeContext(GenerateAround));
};

FText URectangleGridGenerator::GetDescriptionDetails() const
{
	FText Desc = FText::Format(LOCTEXT("SimpleRectangleGridDescription", "width: {0}, height: {1}, angle: {2}, space between: {3}"),
		FText::FromString(GridHalfWidth.ToString()), FText::FromString(GridHalfHeight.ToString()), FText::FromString(GridAngle.ToString()), FText::FromString(SpaceBetween.ToString()));

	FText ProjDesc = ProjectionData.ToText(FEnvTraceData::Brief);
	if (!ProjDesc.IsEmpty())
	{
		FFormatNamedArguments ProjArgs;
		ProjArgs.Add(TEXT("Description"), Desc);
		ProjArgs.Add(TEXT("ProjectionDescription"), ProjDesc);
		Desc = FText::Format(LOCTEXT("SimpleRecatangleGridDescriptionWithProjection", "{Description}, {ProjectionDescription}"), ProjArgs);
	}

	return Desc;
}