// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Decorators/HasAmmo.h"
#include "AIController.h"
#include "OrderSystem/Orderable.h"
#include "Weapon/IWeapon.h"


bool UHasAmmo::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (APawn* OwningPawn = AIController->GetPawn())
		{
			//return true;
			if (UObject* Weapon = IOrderable::Execute_GetWeapon(OwningPawn).GetObject())
			{
				return IIWeapon::Execute_HasAmmo(Weapon);
			}
			return true;
		}
	}
	return true;
}
