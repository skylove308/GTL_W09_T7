#pragma once
#include "Container/Map.h"
#include "Delegates/Delegate.h"
#include "Delegates/DelegateCombination.h"
#include "Math/Vector.h"

DECLARE_MULTICAST_DELEGATE(FVoidDelegate)
DECLARE_MULTICAST_DELEGATE_OneParam(FOneFloatDelegate, const float&)
DECLARE_MULTICAST_DELEGATE_OneParam(FOneVectorDelegate, const FVector&)

class FEventManager
{
public:
    FEventManager() = default;
    ~FEventManager() = default;

    FEventManager(const FEventManager&) = delete;

    TMap<FString, FVoidDelegate> Delegates;
    TMap<FString, FOneFloatDelegate> OneFloatDelegates;
    TMap<FString, FOneVectorDelegate> OneVectorDelegates;
};
