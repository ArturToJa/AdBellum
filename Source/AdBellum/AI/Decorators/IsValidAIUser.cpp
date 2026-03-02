// Fill out your copyright notice in the Description page of Project Settings.


#include "IsValidAIUser.h"
#include "AIController.h"
#include "Unit/Selectable.h"

bool UIsValidAIUser::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (APawn* OwningPawn = AIController->GetPawn())
		{
			return ISelectable::Execute_IsAlive(OwningPawn) && !AIController->IsPlayerController();
		}
	}
	return false;
}