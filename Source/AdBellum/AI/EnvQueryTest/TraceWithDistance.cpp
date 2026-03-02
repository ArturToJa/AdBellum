#include "TraceWithDistance.h"
#include "UObject/Package.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Engine/World.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"

UEnvQueryTest_TraceWithDistance::UEnvQueryTest_TraceWithDistance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	SetWorkOnFloatValues(false);

	Context = UEnvQueryContext_Querier::StaticClass();
	TraceData.SetGeometryOnly();
}

void UEnvQueryTest_TraceWithDistance::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* DataOwner = QueryInstance.Owner.Get();
	BoolValue.BindData(DataOwner, QueryInstance.QueryID);
	TraceFromContext.BindData(DataOwner, QueryInstance.QueryID);
	ItemHeightOffset.BindData(DataOwner, QueryInstance.QueryID);
	ContextHeightOffset.BindData(DataOwner, QueryInstance.QueryID);

	bool bWantsHit = BoolValue.GetValue();
	bool bTraceToItem = TraceFromContext.GetValue();
	float ItemZ = ItemHeightOffset.GetValue();
	float ContextZ = ContextHeightOffset.GetValue();

	TArray<FVector> ContextLocations;
	if (!QueryInstance.PrepareContext(Context, ContextLocations))
	{
		return;
	}

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(EnvQueryTrace), TraceData.bTraceComplex);

	TArray<AActor*> IgnoredActors;
	if (QueryInstance.PrepareContext(Context, IgnoredActors))
	{
		TraceParams.AddIgnoredActors(IgnoredActors);
	}

	ECollisionChannel TraceCollisionChannel = ECC_WorldStatic;
	FCollisionResponseParams ResponseParams = FCollisionResponseParams::DefaultResponseParam;
	if (TraceData.TraceMode == EEnvQueryTrace::Type::GeometryByProfile)
	{
		if (!UCollisionProfile::GetChannelAndResponseParams(TraceData.TraceProfileName, TraceCollisionChannel, ResponseParams))
		{
			for (FEnvQueryInstance::ItemIterator It(&*this, QueryInstance); It; ++It)
			{
				It.SetScore(TestPurpose, FilterType, !bWantsHit, bWantsHit);
			}

			return;
		}
	}
	else if (TraceData.TraceMode == EEnvQueryTrace::Type::GeometryByChannel)
	{
		TraceCollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceData.TraceChannel);
	}

	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	{
		ContextLocations[ContextIndex].Z += ContextZ;
	}

	for (FEnvQueryInstance::ItemIterator It(&*this, QueryInstance); It; ++It)
	{
		const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex()) + FVector(0, 0, ItemZ);
		AActor* ItemActor = GetItemActor(QueryInstance, It.GetIndex());

		FCollisionQueryParams PerItemTraceParams(TraceParams);
		PerItemTraceParams.AddIgnoredActor(ItemActor);

		for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
		{
			FHitResult HitResult;
			bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, ItemLocation, ContextLocations[ContextIndex], TraceCollisionChannel, PerItemTraceParams, ResponseParams);
			if (bHit)
			{
				bHit = bHit && HitResult.Distance < 500.f;
				//It.SetScore(TestPurpose, FilterType, bHit, bWantsHit);
				// wykrywanie, czy przeszkoda jest wysoka czy niska
				//HitResult.GetActor()->GetComponentsBoundingBox(HitResult.Component.Get(), HitResult.Location, HitResult.Normal);
			}
			if (bHit)
			{
				//float Score = (500.f - HitResult.Distance) / 500.f;
				It.SetScore(EEnvTestPurpose::FilterAndScore, EEnvTestFilterType::Maximum, HitResult.Distance, 500.f, 500.f);
				//It.SetScore(EEnvTestPurpose::Score, EEnvTestFilterType::Range, Score, 0.f, 1.f);
				//It.ForceItemState(EEnvItemStatus::Passed);
				
			}
			else
			{
				It.SetScore(EEnvTestPurpose::Score, EEnvTestFilterType::Range, 0.f, 0.f, 1.f);
				It.ForceItemState(EEnvItemStatus::Failed);
			}
		}
	}
}

FText UEnvQueryTest_TraceWithDistance::GetDescriptionTitle() const
{
	UEnum* ChannelEnum = StaticEnum<ETraceTypeQuery>();
	FString ChannelDesc = ChannelEnum->GetDisplayNameTextByValue(TraceData.TraceChannel).ToString();

	FString DirectionDesc = TraceFromContext.IsDynamic() ?
		FString::Printf(TEXT("%s, direction: %s"), *UEnvQueryTypes::DescribeContext(Context).ToString(), *TraceFromContext.ToString()) :
		FString::Printf(TEXT("%s %s"), TraceFromContext.DefaultValue ? TEXT("from") : TEXT("to"), *UEnvQueryTypes::DescribeContext(Context).ToString());

	return FText::FromString(FString::Printf(TEXT("%s: %s on %s"),
		*Super::GetDescriptionTitle().ToString(), *DirectionDesc, *ChannelDesc));
}

FText UEnvQueryTest_TraceWithDistance::GetDescriptionDetails() const
{
	return FText::Format(FText::FromString("{0}\n{1}"),
		TraceData.ToText(FEnvTraceData::Detailed), DescribeBoolTestParams("hit"));
}

void UEnvQueryTest_TraceWithDistance::PostLoad()
{
	Super::PostLoad();
	TraceData.OnPostLoad();
}