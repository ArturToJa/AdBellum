// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "ReloadOrder.h"
#include "Orderable.h"
#include "Weapon/IWeapon.h"
#include "Unit/UnitAIController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Character/ALSInputInterface.h"

class ADBELLUM_API AttackUnitOrder : public BaseOrder
{
public:
	// standardized default delay (can be tuned per-unit later)
	inline static constexpr float DefaultAttackDelay = 3.0f;
	// initial delay on first entry to allow aim/animation to start
	inline static constexpr float InitialEntryDelay = 2.5f;
	// tolerance for when to recompute aim
	inline static constexpr float AimPositionTolerance = 10.0f; // units
	inline static constexpr float AimVelocityTolerance = 10.0f; // units/sec
	// delay before performing aim setup (gives opportunity to batch rapid changes)
	inline static constexpr float AimSetupDelay = 0.05f; // seconds
	// additional small confirmation delay after SetupAim to allow async resources to complete
	inline static constexpr float AimConfirmDelay = 0.1f; // seconds

	AttackUnitOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(inTargetUnit, FVector::ZeroVector) {}

	virtual ~AttackUnitOrder() {}

	virtual void Execute() override
	{
		// Guard against missing owner
		if (!owningUnit)
		{
			UE_LOG(LogTemp, Warning, TEXT("AttackUnitOrder::Execute: owningUnit is null"));
			return;
		}

		// Try to obtain weapon interface & object; WeaponObject may remain null if none available
		TScriptInterface<IIWeapon> Weapon = IOrderable::Execute_GetWeapon(owningUnit);
		WeaponObject = Weapon.GetObject();
		if (!WeaponObject)
		{
			UE_LOG(LogTemp, Warning, TEXT("AttackUnitOrder::Execute: No weapon found on owningUnit"));
		}

		// Initialize standardized delay (can be customized later per unit/weapon)
		// On first entry, use initial entry delay to allow aim animation to start; subsequent resets use DefaultAttackDelay
		if (bFirstEntry)
		{
			AttackDelay = InitialEntryDelay;
			bFirstEntry = false;
		}
		else
		{
			AttackDelay = DefaultAttackDelay;
		}

		// initialize aim tracking
		LastTargetLocation = FVector::ZeroVector;
		LastTargetVelocity = FVector::ZeroVector;
		bAimNeedsSetup = false;
		bAwaitingAimSetup = false;
		bAimConfigured = false;

		if (APawn* PawnUnit = Cast<APawn>(owningUnit))
		{
			if (AController* Ctrl = PawnUnit->GetController())
			{
				AUnitAIController* UnitController = Cast<AUnitAIController>(Ctrl);
				if (UnitController)
				{
					UnitController->SetTarget(Cast<AActor>(targetUnit));
				}
			}
		}

		// If we have a weapon and a valid target, schedule an initial aim setup
		if (WeaponObject)
		{
			if (AActor* TargetActor = Cast<AActor>(targetUnit))
			{
				LastTargetLocation = TargetActor->GetActorLocation();
				LastTargetVelocity = TargetActor->GetVelocity();
				ScheduleAimSetup(TargetActor);
			}
		}
	}

	virtual void Finalize() override
	{
		// Clear any pending timers
		if (UWorld* World = owningUnit ? owningUnit->GetWorld() : nullptr)
		{
			World->GetTimerManager().ClearTimer(AimSetupTimerHandle);
			World->GetTimerManager().ClearTimer(AimConfirmTimerHandle);
		}

		if (APawn* PawnUnit = Cast<APawn>(owningUnit))
		{
			AController* Ctrl = PawnUnit->GetController();
			if (Ctrl)
			{
				AUnitAIController* UnitController = Cast<AUnitAIController>(Ctrl);
				if (UnitController)
				{
					UnitController->ClearTarget();
				}
			}
		}
		if (WeaponObject)
		{
			// ensure weapon object still valid before calling interface
			IIWeapon::Execute_SetupAim(WeaponObject, nullptr);
		}
	}

	virtual bool IsFinished() const override
	{
		return BaseOrder::IsFinished() || !ISelectable::Execute_IsAlive(targetUnit) || WeaponObject == nullptr;
	}

