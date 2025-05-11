#pragma once
#include "KeyHandle.h"
#include "HAL/PlatformType.h"

struct FIndexedCurve
{
public:
    FIndexedCurve() { }

    virtual ~FIndexedCurve() { }

public:
    /** Get number of keys in curve. */
    virtual int32 GetNumKeys() const = 0;

    /** Move a key to a new time. This may change the index of the key, so the new key index is returned. */
    virtual void SetKeyTime(FKeyHandle KeyHandle, float NewTime) = 0;

    /** Get the time for the Key with the specified index. */
    virtual float GetKeyTime(FKeyHandle KeyHandle) const = 0;

    mutable FKeyHandleMap KeyHandlesToIndices;
};
