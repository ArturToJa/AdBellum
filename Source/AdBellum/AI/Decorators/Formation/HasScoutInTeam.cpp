// Fill out your copyright notice in the Description page of Project Settings.


#include "HasScoutInTeam.h"
#include "AIController.h"
#include "Formation/FormationInterface.h"


bool UHasScoutInTeam::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (APawn* OwningPawn = AIController->GetPawn())
		{
			return false;
		}
	}
	return false;
}
