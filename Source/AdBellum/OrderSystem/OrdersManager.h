// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OrderSystem/OrderType.h"
#include "Templates/SharedPointer.h"
#include "OrderSystem/OrderSystem.h"
#include "OrdersManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(OrdersManager, Log, All);

USTRUCT(BlueprintType)
struct FOrdersParams
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	OrderEnum OrderType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UObject* TargetObject;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector TargetPosition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsAggressive;

	FOrdersParams(OrderEnum Type, UObject* Object, FVector Position, bool bInIsAggressive)
		: OrderType(Type)
		, TargetObject(Object)
		, TargetPosition(Position)
		, bIsAggressive(bInIsAggressive)
	{}

	FOrdersParams()
		: FOrdersParams(OrderEnum::Stop, nullptr, FVector::ZeroVector, true)
	{}

	FOrdersParams(OrderEnum Type, UObject* Object, FVector Position)
		: FOrdersParams(Type, Object, Position, true)
	{}

	bool operator==(const FOrdersParams& OtherOrder)
	{
		return this->OrderType == OtherOrder.OrderType && this->TargetObject == OtherOrder.TargetObject && this->TargetPosition == OtherOrder.TargetPosition;
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADBELLUM_API UOrdersManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UOrdersManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	TUniquePtr<BaseOrder> CurrentOrder;
	TArray<TUniquePtr<BaseOrder>> OrderQueue;

public:	
	UFUNCTION(BlueprintCallable)
		void RunAILogic();
	UFUNCTION(BlueprintCallable)
		void StopAILogic();
	UFUNCTION(BlueprintCallable)
		void SetStopOrder();
		void ProcessNextOrder();
	UFUNCTION(BlueprintCallable)
		bool ShouldAttackEnemies();
	UFUNCTION(BlueprintCallable)
		void NotifyCurrentOrderCompleted();
	UFUNCTION(BlueprintCallable)
		void NotifyMainOrderCompleted();
		void NotifySubOrderStarted();
	UFUNCTION(BlueprintCallable)
		bool IsCurrentOrderOfType(OrderEnum Type);
	UFUNCTION(BlueprintCallable)
		bool IsOrderOfType(OrderEnum Type);
		void PerformOrder(TUniquePtr<BaseOrder> OrderToPerform);
		void AddOrder(TUniquePtr<BaseOrder> OrderToPerform, bool bIsQueued);
	UFUNCTION(BlueprintCallable)
		bool IsRunningSubOrders();
	UFUNCTION(BlueprintCallable)
		void UpdateOrder();
	UFUNCTION(BlueprintCallable)
		bool HasOrders();
	UFUNCTION(BlueprintCallable)
		void MoveOrder(FVector TargetPosition);
	UFUNCTION(BlueprintCallable)
		void HideBehindCover(FVector TargetPosition);
	UFUNCTION(BlueprintCallable)
		OrderEnum GetOrderType();
	UFUNCTION(BlueprintCallable)
		OrderEnum GetCurrentOrderType();
	UFUNCTION(BlueprintCallable)
		void BP_AddOrder(OrderEnum OrderType, bool bIsQueued, AActor* TargetObject, FVector TargetPosition);
	void SetAsNonAggressiveOrder();
	void SetAsNonAggressiveOrderInQueue();
	BaseOrder* GetOrder();
	BaseOrder* GetSubOrder();

	FSimpleDelegate OnHUDNotify;
};
