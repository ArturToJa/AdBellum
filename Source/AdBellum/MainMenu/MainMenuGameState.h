// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "System/AdBellumGameInstance.h"
#include "MainMenuGameState.generated.h"

USTRUCT(BlueprintType)
struct ADBELLUM_API FMapDataStruct
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> MapAsset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FString MapDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UTexture2D* Minimap;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FGameModeDataStruct
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FString GameMode;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FString GameModeDescription;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UTexture2D* GameModeIcon;
};

UENUM(BlueprintType)
enum class LobbyRole : uint8
{
	Admin UMETA(DisplayName = "Admin"),
	Player UMETA(DisplayName = "Player")
};

UCLASS()
class ADBELLUM_API AMainMenuGameState : public AGameState
{
	GENERATED_BODY()

public:
	AMainMenuGameState(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	UPROPERTY(BlueprintReadOnly)
	TArray<FPlayerDataStruct> TeamAlpha;
	UPROPERTY(BlueprintReadOnly)
	TArray<FPlayerDataStruct> TeamBeta;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FMapDataStruct> MapArray;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGameModeDataStruct> GameModeArray;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<int> TicketsArray;

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateTeamData(int team, int slot, FPlayerDataStruct PlayerInfo);

	void AddNewPlayer(FPlayerDataStruct PlayerInfo, int& team, int& slot, FString UniqueId);
	UFUNCTION(BlueprintCallable)
	void RemovePlayer(int team, int slot, FString UniqueId);
	void OnPlayerLogout(AController* LeavingPlayer, FString UniqueId);
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void UpdatePlayer(int team, int slot, FPlayerDataStruct PlayerInfo);

	void RestorePlayer(FString PlayerNetId, FPlayerDataStruct PlayerData);

	void NotifyNewPlayer(AController* NewPlayer);
	void SetMapIndex(int _MapIndex);
	void SetGameModeIndex(int _GameModeIndex);
	void SetTicketsIndex(int _TicketsIndex);

	UFUNCTION(BlueprintImplementableEvent)
	void NotifyNewPlayerOnPlayerList(AController* NewPlayer);
	UFUNCTION(BlueprintImplementableEvent)
	void NotifyNewMap(int NewMapIndex);
	UFUNCTION(BlueprintImplementableEvent)
	void NotifyNewGameMode(int NewGameModeIndex);
	UFUNCTION(BlueprintImplementableEvent)
	void NotifyRole(AController* NewPlayer, LobbyRole LobbyRole);
	UFUNCTION(BlueprintImplementableEvent)
	void NotifyOwnership(AController* NewPlayer, int team, int slot);
	UFUNCTION(BlueprintImplementableEvent)
	void SetPlayerSquadName(const FString& SquadName, int Team, int Slot);
	UFUNCTION(BlueprintImplementableEvent)
	void NotifyNewTicketsIndex(int NewTicketsIndex);

	// Applies a player's chosen squad to the server-authoritative team arrays.
	// Only ever runs on the server: called from
	// AMainMenuPlayerController::Server_SetPlayerSquad once that client's
	// selection has actually arrived. Not an RPC itself - a client can only
	// call Server RPCs on actors it owns, and no client owns the GameState.
	void ApplyPlayerSquad(const FString& SquadName, const TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, int Team, int Slot);

	UFUNCTION(BlueprintCallable)
	void SetPlayerSquad(const FString& SquadName, int Team, int Slot);

	void SetMultiplayerData(int GameModeIndex, int TicketsIndex);
};
