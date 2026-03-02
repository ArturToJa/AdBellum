#include "BottleneckAssignmentComponent.h"
#include "Async/Async.h"

void UBottleneckAssignmentComponent::SolveAssignmentAsync(
	const TArrayView<APawn*> Units,
	const TArrayView<FVector> Targets,
	TFunction<void(const TMap<APawn*, FVector>)> OnCompleted)
{
	const int32 N = Units.Num();
	if (N != Targets.Num() || N == 0) return;

	// Run in background thread
	Async(EAsyncExecution::ThreadPool, [this, Units, Targets, OnCompleted]()
		{
			TArray<TArray<float>> CostMatrix;
			BuildCostMatrix(Units, Targets, CostMatrix);

			float Lo = 0.0f;
			float Hi = 0.0f;
			for (const auto& Row : CostMatrix)
				for (float Cost : Row)
					Hi = FMath::Max(Hi, Cost); // Note: max DistSquared

			TMap<int32, int32> FinalAssignment;
			while (Hi - Lo > 1.0f) // Precision threshold (1 squared unit)
			{
				float Mid = (Lo + Hi) / 2.0f;
				TMap<int32, int32> TempAssignment;
				if (CanAssignWithThreshold(CostMatrix, Mid, TempAssignment))
				{
					Hi = Mid;
					FinalAssignment = TempAssignment;
				}
				else
				{
					Lo = Mid;
				}
			}

			TMap<APawn*, FVector> Result;
			for (const auto& Pair : FinalAssignment)
			{
				Result.Add(Units[Pair.Key], Targets[Pair.Value]);
			}

			AsyncTask(ENamedThreads::GameThread, [=]()
				{
					OnCompleted(Result);
				});
		});
}

void UBottleneckAssignmentComponent::BuildCostMatrix(const TArrayView<APawn*>& Units, const TArrayView<FVector>& Targets, TArray<TArray<float>>& OutMatrix)
{
	const int32 N = Units.Num();
	OutMatrix.SetNum(N);
	for (int32 i = 0; i < N; ++i)
	{
		OutMatrix[i].SetNum(N);
		FVector UnitLoc = Units[i]->GetActorLocation();
		for (int32 j = 0; j < N; ++j)
		{
			OutMatrix[i][j] = FVector::DistSquared(UnitLoc, Targets[j]);
		}
	}
}

bool UBottleneckAssignmentComponent::CanAssignWithThreshold(const TArray<TArray<float>>& CostMatrix, float Threshold, TMap<int32, int32>& OutAssignment)
{
	const int32 N = CostMatrix.Num();
	TArray<TArray<bool>> Adjacency;
	Adjacency.SetNum(N);
	for (int32 i = 0; i < N; ++i)
	{
		Adjacency[i].SetNum(N);
		for (int32 j = 0; j < N; ++j)
		{
			Adjacency[i][j] = (CostMatrix[i][j] <= Threshold);
		}
	}

	TArray<int32> MatchToTarget;
	MatchToTarget.Init(-1, N);

	TFunction<bool(int32, TSet<int32>&)> BpmMatch;
	BpmMatch = [&](int32 UnitIdx, TSet<int32>& Visited)
		{
			for (int32 j = 0; j < N; ++j)
			{
				if (Adjacency[UnitIdx][j] && !Visited.Contains(j))
				{
					Visited.Add(j);
					if (MatchToTarget[j] == -1 || BpmMatch(MatchToTarget[j], Visited))
					{
						MatchToTarget[j] = UnitIdx;
						return true;
					}
				}
			}
			return false;
		};

	for (int32 i = 0; i < N; ++i)
	{
		TSet<int32> Visited;
		if (!BpmMatch(i, Visited))
		{
			return false;
		}
	}

	OutAssignment.Empty();
	for (int32 j = 0; j < N; ++j)
	{
		if (MatchToTarget[j] != -1)
		{
			OutAssignment.Add(MatchToTarget[j], j);
		}
	}

	return true;
}