	virtual OrderEnum GetOrderType() const override
	{
		return HasSubOrders() ? subOrder->GetOrderType() : OrderEnum::Attack;
	}
	//
	virtual void Update() override
	{
		BaseOrder::Update();

		// Diagnostic: log basic state
		{
			FString OwnerName = owningUnit ? owningUnit->GetName() : TEXT("<null>");
			FString WeaponName = WeaponObject ? WeaponObject->GetName() : TEXT("<null>");
			FString TargetName = targetUnit ? (Cast<AActor>(targetUnit) ? Cast<AActor>(targetUnit)->GetName() : TEXT("object")) : TEXT("<null>");
			UE_LOG(LogTemp, Verbose, TEXT("AttackUnitOrder::Update enter - Owner=%s Weapon=%s Target=%s"), *OwnerName, *WeaponName, *TargetName);
		}

		// Guard weapon-related calls with a valid object
		bool bUnitReloading = IOrderable::Execute_IsReloading(owningUnit);
		UE_LOG(LogTemp, Verbose, TEXT("AttackUnitOrder: bUnitReloading=%d"), bUnitReloading ? 1 : 0);
		if (bUnitReloading)
		{
			UE_LOG(LogTemp, Warning, TEXT("AttackUnitOrder: Owning unit is reloading, cannot attack"));
			return;
		}

		// Cooldown handling
		if (AttackDelay > 0.0f)
		{
			UE_LOG(LogTemp, Verbose, TEXT("AttackUnitOrder: Waiting cooldown AttackDelay=%f"), AttackDelay);
			AttackDelay -= owningUnit->GetWorld()->GetDeltaSeconds();
			return;
		}

		// Validate target before attempting attack
		AActor* TargetActor = Cast<AActor>(targetUnit);
		if (!TargetActor || !ISelectable::Execute_IsAlive(targetUnit))
		{
			UE_LOG(LogTemp, Warning, TEXT("AttackUnitOrder::Update: target is invalid or dead"));
			return;
		}

		// Only schedule aim setup if target moved significantly or changed velocity
		if (TargetActor)
		{
			const FVector CurrentLocation = TargetActor->GetActorLocation();
			const FVector CurrentVelocity = TargetActor->GetVelocity();

			const bool bPositionChanged = FVector::DistSquared(CurrentLocation, LastTargetLocation) > (AimPositionTolerance * AimPositionTolerance);
			const bool bVelocityChanged = FVector::DistSquared(CurrentVelocity, LastTargetVelocity) > (AimVelocityTolerance * AimVelocityTolerance);

			if (bPositionChanged || bVelocityChanged)
			{
				// update stored values and schedule weapon aim recalculation
				LastTargetLocation = CurrentLocation;
				LastTargetVelocity = CurrentVelocity;
				ScheduleAimSetup(TargetActor);
			}
		}

		// Ensure aim configured before firing
		if (!bAimConfigured)
		{
			// If we're still awaiting aim setup, don't fire yet
			UE_LOG(LogTemp, Verbose, TEXT("AttackUnitOrder: Waiting for aim configuration before firing (bAwaiting=%d bNeedsSetup=%d)"), bAwaitingAimSetup ? 1 : 0, bAimNeedsSetup ? 1 : 0);
			return;
		}

		// Check ammo and perform attack; reset cooldown on successful attack
		bool bHasAmmo = IIWeapon::Execute_HasAmmo(WeaponObject);
		UE_LOG(LogTemp, Verbose, TEXT("AttackUnitOrder: bHasAmmo=%d"), bHasAmmo ? 1 : 0);
		if (bHasAmmo)
		{
			UE_LOG(LogTemp, Verbose, TEXT("AttackUnitOrder: Firing at target"));
			IOrderable::Execute_AttackTarget(owningUnit, targetUnit);
			// reset cooldown after firing - standardized approach
			AttackDelay = DefaultAttackDelay;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AttackUnitOrder: No ammo, scheduling reload suborder"));
			RunSubOrder(MakeUnique<ReloadOrder>(nullptr, FVector::ZeroVector));
		}
	}

	UObject* WeaponObject = nullptr;
	float AttackDelay = DefaultAttackDelay;
	bool bFirstEntry = true;

	// Aim tracking
	FVector LastTargetLocation = FVector::ZeroVector;
	FVector LastTargetVelocity = FVector::ZeroVector;
	bool bAimNeedsSetup = false;
	bool bAwaitingAimSetup = false;
	bool bAimConfigured = false;

	FTimerHandle AimSetupTimerHandle;
	FTimerHandle AimConfirmTimerHandle;
	TWeakObjectPtr<AActor> PendingAimTarget;

	// Schedule an aim setup; coalesces rapid updates and runs after AimSetupDelay
	void ScheduleAimSetup(AActor* TargetActor)
	{
		if (!TargetActor || !WeaponObject)
			return;

		// Cancel existing timers to coalesce
		if (UWorld* World = owningUnit ? owningUnit->GetWorld() : nullptr)
		{
			World->GetTimerManager().ClearTimer(AimSetupTimerHandle);
			World->GetTimerManager().ClearTimer(AimConfirmTimerHandle);
		}

		PendingAimTarget = TargetActor;
		bAwaitingAimSetup = true;
		bAimConfigured = false;

		if (UWorld* World = owningUnit ? owningUnit->GetWorld() : nullptr)
		{
			// create delegate that calls OnAimSetupTimer
			FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this]() {
				this->OnAimSetupTimer();
			});
			World->GetTimerManager().SetTimer(AimSetupTimerHandle, Delegate, AimSetupDelay, false);
		}
	}

	void OnAimSetupTimer()
	{
		bAwaitingAimSetup = false;
		if (!WeaponObject)
			return;
		AActor* Target = PendingAimTarget.IsValid() ? PendingAimTarget.Get() : nullptr;
		if (!Target)
			return;
		// perform aim setup
		bAimNeedsSetup = true;
		IIWeapon::Execute_SetupAim(WeaponObject, Target);
		bAimNeedsSetup = false;
		// schedule confirmation timer to allow async resources to settle before permitting fire
		if (UWorld* World = owningUnit ? owningUnit->GetWorld() : nullptr)
		{
			FTimerDelegate ConfirmDelegate = FTimerDelegate::CreateLambda([this]() {
				this->OnAimConfirmTimer();
			});
			World->GetTimerManager().SetTimer(AimConfirmTimerHandle, ConfirmDelegate, AimConfirmDelay, false);
		}
	}

	void OnAimConfirmTimer()
	{
		// After confirmation delay, allow firing (unless another setup has been scheduled)
		bAimConfigured = true;
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Attack>
{
	using OrderType = AttackUnitOrder;
};
