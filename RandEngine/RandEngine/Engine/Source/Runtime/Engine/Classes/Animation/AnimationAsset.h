#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UAnimationAsset : public UObject
{
    DECLARE_CLASS(UAnimationAsset, UObject)
public:
    UAnimationAsset() = default;
    virtual ~UAnimationAsset() override = default;
};
