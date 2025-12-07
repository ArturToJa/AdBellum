// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Decorators/ShouldTakeCover.h"
#include "AIController.h"
#include "OrderSystem/Orderable.h"
#include "ITargetable.h"


bool UShouldTakeCover::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (APawn* OwningPawn = AIController->GetPawn())
		{
			return IITargetable::Execute_IsTargetedBy(OwningPawn).Num() > 2;
		}
	}
	return false;
}
