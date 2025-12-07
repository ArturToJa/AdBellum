// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "System/PlayerStateInterface.h"
#include "AdBellumPlayerState.generated.h"

class ABaseFormation;

/**
 * 
 */
UCLASS()
class ADBELLUM_API AAdBellumPlayerState : public APlayerState, public IPlayerStateInterface
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetInitialTickets_Implementation(int InitialTickets) override;
	virtual bool TryConsumeTickets_Implementation(int NumberOfTickets) override;
	bool HasEnoughTickets(int RequiredAmount);

	virtual int GetAvailableTickets_Implementation() override;

	UFUNCTION()
	void OnRep_AvailableTickets();

	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnRep_AvailableTickets(int Tickets);

	virtual void RespawnFormation_Implementation(AActor* Formation) override;

	UFUNCTION(Client, Reliable)
	void Client_RespawnFormation(AActor* Formation);

	UFUNCTION(BlueprintCallable)
	void BP_TravelToLobby(APlayerController* PlayerController);

	UFUNCTION(Server, Reliable)
	void Server_TravelToLobby(APlayerController* PlayerController);

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> LobbyLevel;

private:
	UPROPERTY(ReplicatedUsing=OnRep_AvailableTickets)
	int AvailableTickets = 0;
	
	TArray<ABaseFormation*> OwnedFormations;
};
