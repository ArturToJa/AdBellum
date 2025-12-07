// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Decorators/HasOrders.h"
#include "AIController.h"
#include "OrderSystem/OrdersManager.h"
#include "Selectable.h"

bool UHasOrders::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (APawn* OwningPawn = AIController->GetPawn())
		{
			if (class UOrdersManager* Manager = ISelectable::Execute_GetOrdersManagerComponent(OwningPawn))
			{
				return Manager->HasOrders();
			}
			return false;
		}
	}
	return false;
}
