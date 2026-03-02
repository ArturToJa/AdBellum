// Fill out your copyright notice in the Description page of Project Settings.


#include "InstancedStaticMeshManager.h"
#include "Unit/Selectable.h"

AInstancedStaticMeshManager::AInstancedStaticMeshManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SelectionCircleInstanced = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>("SelectionCircleInstanced");
}

void AInstancedStaticMeshManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (int32 i = 0; i < SelectionActors.Num(); ++i)
	{
		AActor* SelectionActor = SelectionActors[i];
		if (!SelectionActor->GetVelocity().IsZero())
		{
			FTransform SelectionTransform;
			SelectionTransform.SetLocation(ISelectable::Execute_GetSelectionCircleLocation(SelectionActor));
			SelectionTransform.SetScale3D(ISelectable::Execute_GetSelectionCircleScale(SelectionActor));
			SelectionCircleInstanced->UpdateInstanceTransform(i, SelectionTransform, true, true, true);
		}
	}
}

void AInstancedStaticMeshManager::CreateSelectionInstance_Implementation(AActor* Actor)
{
	if (Actor->GetClass()->ImplementsInterface(USelectable::StaticClass()))
	{
		if (!SelectionActors.Contains(Actor))
		{
			int32 NewIndex = SelectionActors.Add(Actor);
			ISelectable::Execute_SetInstanceIndex(Actor, NewIndex);
			FTransform SelectionTransform;
			SelectionTransform.SetLocation(ISelectable::Execute_GetSelectionCircleLocation(Actor));
			SelectionTransform.SetScale3D(ISelectable::Execute_GetSelectionCircleScale(Actor));
			SelectionCircleInstanced->AddInstance(SelectionTransform, true);
		}
	}
}

void AInstancedStaticMeshManager::RemoveSelectionInstance_Implementation(int32 InstanceIndex)
{
	if (SelectionActors.Num() > InstanceIndex && InstanceIndex >= 0)
	{
		if (SelectionCircleInstanced->RemoveInstance(SelectionCircleInstanced->GetInstanceCount() - 1))
		{
			int32 LastIndex = SelectionActors.Num() - 1;
			ISelectable::Execute_SetInstanceIndex(SelectionActors[InstanceIndex], -1);
			SelectionActors.Swap(InstanceIndex, LastIndex);
			ISelectable::Execute_SetInstanceIndex(SelectionActors[InstanceIndex], InstanceIndex);
			SelectionActors.Pop(EAllowShrinking::No);
		}
	}
}
