// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Decorators/IsTargetAlive.h"
#include "AIController.h"
#include "Selectable.h"

bool UIsTargetAlive::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (AActor* TargetActor = AIController->GetFocusActor())
		{
			return ISelectable::Execute_IsAlive(TargetActor);
		}
	}
	return false;
}