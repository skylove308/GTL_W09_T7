#include "AnimSequenceBase.h"

UAnimDataModel* UAnimSequenceBase::GetDataModel() const
{
    return DataModel;
}

void UAnimSequenceBase::SetAnimDataModel(UAnimDataModel* InDataModel)
{
    DataModel = InDataModel;
}
