// Fill out your copyright notice in the Description page of Project Settings.


#include "AdBellumGameState.h"
#include "Interfaces/Selectable.h"

void AAdBellumGameState::BeginPlay()
{
	Super::BeginPlay();
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save HUDs into a map
	InstancedStaticMeshManager = GetWorld()->SpawnActor<AInstancedStaticMeshManager>(InstancedStaticMeshManagerClass, SpawnInfo);
}

AInstancedStaticMeshManager* AAdBellumGameState::GetInstancedStaticMeshManager_Implementation()
{
	return InstancedStaticMeshManager;
}

void AAdBellumGameState::SetSelectionCircle(bool Visible, AActor* Unit)
{
	AInstancedStaticMeshManager* ISMM = GetInstancedStaticMeshManager();
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] SetSelectionCircle: Visible=%d Unit=%s ISMM=%s"),
		Visible, Unit ? *Unit->GetName() : TEXT("NULL"), ISMM ? TEXT("valid") : TEXT("NULL"));
	if (ISMM)
	{
		if (Visible)
		{
			ISMM->CreateSelectionInstance(Unit);
		}
		else
		{
			APlayerController* PlayerController = GEngine->GetFirstLocalPlayerController(GetWorld());
			ISMM->RemoveSelectionInstance(ISelectable::Execute_GetInstanceIndex(Unit));
		}
	}
}

