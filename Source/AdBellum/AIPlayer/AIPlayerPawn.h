// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Player/IPlayer.h"
#include "System/PlayerStateInterface.h"
#include "AIPlayerPawn.generated.h"

class UTestSpawnerComponent;

UCLASS()
class ADBELLUM_API AAIPlayerPawn : public APawn, public IIPlayer, public IPlayerStateInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AAIPlayerPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	int PlayerIndex;
	UPROPERTY(Replicated)
	int TeamIndex;
	UPROPERTY(Replicated)
	int Tickets;

public:
	virtual int GetPlayerIndex_Implementation() override;
	virtual void SetPlayerIndex_Implementation(int index) override;
	virtual int GetTeamIndex_Implementation() override;
	virtual void SetTeamIndex_Implementation(int index) override;
	virtual AActor* GetPlayerStateActor_Implementation() override;
	virtual void GameEnded_Implementation(MatchResult Result) override;

	virtual int GetAvailableTickets_Implementation() override;
	virtual void SetInitialTickets_Implementation(int InitialTickets) override;
	virtual bool TryConsumeTickets_Implementation(int NumberOfTickets) override;
	virtual void RespawnFormation_Implementation(AActor* Formation) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTestSpawnerComponent> TestSpawner;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	AActor* TestAOI;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	bool bUseCustomSpawnPoint;

	UFUNCTION(Server, Reliable)
	void SpawnUnits();
};
