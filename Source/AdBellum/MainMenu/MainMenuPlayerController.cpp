// Fill out your copyright notice in the Description page of Project Settings.

#include "MainMenuPlayerController.h"
#include "MainMenuGameState.h"

void AMainMenuPlayerController::Server_SetPlayerSquad_Implementation(const FString& SquadName, const TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, int Team, int Slot)
{
	if (AMainMenuGameState* GameState = GetWorld()->GetGameState<AMainMenuGameState>())
	{
		GameState->ApplyPlayerSquad(SquadName, UnitPrefabs, Team, Slot);
	}
}
