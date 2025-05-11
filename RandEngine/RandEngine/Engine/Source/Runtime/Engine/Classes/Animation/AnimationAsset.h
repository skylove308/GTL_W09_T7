#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class USkeleton;

class UAnimationAsset : public UObject
{
    DECLARE_CLASS(UAnimationAsset, UObject)
public:
    UAnimationAsset() = default;
    virtual ~UAnimationAsset() override = default;

    USkeleton* Skeleton;

    USkeleton* GetSkeleton() const { return Skeleton; }
};
