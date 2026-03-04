// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "MoveToLocationOrder.h"
#include "OrderSystem/Orderable.h"
#include "Interfaces/ITargetable.h"
#include "AIController.h"

class ADBELLUM_API TakeCoverOrder : public BaseOrder
{
public:
	TakeCoverOrder(AActor* inTargetUnit, FVector inLocation) : BaseOrder(nullptr, inLocation) {}

	virtual ~TakeCoverOrder() {}

	virtual void Execute() override
	{
		RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, targetPosition));
	}

	virtual void Update() override
	{
		BaseOrder::Update();
		if (!timerHandle.IsValid())
		{
			bIsHiding = true;
			if (!HasSubOrders())
			{
				IOrderable::Execute_DoCrouch(owningController);
				if (!bDetectedCoverHeight)
				{
					FHitResult Hit;

					AActor* FocusActor = owningController->GetFocusActor();

					if (FocusActor)
					{
						TEnumAsByte<ECollisionChannel> TraceChannelProperty = ECC_Visibility;

						FVector TraceStart = owningController->GetPawn()->GetActorLocation();
						FVector TraceEnd = FocusActor->GetActorLocation();

						// You can use FCollisionQueryParams to further configure the query
						// Here we add ourselves to the ignored list so we won't block the trace
						FCollisionQueryParams QueryParams;
						QueryParams.AddIgnoredActor(owningController->GetPawn());
						QueryParams.AddIgnoredActor(FocusActor);

						// To run the query, you need a pointer to the current level, which you can get from an Actor with GetWorld()
						// UWorld()->LineTraceSingleByChannel runs a line trace and returns the first actor hit over the provided collision channel.
						FocusActor->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannelProperty, QueryParams);

						// You can use DrawDebug helpers and the log to help visualize and debug your trace queries.
						DrawDebugLine(FocusActor->GetWorld(), TraceStart, TraceEnd, Hit.bBlockingHit ? FColor::Blue : FColor::Red, false, 5.0f, 0, 10.0f);

						// If the trace hit something, bBlockingHit will be true,
						// and its fields will be filled with detailed info about what was hit
						if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
						{
							bIsHighCover = true;
							UE_LOG(LogTemp, Log, TEXT("Is High Cover"));
						}
						else
						{
							bIsHighCover = false;
							UE_LOG(LogTemp, Log, TEXT("Is Low Cover"));
						}

						bDetectedCoverHeight = true;
					}
				}
				else
				{
					if (bIsHighCover)
					{
						// todo
					}
					else
					{
						//FTimerDelegate timerDelegate;
						//timerDelegate.BindRaw(this, &TakeCoverOrder::ToggleHiding);
						//owningController->GetWorldTimerManager().SetTimer(timerHandle, timerDelegate, 3.0f, true);
					}
				}
			}
		}
	}

	virtual void Finalize() override
	{
		IOrderable::Execute_DoStandUp(owningController);
		owningController->GetWorldTimerManager().PauseTimer(timerHandle);
		owningController->GetWorldTimerManager().ClearTimer(timerHandle);
	}

	virtual bool IsFinished() const override
	{
		if (BaseOrder::IsFinished())
		{
			return true;
		}
		else
		{
			TArray<AActor*> TargetedBy = IITargetable::Execute_IsTargetedBy(owningController->GetPawn());
			return !ITargetable::Execute_IsAlive(owningController->GetPawn()) || TargetedBy.IsEmpty();
		}
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::TakeCover;
	}

	void ToggleHiding()
	{
		if (bIsHiding)
		{
			IOrderable::Execute_DoStandUp(owningController);
			bIsHiding = false;
		}
		else
		{
			IOrderable::Execute_DoCrouch(owningController);
			bIsHiding = true;
		}
	}

private:
	bool bIsHighCover;
	bool bDetectedCoverHeight = false;
	bool bIsHiding = false;
	FTimerHandle timerHandle;
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::TakeCover>
{
	using OrderType = TakeCoverOrder;
};
