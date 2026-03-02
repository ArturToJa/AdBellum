#include "MeshMergeFunctionLibrary.h"
#include "SkeletalMeshMerge.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"

DEFINE_LOG_CATEGORY_STATIC(LogMeshMergeLibrary, All, All);

USkeletalMesh* UMeshMergeFunctionLibrary::MergeMeshes(const FSkeletalMeshMergeParams& Params)
{
	TArray<USkeletalMesh*> MeshesToMergeCopy = Params.MeshesToMerge;
	MeshesToMergeCopy.RemoveAll([](USkeletalMesh* InMesh)
		{
			return InMesh == nullptr;
		});

	if (MeshesToMergeCopy.Num() <= 1)
	{
		UE_LOG(LogMeshMergeLibrary, Warning, TEXT("Must provide multiple valid Skeletal Meshes in order to perform a merge."));
		return nullptr;
	}

	EMeshBufferAccess BufferAccess = Params.bNeedsCpuAccess ?
		EMeshBufferAccess::ForceCPUAndGPU :
		EMeshBufferAccess::Default;

	TArray<FSkelMeshMergeSectionMapping> SectionMappings;
	TArray<FSkelMeshMergeMeshUVTransforms> UvTransforms;

	ToMergeParams(Params.MeshSectionMappings, SectionMappings);
	ToMergeParams(Params.UVTransformsPerMesh, UvTransforms);

	bool bRunDuplicateCheck = false;
	USkeletalMesh* BaseMesh = NewObject<USkeletalMesh>();

	if (Params.Skeleton && Params.bSkeletonBefore)
	{
		BaseMesh->SetSkeleton(Params.Skeleton);
		bRunDuplicateCheck = true;

		for (USkeletalMeshSocket* Socket : BaseMesh->GetMeshOnlySocketList())
		{
			if (Socket)
			{
				UE_LOG(LogMeshMergeLibrary, Warning, TEXT("SkelMeshSocket: %s"), *(Socket->SocketName.ToString()));
			}
		}

		for (USkeletalMeshSocket* Socket : BaseMesh->GetSkeleton()->Sockets)
		{
			if (Socket)
			{
				UE_LOG(LogMeshMergeLibrary, Warning, TEXT("SkelSocket: %s"), *(Socket->SocketName.ToString()));
			}
		}
	}

	FSkelMeshMergeUVTransformMapping* UVTransformMapping = new FSkelMeshMergeUVTransformMapping();
	UVTransformMapping->UVTransformsPerMesh = UvTransforms;

	FSkeletalMeshMerge Merger(BaseMesh, MeshesToMergeCopy, SectionMappings, Params.StripTopLODS, BufferAccess, UVTransformMapping);

	if (!Merger.DoMerge())
	{
		UE_LOG(LogMeshMergeLibrary, Warning, TEXT("Merge failed!"));
		return nullptr;
	}

	if (Params.Skeleton && !Params.bSkeletonBefore)
	{
		BaseMesh->SetSkeleton(Params.Skeleton);
	}

	if (bRunDuplicateCheck)
	{
		TArray<FName> SkelMeshSockets;
		TArray<FName> SkelSockets;

		for (USkeletalMeshSocket* Socket : BaseMesh->GetMeshOnlySocketList())
		{
			if (Socket)
			{
				SkelMeshSockets.Add(Socket->GetFName());
				UE_LOG(LogMeshMergeLibrary, Warning, TEXT("SkelMeshSocket: %s"), *(Socket->SocketName.ToString()));
			}
		}

		for (USkeletalMeshSocket* Socket : BaseMesh->GetSkeleton()->Sockets)
		{
			if (Socket)
			{
				SkelSockets.Add(Socket->GetFName());
				UE_LOG(LogMeshMergeLibrary, Warning, TEXT("SkelSocket: %s"), *(Socket->SocketName.ToString()));
			}
		}

		for (const FName& MeshSocketName : SkelMeshSockets)
		{
			if (!SkelSockets.Contains(MeshSocketName))
			{
				UE_LOG(LogMeshMergeLibrary, Warning, TEXT("Socket %s in MeshOnlySocketList not found in Skeleton"), *(MeshSocketName.ToString()));
			}
		}
	}
		
	BaseMesh->UpdateUVChannelData(true);

	return BaseMesh;
}
