// Fill out your copyright notice in the Description page of Project Settings.


#include "IsAggresiveOrder.h"
#include "AIController.h"
#include "OrderSystem/OrdersManager.h"

bool UIsAggresiveOrder::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		return IOrderable::Execute_GetOrdersManagerComponent(AIController)->ShouldAttackEnemies();
	}
	return false;
}