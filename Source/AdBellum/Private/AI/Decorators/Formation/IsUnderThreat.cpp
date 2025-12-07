// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Decorators/Formation/IsUnderThreat.h"
#include "AIController.h"
#include "Formation/FormationInterface.h"


bool UIsUnderThreat::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (APawn* OwningPawn = AIController->GetPawn())
		{
			return true;
		}
	}
	return true;
}
