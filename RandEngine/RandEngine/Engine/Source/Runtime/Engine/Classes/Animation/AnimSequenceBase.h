#pragma once
#include "AnimationAsset.h"
#include "AnimData/AnimDataModel.h"

class UAnimSequenceBase : public UAnimationAsset
{
    DECLARE_CLASS(UAnimSequenceBase, UAnimationAsset)
public:
    UAnimSequenceBase() = default;

    UAnimDataModel* GetDataModel() const;

    // 언리얼에서는 Set이 없음.
    void SetAnimDataModel(UAnimDataModel* InDataModel);


    //TArray<struct FAnimNotifyEvent> Notifies;
    
private:
    UAnimDataModel* DataModel;
};
