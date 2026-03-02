// Fill out your copyright notice in the Description page of Project Settings.


#include "IsTargetAlive.h"
#include "AIController.h"
#include "Unit/Selectable.h"

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