// Fill out your copyright notice in the Description page of Project Settings.


#include "IsAggresiveOrder.h"
#include "AIController.h"
#include "OrderSystem/OrdersManager.h"
#include "Unit/Selectable.h"

bool UIsAggresiveOrder::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (APawn* OwningPawn = AIController->GetPawn())
		{
			return ISelectable::Execute_GetOrdersManagerComponent(OwningPawn)->ShouldAttackEnemies();
		}
	}
	return false;
}