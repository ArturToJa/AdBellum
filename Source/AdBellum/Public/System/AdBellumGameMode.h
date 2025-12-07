// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Player/IPlayer.h"
#include "System/AdBellumGameInstance.h"
#include "AdBellumGameState.h"
#include "System/GameModes/GameModeBase.h"
#include "AdBellumGameMode.generated.h"

class ABaseSpawnArea;
class ABaseFormation;

USTRUCT(BlueprintType)
struct ADBELLUM_API FPlayerSquadDataArrayStruct
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FPlayerSquadDataStruct> PlayerSquadDataArray;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FPlayerTeamData
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FUnitDataStruct> ownedUnits;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 playerNumber;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		AActor* playerReference;
};

UCLASS()
class ADBELLUM_API AAdBellumGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
		/** The class of PlayerController to spawn for players logging in. */
	UPROPERTY(EditAnywhere, NoClear, BlueprintReadOnly, Category = Classes)
	TSubclassOf<APlayerController> VRPlayerControllerClass;
	UPROPERTY(EditAnywhere, NoClear, BlueprintReadOnly, Category = Classes)
	TSubclassOf<AActor> AIPlayerClass;

	virtual APlayerController* SpawnPlayerController(ENetRole InRemoteRole, const FString& Options) override;

	virtual bool ReadyToStartMatch_Implementation() override;

	virtual void HandleMatchHasStarted() override;
	virtual void EndMatch() override;

	void TeamWins(int32 TeamIndex);
	void Draw();
protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	virtual void Logout(AController* ExitingPlayer) override;
public:
	int AddPlayer(int Team, AActor* Player);
	void AddUnitForPlayer(AActor* Unit, AActor* Player, FMeshCreatorPrefabStruct UnitPrefab);
	void RemoveUnitForPlayer(AActor* Unit, AActor* Player);
	ABaseSpawnArea* GetDefaultSpawnArea(int32 TeamIndex);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IsEnemyUnit(AActor* Unit, AActor* Player);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	TArray<AActor*> GetPlayerEnemyUnits(AActor* PlayerActor);

	void NotifyActorReplicated(FString NetId, AActor* ReplicatedActor);

	TArray<TArray<FPlayerTeamData>> PlayersData;
	TArray<AActor*> AIPlayers;

	UPROPERTY(EditDefaultsOnly)
	bool bShouldSpawnDefaultAIPlayer;

	UPROPERTY(EditDefaultsOnly)
	int NetworkPlayers;

	UPROPERTY(EditDefaultsOnly)
	TArray<FPlayerSquadDataArrayStruct> DefaultPlayerSquads;

	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	FDataTableRowHandle WeaponData;

	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	int InitialTickets = 5;
	
	UPROPERTY(EditDefaultsOnly, Category = "Formation")
	TSubclassOf<ABaseFormation> DefaultFormationClass;

	void SpawnUnitsForPlayer(AActor* Player, int TeamId, int PlayerId, ABaseFormation* Formation, TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, ABaseSpawnArea* SpawnArea = nullptr, bool bIsDefault = false);
	void SpawnFormationForPlayer(AActor* Player, int TeamId, int PlayerId, TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, ABaseSpawnArea* SpawnArea = nullptr, bool bIsDefault = false);
	void SpawnWeaponsForUnit(AActor* Unit, TArray<FWeaponPrefabDataStruct> WeaponPrefabData, bool bIsDefault = false);

private:
	void SpawnAIPlayer(int TeamId, int PlayerId, FMultiplayerDataStruct& PlayerData);
	
	void SetupUnits();
	void ScheduleSpawnTimer();
	void CountPendingConnections();

	void InitializeSpawnAreas();

	TMap<FString, bool> InitializePlayerIsReplicatedMap();
	void OnNewPlayerArrived(FString PlayerNetId);

	void InitializeGameMode();
	void AutoPossessPlayers();

	TArray<TArray<FMultiplayerDataStruct>> MultiplayerTeams;
	bool bTraveledFromLobby;
	TArray<TArray<ABaseSpawnArea*>> SpawnAreas;
	int ConnectedPlayers;
	int PendingConnections;
	FTimerHandle SpawnTimerHandle;
	EGameMode GameModeType = EGameMode::TeamDeathmatch;

	FString AdminNetId;

	TArray<AActor*> AllUnits;
	TArray<FMeshCreatorPrefabStruct> AllUnitsPrefabs;
	TArray<AActor*> AllWeapons;
	TArray<FUnitWeaponDataStruct> AllWeaponsPrefabs;
	TMap<AActor*, TMap<FString, bool>> AllActorsMap;

	TArray<AActor*> AllUnitsReady;
	TArray<FMeshCreatorPrefabStruct> AllUnitsPrefabsReady;
	TArray<AActor*> AllWeaponsReady;
	TArray<FUnitWeaponDataStruct> AllWeaponsPrefabsReady;

	GameModeBase* GameMode;
	bool bIsInitializing = false;
};
