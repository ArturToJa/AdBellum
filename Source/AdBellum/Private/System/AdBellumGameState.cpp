// Fill out your copyright notice in the Description page of Project Settings.


#include "System/AdBellumGameState.h"
#include "Selectable.h"
#include "GameFramework/PlayerState.h"
#include "Player/AdBellumPlayerController.h"

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

void AAdBellumGameState::SetUnitPrefab(TArray<AActor*> Units, TArray<FMeshCreatorPrefabStruct> Prefabs)
{
	for (TObjectPtr<APlayerState> PlayerState : PlayerArray)
	{
		if (AAdBellumPlayerController* AdPlayerController = Cast<AAdBellumPlayerController>(PlayerState->GetPlayerController()))
		{
			AdPlayerController->Client_SetUnitPrefab(Units, Prefabs);
		}
	}
}

void AAdBellumGameState::OnWeaponCreated(TArray<AActor*> Weapons, TArray<FUnitWeaponDataStruct> WeaponPrefabs)
{
	for (TObjectPtr<APlayerState> PlayerState : PlayerArray)
	{
		if (AAdBellumPlayerController* AdPlayerController = Cast<AAdBellumPlayerController>(PlayerState->GetPlayerController()))
		{
			AdPlayerController->Client_OnWeaponCreated(Weapons, WeaponPrefabs);
		}
	}
}
