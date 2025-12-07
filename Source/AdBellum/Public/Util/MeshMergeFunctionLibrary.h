#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SkeletalMeshMerge.h"
#include "MeshMergeFunctionLibrary.generated.h"

USTRUCT(BlueprintType)
struct ADBELLUM_API FSkelMeshMergeSectionMapping_BP
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	TArray<int32> SectionIDs;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FSkelMeshMergeMeshUVTransforms_BP
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	TArray<FTransform> UVTransforms;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FSkeletalMeshMergeParams
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	TArray<USkeletalMesh*> MeshesToMerge;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	TArray<FSkelMeshMergeSectionMapping_BP> MeshSectionMappings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	TArray<FSkelMeshMergeMeshUVTransforms_BP> UVTransformsPerMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	bool StripTopLODS = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	bool bNeedsCpuAccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	USkeleton* Skeleton = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merge Params")
	bool bSkeletonBefore = true;
};

UCLASS()
class ADBELLUM_API UMeshMergeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Mesh Merge")
	static USkeletalMesh* MergeMeshes(const FSkeletalMeshMergeParams& Params);

private:
	static void ToMergeParams(const TArray<FSkelMeshMergeSectionMapping_BP>& InSectionMappings, TArray<FSkelMeshMergeSectionMapping>& OutSectionMappings)
	{
		if (InSectionMappings.Num() > 0)
		{
			OutSectionMappings.AddUninitialized(InSectionMappings.Num());

			for (int32 i = 0; i < InSectionMappings.Num(); ++i)
			{
				OutSectionMappings[i].SectionIDs = InSectionMappings[i].SectionIDs;
			}
		}
	}
	static void ToMergeParams(const TArray<FSkelMeshMergeMeshUVTransforms_BP>& InUVTransformsPerMesh, TArray<FSkelMeshMergeMeshUVTransforms>& OutUVTransformsPerMesh)
	{
		if (InUVTransformsPerMesh.Num() > 0)
		{
			OutUVTransformsPerMesh.Empty();
			OutUVTransformsPerMesh.AddUninitialized(InUVTransformsPerMesh.Num());

			for (int32 i = 0; i < InUVTransformsPerMesh.Num(); ++i)
			{
				TArray<FTransform>& OutUVTransforms = OutUVTransformsPerMesh[i].UVTransforms;
				const TArray<FTransform>& InUVTransforms = InUVTransformsPerMesh[i].UVTransforms;

				if (InUVTransforms.Num() > 0)
				{
					OutUVTransforms.Empty();
					OutUVTransforms.AddUninitialized(InUVTransforms.Num());

					for (int32 j = 0; j < InUVTransforms.Num(); ++j)
					{
						OutUVTransforms[j] = InUVTransforms[j];
					}
				}
			}
		}
	}
};