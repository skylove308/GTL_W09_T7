#pragma once
#include "TextComponent.h"

class UTextUUID : public UTextComponent
{
    DECLARE_CLASS(UTextUUID, UTextComponent)

public:
    UTextUUID();

    virtual int CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const override;
    void SetUUID(uint32 UUID);
};
