// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Decorators/Formation/SeeEnemy.h"
#include "AIController.h"
#include "Formation/FormationInterface.h"


bool USeeEnemy::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (APawn* OwningPawn = AIController->GetPawn())
		{
			TMap<AActor*, int> EnemiesInSight = IFormationInterface::Execute_GetEnemiesInSight(OwningPawn);
			return !EnemiesInSight.IsEmpty();
		}
	}
	return true;
}
