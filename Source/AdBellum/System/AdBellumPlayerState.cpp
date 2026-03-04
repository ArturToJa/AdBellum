// Fill out your copyright notice in the Description page of Project Settings.


#include "AdBellumPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Interfaces/OwnershipInterface.h"
#include "AdBellumGameMode.h"
#include "Formation/FormationInterface.h"



void AAdBellumPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// Call the Super
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Add properties to replicated for the derived class
	DOREPLIFETIME(AAdBellumPlayerState, AvailableTickets);
}

void AAdBellumPlayerState::SetInitialTickets_Implementation(int InitialTickets)
{
	AvailableTickets = InitialTickets;
}

bool AAdBellumPlayerState::TryConsumeTickets_Implementation(int NumberOfTickets)
{
	if (HasEnoughTickets(NumberOfTickets))
	{
		AvailableTickets -= NumberOfTickets;
		return true;
	}
	return false;
}

bool AAdBellumPlayerState::HasEnoughTickets(int RequiredAmount)
{
	return AvailableTickets >= RequiredAmount;
}

int AAdBellumPlayerState::GetAvailableTickets_Implementation()
{
	return AvailableTickets;
}

void AAdBellumPlayerState::OnRep_AvailableTickets()
{
	BP_OnRep_AvailableTickets(AvailableTickets);
}

void AAdBellumPlayerState::RespawnFormation_Implementation(AActor* Formation)
{
	int32 TeamIndex = IOwnershipInterface::Execute_GetTeamIndex(Formation);
	// This condition should be modified, what if player still has other formations alive but no more tickets?
	// What if player has some tickets but not enough to respawn formation?
	if (TryConsumeTickets_Implementation(IFormationInterface::Execute_GetFormationCost(Formation)))
	{

		AAdBellumGameMode* GameMode = Cast<AAdBellumGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
		check(GameMode);
		IFormationInterface::Execute_RespawnFormation(Formation, GameMode->GetDefaultSpawnArea(TeamIndex));

		Client_RespawnFormation(Formation);
	}
	else
	{
		// Notify GameMode that this player has no more tickets and lost a formation
		
		PlayerDiedDelegate.ExecuteIfBound(TeamIndex);
	}
}

void AAdBellumPlayerState::Client_RespawnFormation_Implementation(AActor* Formation)
{
	// Show Unit spawn widget
}

void AAdBellumPlayerState::BP_TravelToLobby(APlayerController* PlayerController)
{
	Server_TravelToLobby(PlayerController);
}

void AAdBellumPlayerState::Server_TravelToLobby_Implementation(APlayerController* PlayerController)
{
	PlayerController->ClientTravel(LobbyLevel.GetAssetName().Append("?GameMode=1"), ETravelType::TRAVEL_Relative, true);
}