// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TimerManager.h"
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

	// Create and assign an RTS camera pawn for a specific player controller.
	// The spawned camera will be owned by the player and added to a per-player
	// replication batch so only its owner is expected to report it.
	void CreateRTSCameraForPlayer(class APlayerController* PC, const FTransform& SpawnTransform);
	
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
		/** The class of PlayerController to spawn for players logging in. */
	UPROPERTY(EditAnywhere, NoClear, BlueprintReadOnly, Category = Classes)
	TSubclassOf<APlayerController> VRPlayerControllerClass;

	// Per-client ReplicationReporter actor class to spawn for each connected player.
	UPROPERTY(EditAnywhere, Category = "Replication")
	TSubclassOf<class AReplicationReporter> ReplicationReporterClass;
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

	// Create a replication batch consisting of the provided actors. The batch
	// will be assigned a batch id and the server will inform each connected
	// player's ReplicationReporter about the batch contents. Returns the
	// assigned batch id.
	// Create an open batch. If ExpectedPlayers is empty, the batch is treated
	// as global (will target all current and future players). If non-empty,
	// only those players will be expected to report. bMustWait controls whether
	// GameMode should wait for this batch before considering the match ready.
	int32 CreateOpenBatch(int32 TargetSize = 32, float TimeoutSeconds = 0.5f, const TArray<FString>& ExpectedPlayers = TArray<FString>(), bool bMustWait = true);

	// Add an actor to an existing open batch on the server. When the batch
	// reaches TargetSize or the flush timer fires the batch will be flushed
	// and sent to clients. Returns the id of the batch the actor was placed
	// into.
	int32 AddActorToBatch(AActor* Actor);

	// Force an open batch to flush immediately (send to clients even if not
	// full).
	void ForceFlushBatch(int32 BatchId);

	// Called by AReplicationReporter on the server when a client reports that
	// it has successfully received and processed all actors in a batch.
	void OnClientReportedBatchComplete(const FString& PlayerNetId, int32 BatchId);

	// Called when a per-client ReplicationReporter notifies the server it has
	// been successfully replicated to the client and is ready to receive
	// batch notifications.
	void OnReporterReady(const FString& PlayerNetId);

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

	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	int UnitSpawnMultiplier = 1;
	
	UPROPERTY(EditDefaultsOnly, Category = "Formation")
	TSubclassOf<ABaseFormation> DefaultFormationClass;

	void SpawnUnitsForPlayer(AActor* Player, int TeamId, int PlayerId, ABaseFormation* Formation, TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, ABaseSpawnArea* SpawnArea = nullptr);
	void SpawnFormationForPlayer(AActor* Player, int TeamId, int PlayerId, TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, ABaseSpawnArea* SpawnArea = nullptr);
	void SpawnWeaponsForUnit(AActor* Unit, TArray<FWeaponPrefabDataStruct> WeaponPrefabData);

private:
	// Everything the GameMode needs to track for one in-flight replication
	// batch: its actors, when/how it should flush, and which players are
	// expected to (and have) acknowledged receiving it. Previously this was
	// spread across nine parallel TMaps all keyed by BatchId; keeping them
	// together means a batch's lifecycle (create/flush/remove) is a single
	// map operation instead of nine that have to be kept in sync by hand.
	struct FReplicationBatch
	{
		TArray<TWeakObjectPtr<AActor>> Actors;
		int32 TargetSize = 0;
		float TimeoutSeconds = 0.f;
		FTimerHandle TimerHandle;
		bool bFlushed = false;
		// Whether a flushed batch should be sent to players that join later
		bool bIncludeFuturePlayers = false;
		// Whether GameMode must wait for this batch to be reported before allowing match start
		bool bMustWait = true;
		// Which players were expected to report (netid strings)
		TSet<FString> ExpectedPlayers;
		// Which players have reported
		TSet<FString> ReportsPerPlayer;

		// Units/weapons in this batch that still need to be told to
		// configure themselves (mesh/customization) on a given client.
		// Populated by AddUnitToBatch/AddWeaponToBatch alongside Actors, and
		// sent out per-player from OnClientReportedBatchComplete - only once
		// we know that specific player has actually received these actors,
		// never before.
		TArray<TWeakObjectPtr<AActor>> UnitActors;
		TArray<FMeshCreatorPrefabStruct> UnitPrefabsData;
		TArray<TWeakObjectPtr<AActor>> WeaponActors;
		TArray<FUnitWeaponDataStruct> WeaponPrefabsData;
	};

	// Batch bookkeeping
	int32 NextReplicationBatchId = 1;
	TMap<int32, FReplicationBatch> ReplicationBatches;

	// Which players have their per-client ReplicationReporter ready
	TSet<FString> ReporterReadyPlayers;

	// Whether GameMode has completed the reporter-ready phase
	bool bReportersReadyPhaseComplete = false;

	// Which players have finished reporting required batches and are ready to
	// progress to their next per-player initialization phase.
	TSet<FString> PlayersFinishedReplication;

	// Called when a single player's replication (batches they are expected to
	// report) is complete and the server can progress that player to the next
	// initialization phase.
	void OnPlayerReplicationFinished(const FString& PlayerNetId);

	// Adds a unit/weapon to whichever batch AddActorToBatch places it in,
	// and records its configuration data (mesh prefab / weapon customization)
	// against that same batch so it can be sent to each player individually
	// once that player confirms they've actually received it - see
	// OnClientReportedBatchComplete.
	void AddUnitToBatch(AActor* Unit, const FMeshCreatorPrefabStruct& Prefab);
	void AddWeaponToBatch(AActor* Weapon, const FUnitWeaponDataStruct& Prefab);

	// Sends this batch's unit/weapon configuration data to the single player
	// identified by PlayerNetId. Called once that player has confirmed (via
	// the replication batch/reporter handshake) that they've actually
	// received these actors - never before, so Client_SetUnitPrefab/
	// Client_OnWeaponCreated never race ahead of replication.
	void SendBatchConfigurationToPlayer(const FReplicationBatch& Batch, const FString& PlayerNetId);

	// Finds the AAdBellumPlayerController for a connected player by their
	// unique net id string, or nullptr if none is currently connected.
	class AAdBellumPlayerController* FindPlayerControllerByNetId(const FString& PlayerNetId) const;

	// Active open batch id used for global spawns (0 = none)
	int32 ActiveOpenBatchId = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Replication")
	int32 DefaultBatchSize = 32;

	UPROPERTY(EditDefaultsOnly, Category = "Replication")
	float DefaultBatchTimeout = 0.5f;

	// RTS Camera pawn class (moved from PlayerController)
	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<class ARTSPlayer> RTSCameraClass;

	void SpawnAIPlayer(int TeamId, int PlayerId, FMultiplayerDataStruct& PlayerData);

	void CountPendingConnections();

	void InitializeSpawnAreas();

	void OnNewPlayerArrived(FString PlayerNetId);

	void InitializeGameMode();
	void AutoPossessPlayers();

	void FinalizeReplication();

	// Timer callback to flush an open batch
	void OnBatchFlushTimer(int32 BatchId);

	TArray<TArray<FMultiplayerDataStruct>> MultiplayerTeams;
	bool bTraveledFromLobby;
	TArray<TArray<ABaseSpawnArea*>> SpawnAreas;
	int ConnectedPlayers;
	int PendingConnections;
	EGameMode GameModeType = EGameMode::TeamDeathmatch;

	FString AdminNetId;

	GameModeBase* GameMode;
	bool bIsInitializing = false;
};
