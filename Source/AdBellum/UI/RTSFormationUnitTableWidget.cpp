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
    for (int i = 0; i < FormationWidgets.Num(); ++i)
    {
		UWidget* Widget = FormationWidgets[i];
        if(URTSFormationCardWidget* CardWidget = Cast<URTSFormationCardWidget>(Widget))
        {
			CardWidget->OnFormationCardClicked.BindRaw(this, &URTSFormationUnitTableWidget::OnFormationClicked);
		}
    }
    TArray<UWidget*> UnitWidgets = UnitScrollBox->GetAllChildren();
    for (int i = 0; i < UnitWidgets.Num(); ++i)
    {
        UWidget* Widget = UnitWidgets[i];
        if (URTSUnitCardWidget* CardWidget = Cast<URTSUnitCardWidget>(Widget))
        {
            CardWidget->OnUnitCardClicked.BindRaw(this, &URTSFormationUnitTableWidget::OnUnitClicked);
        }
    }
}

void URTSFormationUnitTableWidget::OnFormationClicked(ABaseFormation* Formation, int Index)
{
    //OnFormationClicked.Broadcast(Formation);
}

void URTSFormationUnitTableWidget::OnUnitClicked(ABaseUnit* Unit, int Index)
{
    //OnUnitClicked.Broadcast(Unit);
}