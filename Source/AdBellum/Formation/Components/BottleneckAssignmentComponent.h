#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BottleneckAssignmentComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ADBELLUM_API UBottleneckAssignmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void SolveAssignmentAsync(const TArrayView<APawn*> Units, const TArrayView<FVector> Targets, TFunction<void(const TMap<APawn*, FVector>)> OnCompleted);

private:
	bool CanAssignWithThreshold(const TArray<TArray<float>>& CostMatrix, float Threshold, TMap<int32, int32>& OutAssignment);
	void BuildCostMatrix(const TArrayView<APawn*>& Units, const TArrayView<FVector>& Targets, TArray<TArray<float>>& OutMatrix);
};