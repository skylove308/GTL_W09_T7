#pragma once
#include "IndexedCurve.h"
#include "Math/MathUtility.h"

struct FRealCurve : public FIndexedCurve
{
public:
    FRealCurve() : FIndexedCurve()
    { }
public:
    /**
  * Add a new key to the curve with the supplied Time and Value. Returns the handle of the new key.
  * 
  * @param	bUnwindRotation		When true, the value will be treated like a rotation value in degrees, and will automatically be unwound to prevent flipping 360 degrees from the previous key 
  * @param  KeyHandle			Optionally can specify what handle this new key should have, otherwise, it'll make a new one
  */
    virtual FKeyHandle AddKey(float InTime, float InValue, const bool bUnwindRotation = false, FKeyHandle KeyHandle = FKeyHandle())
    {
        return FKeyHandle::Invalid();
    }

    virtual void DeleteKey(FKeyHandle KeyHandle) = 0;

    virtual FKeyHandle UpdateOrAddKey(float InTime, float InValue, const bool bUnwindRotation = false, float KeyTimeTolerance = KINDA_SMALL_NUMBER) { return FKeyHandle::Invalid(); }

    FKeyHandle FindKey(float KeyTime, float KeyTimeTolerance = KINDA_SMALL_NUMBER) const;

    /** Evaluate this curve at the specified time */
    virtual float Eval(float InTime, float InDefaultValue = 0.0f) const = 0;
};
