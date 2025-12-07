// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Decorators/Formation/HasHeardNoise.h"
#include "AIController.h"
#include "OrderSystem/Orderable.h"
#include "Weapon/IWeapon.h"
#include "Formation/FormationInterface.h"


bool UHasHeardNoise::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
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
