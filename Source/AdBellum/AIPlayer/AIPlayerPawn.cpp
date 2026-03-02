// Fill out your copyright notice in the Description page of Project Settings.


#include "AIPlayerPawn.h"
#include "Kismet/GameplayStatics.h"
#include "OrderSystem/Orderable.h"
#include "Unit/Selectable.h"
#include "OrderSystem/Orders/OccupyAreaOfInterestOrder.h"
#include "Player/TestSpawnerComponent.h"
#include "System/AdBellumGameMode.h"
#include "Formation/BaseFormation.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AAIPlayerPawn::AAIPlayerPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	TestSpawner = CreateDefaultSubobject<UTestSpawnerComponent>("TestSpawner");
}

// Called when the game starts or when spawned
void AAIPlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnUnits();
}

void AAIPlayerPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAIPlayerPawn, PlayerIndex);
	DOREPLIFETIME(AAIPlayerPawn, TeamIndex);
}

int AAIPlayerPawn::GetPlayerIndex_Implementation()
{
	return PlayerIndex;
}

void AAIPlayerPawn::SetPlayerIndex_Implementation(int index)
{
	PlayerIndex = index;
}

int AAIPlayerPawn::GetTeamIndex_Implementation()
{
	return TeamIndex;
}

void AAIPlayerPawn::SetTeamIndex_Implementation(int index)
{
	TeamIndex = index;
}

AActor* AAIPlayerPawn::GetPlayerStateActor_Implementation()
{
	return this;
}

void AAIPlayerPawn::GameEnded_Implementation(MatchResult Result)
{

}

void AAIPlayerPawn::SpawnUnits_Implementation()
{
	TArray<AActor*> SpawnedUnits = TestSpawner->SpawnTestUnits();
}

int AAIPlayerPawn::GetAvailableTickets_Implementation()
{
	return Tickets;
}

void AAIPlayerPawn::SetInitialTickets_Implementation(int InitialTickets)
{
	Tickets = InitialTickets;
}

bool AAIPlayerPawn::TryConsumeTickets_Implementation(int NumberOfTickets)
{
	if (Tickets >= NumberOfTickets)
	{
		Tickets -= NumberOfTickets;
		return true;
	}
	return false;
}

void AAIPlayerPawn::RespawnFormation_Implementation(AActor* Formation)
{
	if (TryConsumeTickets_Implementation(IFormationInterface::Execute_GetFormationCost(Formation)))
	{
		// Respawn Formation
		AAdBellumGameMode* GameMode = Cast<AAdBellumGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
		check(GameMode);
		IFormationInterface::Execute_RespawnFormation(Formation, GameMode->GetDefaultSpawnArea(TeamIndex));
	}
	else
	{
		PlayerDiedDelegate.ExecuteIfBound(TeamIndex);
	}
}