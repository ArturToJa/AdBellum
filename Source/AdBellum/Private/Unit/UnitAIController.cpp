// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit/UnitAIController.h"
/*
void AUnitAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
    APawn* const MyPawn = GetPawn();
    if (MyPawn)
    {
        FRotator NewControlRotation = GetControlRotation();

        // Look toward focus
        const FVector FocalPoint = GetFocalPoint();
        if (FAISystem::IsValidLocation(FocalPoint))
        {
            NewControlRotation = (FocalPoint - MyPawn->GetPawnViewLocation()).Rotation();
        }
        else if (bSetControlRotationFromPawnOrientation)
        {
            NewControlRotation = MyPawn->GetActorRotation();
        }

        // We override this so flying units can go up and down
        //// Don't pitch view unless looking at another pawn
        //if (NewControlRotation.Pitch != 0 && Cast<APawn>(GetFocusActor()) == nullptr)
        //{
        //  NewControlRotation.Pitch = 0.f;
        //}

        SetControlRotation(NewControlRotation);

        if (bUpdatePawn)
        {
            const FRotator CurrentPawnRotation = MyPawn->GetActorRotation();

            if (CurrentPawnRotation.Equals(NewControlRotation, 1e-3f) == false)
            {
                MyPawn->FaceRotation(NewControlRotation, DeltaTime);
            }
        }
    }
}*/

void AUnitAIController::DecodeStrengthAndFlags(float EncodedValue)
{
	uint32 Packed;
	FMemory::Memcpy(&Packed, &EncodedValue, sizeof(float));

	// Extract
	flagsSight = Packed & 0b00111111;
	uint32 StrengthQuantized = (Packed >> 6) & 0x3FF; // 10 bitów

	// Dequantize
	strengthSight = (float)StrengthQuantized / 1023.0f;

	bool bFlag0 = (flagsSight & (1 << 0)) != 0;
	bool bFlag1 = (flagsSight & (1 << 1)) != 0;
	bool bFlag2 = (flagsSight & (1 << 2)) != 0;
	bool bFlag3 = (flagsSight & (1 << 3)) != 0;
	bool bFlag4 = (flagsSight & (1 << 4)) != 0;
	bool bFlag5 = (flagsSight & (1 << 5)) != 0;
	FString string = "";
	string += bFlag0 ? "1" : "0";
	string += bFlag1 ? "1" : "0";
	string += bFlag2 ? "1" : "0";
	string += bFlag3 ? "1" : "0";
	string += bFlag4 ? "1" : "0";
	string += bFlag5 ? "1" : "0";

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
		FString::Printf(TEXT("Decoded Sight Strength: %f, Flags: %s"), strengthSight, *string));
}