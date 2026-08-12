// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interfaces/Selectable.h"
#include "OrderSystem/Orderable.h"
#include "Formation/Formable.h"
#include "Formation/FormationInterface.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "Components/BottleneckAssignmentComponent.h"
#include "BaseFormation.generated.h"

class UOrdersManager;
class UNetworkComponent;

UCLASS()
class ADBELLUM_API ABaseFormation : public APawn, public IOrderable, public IFormable, public IFormationInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABaseFormation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UOrdersManager> OrdersManagerComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNetworkComponent> NetworkComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBottleneckAssignmentComponent> BottleneckAssignmentComponent;

	UPROPERTY(EditDefaultsOnly, Category = "EQS")
	UEnvQuery* FindCoverQuery;


	// Selectable
	virtual class UOrdersManager* GetOrdersManagerComponent_Implementation() override;
	virtual void SetSelection_Implementation(bool bIsSelected) override;
	virtual AActor* GetOwningPlayer_Implementation() override;
	virtual int32 GetTeamIndex_Implementation() override;
	virtual void SetOwningPlayer_Implementation(AActor* Player) override;
	virtual void SetTeamIndex_Implementation(int32 Index) override;

	// Orderable
	virtual void Stop_Implementation(FVector TargetPosition) override;
	virtual void MoveOrder_Implementation(FVector TargetPosition) override;
	virtual void AttackTarget_Implementation(UObject* TargetObject) override;
	virtual void AttackLocation_Implementation(FVector TargetPosition) override;

	// IFormationInterface
	virtual TMap<AActor*, int> GetEnemiesInSight_Implementation() override;
	virtual TMap<AActor*, FVector> GetLastKnownPosition_Implementation() override;
	virtual TArray<APawn*> GetUnitsInFormation_Implementation() override;
	virtual void AddUnitToFormation_Implementation(APawn* UnitToAdd) override;
	virtual void OnEnemyInSightChanged_Implementation(AActor* EnemyUnit, bool IsVisible) override;
	virtual void OnNoiseHeard_Implementation(AActor* NoiseSource) override;
	virtual int GetFormationCost_Implementation() override;
	virtual void RespawnFormation_Implementation(ABaseSpawnArea* SpawnArea) override;
	virtual AActor* GetFirstActorOfInterest_Implementation() override;

	void SetActors(TArray<APawn*> Actors);
	APawn* GetUnitForPossesion();

	template<OrderEnum T>
	void PerformOrder(AActor* TargetUnit, FVector TargetPosition);
	template<OrderEnum T>
	void PerformOrder(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition);

	UFUNCTION(BlueprintCallable)
	void OrderUnits(OrderEnum OrderType, AActor* TargetUnit, FVector TargetPosition);
	UFUNCTION(BlueprintCallable)
	void OrderUnit(APawn* UnitToOrder, OrderEnum OrderType, AActor* TargetUnit, FVector TargetPosition);

	UFUNCTION(BlueprintCallable)
	void OnNoiseTimeout(AActor* NoiseSource);

	void CheckFirstActorOfInterest(AActor* CompareActor);

	void EnemyDies(AActor* DyingEnemy);
	void UnitDied(APawn* DyingUnit);

	void SetFormationCost(int Cost);

	void FindCovers(FVector CoverFromLocation);
	void OnCoverQueryFinished(TSharedPtr<FEnvQueryResult> Result);

	void FinalizeFormation();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void ClearCurrentSelection();
	void SetCurrentSelection();

	TArray<APawn*> ActorsInFormation;
private:
	void SetNoiseTimeoutTimer(AActor* EnemyUnit);
	void ClearNoiseTimeoutTimer(AActor* EnemyUnit);
	void SetMiddlePosition(float DistanceThreshold);
	void ClearCoverData();
	TMap<AActor*, int> EnemiesInSight;
	TMap<AActor*, FVector> LastKnownPosition;
	TMap<AActor*, FTimerHandle> LastPositionTimerMap;
	UPROPERTY(Replicated)
	int32 TeamIndex;
	UPROPERTY(Replicated)
	AActor* OwningPlayer;
	int AliveUnits = 0;
	int FormationCostInTickets = 0;

	AActor* FirstActorOfInterest = nullptr;
	TArray<FVector> CoverLocations;
};
