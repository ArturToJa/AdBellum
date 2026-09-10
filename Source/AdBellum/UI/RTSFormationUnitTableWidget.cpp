#include "RTSFormationUnitTableWidget.h"
#include "RTSUnitCardWidget.h"
#include "RTSFormationCardWidget.h"
#include "Components/ScrollBox.h"
#include "Unit/BaseUnit.h"
#include "Formation/BaseFormation.h"

void URTSFormationUnitTableWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void URTSFormationUnitTableWidget::InitializeWidget(const TArray<ABaseFormation*>& Formations)
{
    TArray<UWidget*> FormationWidgets = FormationScrollBox->GetAllChildren();
    int MaxUnitWidgets = MaxVisibleColumns;

    for(int Index = 0; Index < Formations.Num(); ++Index)
    {
        ABaseFormation* Formation = Formations[Index];
        int CurrentNumberOfUnits = Formation->GetUnitsInFormation_Implementation().Num();
        if (CurrentNumberOfUnits > MaxUnitWidgets)
        {
            MaxUnitWidgets = CurrentNumberOfUnits;
        }
        if (Formation == nullptr) continue;
        if (FormationWidgets.Num() > Index)
        {
            UWidget* FormationWidget = FormationWidgets[Index];
            URTSFormationCardWidget* CastedWidget = Cast<URTSFormationCardWidget>(FormationWidget);
            CastedWidget->InitializeWidget(Formation);
        }
        else
        {
            UWidget* FormationWidget = CreateWidget(this, FormationCardWidgetClass, TEXT("FormationCardWidget"));
            URTSFormationCardWidget* CastedWidget = Cast<URTSFormationCardWidget>(FormationWidget);
            FormationScrollBox->AddChild(FormationWidget);
            CastedWidget->InitializeWidget(Formation);
        }
    }

    int UnitWidgetsNeeded = MaxUnitWidgets - (UnitScrollBox->GetAllChildren().Num() + UnitCardWidgetsBuffer.Num());
    for (int i = 0; i < UnitWidgetsNeeded; ++i)
    {
        UWidget* UnitWidget = CreateWidget(this, UnitCardWidgetClass, TEXT("UnitCardWidget"));
        UnitCardWidgetsBuffer.Add(UnitWidget);
    }

    FormationWidgets = FormationScrollBox->GetAllChildren();
	for (int i = 0; i < FormationWidgets.Num(); ++i)
    {
		UWidget* Widget = FormationWidgets[i];
        if(URTSFormationCardWidget* CardWidget = Cast<URTSFormationCardWidget>(Widget))
        {
			CardWidget->OnFormationCardClicked.BindUObject(this, &URTSFormationUnitTableWidget::OnFormationClicked);
		}
    }
    TArray<UWidget*> UnitWidgets = UnitScrollBox->GetAllChildren();
    for (int i = 0; i < UnitWidgets.Num(); ++i)
    {
        UWidget* Widget = UnitWidgets[i];
        if (URTSUnitCardWidget* CardWidget = Cast<URTSUnitCardWidget>(Widget))
        {
            CardWidget->OnUnitCardClicked.BindUObject(this, &URTSFormationUnitTableWidget::OnUnitClicked);
        }
    }
}

void URTSFormationUnitTableWidget::OnFormationClicked(ABaseFormation* Formation, int InIndex)
{
    UE_LOG(LogTemp, Warning, TEXT("[DEBUG] OnFormationClicked: Formation=%s"), Formation ? *Formation->GetName() : TEXT("nullptr"));
    int ScrollBoxNum = UnitScrollBox->GetAllChildren().Num();
    TArray<APawn*> UnitsInFormation = Formation->GetUnitsInFormation_Implementation();
    int FormationUnitsNum = UnitsInFormation.Num();
    FormationUnitsNum = FMath::Max(FormationUnitsNum, MaxVisibleColumns);
    if (ScrollBoxNum < FormationUnitsNum)
    {
        if (UnitCardWidgetsBuffer.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("[DEBUG] OnFormationClicked: EARLY RETURN - UnitCardWidgetsBuffer empty. ScrollBoxNum=%d FormationUnitsNum=%d"), ScrollBoxNum, FormationUnitsNum);
            return;
        }
        for (int i = 0; i < FormationUnitsNum - ScrollBoxNum; ++i)
        {
            UWidget* BufferedWidget = UnitCardWidgetsBuffer[0];
            UnitCardWidgetsBuffer.RemoveAtSwap(0);
            UnitScrollBox->AddChild(BufferedWidget);
        }
    }
    else if (ScrollBoxNum > FormationUnitsNum)
    {
        for (int i = 0; i < ScrollBoxNum - FormationUnitsNum; ++i)
        {
            int UnitIndexToRemove = UnitScrollBox->GetAllChildren().Num() - 1;
            UWidget* BufferedWidget = UnitScrollBox->GetChildAt(UnitIndexToRemove);
            if (!UnitScrollBox->RemoveChildAt(UnitIndexToRemove))
            {
                UE_LOG(LogTemp, Warning, TEXT("[DEBUG] OnFormationClicked: EARLY RETURN - RemoveChildAt failed"));
                return;
            }
            UnitCardWidgetsBuffer.Add(BufferedWidget);
        }
    }
    for(int Index = 0; Index < UnitsInFormation.Num(); ++Index)
    {
        APawn* Unit = UnitsInFormation[Index];
        UWidget* UnitWidget = UnitScrollBox->GetChildAt(Index);
        URTSUnitCardWidget* CastedWidget = Cast<URTSUnitCardWidget>(UnitWidget);
        CastedWidget->InitializeWidget(Unit);
    }
    TArray<ABaseFormation*> Formations;
    Formations.Add(Formation);
    UE_LOG(LogTemp, Warning, TEXT("[DEBUG] OnFormationClicked: Broadcasting, IsBound=%d"), OnFormationClickedDelegate.IsBound());
    OnFormationClickedDelegate.Broadcast(Formations);
}

void URTSFormationUnitTableWidget::OnUnitClicked(ABaseUnit* Unit, int Index)
{
    OnUnitClickedDelegate.Broadcast(Unit);
}