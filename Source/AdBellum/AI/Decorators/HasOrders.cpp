// Fill out your copyright notice in the Description page of Project Settings.


#include "HasOrders.h"
#include "AIController.h"
#include "OrderSystem/OrdersManager.h"

bool UHasOrders::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (class UOrdersManager* Manager = IOrderable::Execute_GetOrdersManagerComponent(AIController))
		{
			return Manager->HasOrders();
		}
		return false;
	}
	return false;
}
