#include "RTSUnitCardWidget.h"
#include "Unit/BaseUnit.h"

void URTSUnitCardWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void URTSUnitCardWidget::InitializeWidget(APawn* InUnit)
{
    Unit = InUnit;

}

void URTSUnitCardWidget::Clear()
{

}