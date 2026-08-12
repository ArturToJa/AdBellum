#include "RTSFormationUnitTableWidget.h"
#include "RTSMinimumDataCardWidget.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Unit/BaseUnit.h"
#include "Formation/BaseFormation.h"

void URTSFormationUnitTableWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void URTSFormationUnitTableWidget::InitializeWidget(const TArray<ABaseFormation*>& Formations)
{
    FormationsArray.Empty();
    CardMap.Empty();

    for (ABaseFormation* F : Formations)
    {
        if (IsValid(F))
            FormationsArray.Add(F);
    }

    // default select first formation if any
    if (FormationsArray.Num() > 0)
    {
        SelectedFormation = FormationsArray[0].Get();
    }
    else
    {
        SelectedFormation = nullptr;
    }

    RebuildFormationCards();
    RebuildUnitCards();
}

void URTSFormationUnitTableWidget::Refresh()
{
    // refresh formations
    for (TWeakObjectPtr<ABaseFormation> WeakF : FormationsArray)
    {
        if (ABaseFormation* F = WeakF.Get())
        {
            RefreshFormationCard(F);
        }
    }

    // refresh units for selected formation
    RebuildUnitCards();
}

void URTSFormationUnitTableWidget::RebuildFormationCards()
{
    if (!FormationScrollBox) return;

    FormationScrollBox->ClearChildren();

    int32 ColumnCount = 0;
    for (TWeakObjectPtr<ABaseFormation> WeakF : FormationsArray)
    {
        ABaseFormation* F = WeakF.Get();
        if (!IsValid(F)) continue;

        URTSMinimumDataCardWidget* Card = nullptr;
        UClass* UseClass = CardWidgetClass ? CardWidgetClass.Get() : URTSMinimumDataCardWidget::StaticClass();
        if (UseClass)
        {
            Card = CreateWidget<URTSMinimumDataCardWidget>(GetWorld(), UseClass);
            if (Card)
            {
                Card->BindToFormation(F);
                Card->OnCardClicked.AddDynamic(this, &URTSFormationUnitTableWidget::OnCardClicked);
                FormationScrollBox->AddChild(Card);
                CardMap.Add(TWeakObjectPtr<UObject>(F), Card);
            }
        }

        ColumnCount++;
        if (ColumnCount >= MaxVisibleColumns)
        {
            // scrolling will allow more
        }
    }
}

void URTSFormationUnitTableWidget::RebuildUnitCards()
{
    if (!UnitScrollBox) return;

    UnitScrollBox->ClearChildren();

    if (!SelectedFormation.IsValid()) return;

    // attempt to get units from formation
    TArray<APawn*> Units;
    // Use interface method GetUnitsInFormation if available
    if (SelectedFormation.IsValid())
    {
        Units = SelectedFormation->GetUnitsInFormation_Implementation();
    }

    for (APawn* Pawn : Units)
    {
        ABaseUnit* Unit = Cast<ABaseUnit>(Pawn);
        if (!IsValid(Unit)) continue;

        URTSMinimumDataCardWidget* Card = nullptr;
        UClass* UseClass = CardWidgetClass ? CardWidgetClass.Get() : URTSMinimumDataCardWidget::StaticClass();
        if (UseClass)
        {
            Card = CreateWidget<URTSMinimumDataCardWidget>(GetWorld(), UseClass);
            if (Card)
            {
                Card->BindToUnit(Unit);
                Card->OnCardClicked.AddDynamic(this, &URTSFormationUnitTableWidget::OnCardClicked);
                UnitScrollBox->AddChild(Card);
                CardMap.Add(TWeakObjectPtr<UObject>(Unit), Card);
            }
        }
    }
}

void URTSFormationUnitTableWidget::RefreshFormationCard(ABaseFormation* Formation)
{
    if (!Formation) return;
    if (URTSMinimumDataCardWidget** Found = CardMap.Find(TWeakObjectPtr<UObject>(Formation)))
    {
        URTSMinimumDataCardWidget* Card = *Found;
        if (IsValid(Card))
            Card->BindToFormation(Formation);
    }
}

void URTSFormationUnitTableWidget::RefreshUnitCard(ABaseUnit* Unit)
{
    if (!Unit) return;
    if (URTSMinimumDataCardWidget** Found = CardMap.Find(TWeakObjectPtr<UObject>(Unit)))
    {
        URTSMinimumDataCardWidget* Card = *Found;
        if (IsValid(Card))
            Card->BindToUnit(Unit);
    }
}

void URTSFormationUnitTableWidget::OnCardClicked(UObject* BoundObject)
{
    if (!BoundObject) return;

    if (ABaseFormation* F = Cast<ABaseFormation>(BoundObject))
    {
        // update selection
        SelectedFormation = F;
        // rebuild units
        RebuildUnitCards();
        OnFormationClicked.Broadcast(F);

        // update visual selected state
        for (auto& Pair : CardMap)
        {
            if (Pair.Value && Pair.Key.IsValid())
            {
                UObject* Obj = Pair.Key.Get();
                URTSMinimumDataCardWidget* Card = Pair.Value;
                if (Obj == SelectedFormation.Get())
                    Card->SetSelected(true);
                else
                    Card->SetSelected(false);
            }
        }

        return;
    }

    if (ABaseUnit* U = Cast<ABaseUnit>(BoundObject))
    {
        SelectedUnit = U;
        OnUnitClicked.Broadcast(U);

        // update visual selected state for unit cards
        for (auto& Pair : CardMap)
        {
            if (Pair.Value && Pair.Key.IsValid())
            {
                UObject* Obj = Pair.Key.Get();
                URTSMinimumDataCardWidget* Card = Pair.Value;
                if (Obj == SelectedUnit.Get())
                    Card->SetSelected(true);
                else if (Cast<ABaseUnit>(Obj))
                    Card->SetSelected(false);
            }
        }

        return;
    }
}
