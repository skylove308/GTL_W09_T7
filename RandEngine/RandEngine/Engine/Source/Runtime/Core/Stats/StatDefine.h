#pragma once

#include "UObject/NameTypes.h"

struct TStatId
{
    FName Name;

    TStatId()
        : Name(NAME_None)
    {
    }

    TStatId(FName Name)
        : Name(Name)
    {
    }

    FName GetName() const
    {
        return Name;
    }

    bool operator==(const TStatId& Other) const
    {
        return Name == Other.Name;
    }

    bool operator!=(const TStatId& Other) const
    {
        return Name != Other.Name;
    }
};
