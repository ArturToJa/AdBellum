// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Library/MeshCreatorParamsStruct.h"
#include "MainMenuPlayerController.generated.h"

/**
 * PlayerController used in the main menu/lobby. Server RPCs that a specific
 * client needs to call (e.g. reporting their chosen squad) belong here
 * rather than on AMainMenuGameState: a client only owns its own
 * PlayerController (ROLE_AutonomousProxy) - GameState is never owned by any
 * particular client (it's ROLE_SimulatedProxy everywhere), so a Server RPC
 * declared on it is silently dropped client-side for every non-host player
 * before it ever reaches the network.
 */
UCLASS()
class ADBELLUM_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// Called by this player's own client to report the squad they picked for
	// their lobby slot. Forwards to AMainMenuGameState::ApplyPlayerSquad on
	// the server once received.
	UFUNCTION(Server, Reliable)
	void Server_SetPlayerSquad(const FString& SquadName, const TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, int Team, int Slot);
};
