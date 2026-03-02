// Fill out your copyright notice in the Description page of Project Settings.


#include "HasTarget.h"
#include "AIController.h"

bool UHasTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		return AIController->GetFocusActor() != nullptr;
	}
	return false;
}