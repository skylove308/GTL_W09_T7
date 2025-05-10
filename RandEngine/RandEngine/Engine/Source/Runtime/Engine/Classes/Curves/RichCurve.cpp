#include "RichCurve.h"

FKeyHandle FRichCurve::AddKey(const float InTime, const float InValue, const bool bUnwindRotation, FKeyHandle NewHandle )
{
    int32 Index = 0;
    if (Keys.Num() == 0 || Keys.Last().Time < InTime)
    {
        Index = Keys.Num();
    }
    else
    {
        for (; Index < Keys.Num() && Keys[Index].Time < InTime; ++Index);
    }
    // Keys.Insert(FRichCurveKey(InTime, InValue), Index);

    // If we were asked to treat this curve as a rotation value and to unwindow the rotation, then
    // we'll look at the previous key and modify the key's value to use a rotation angle that is
    // continuous with the previous key while retaining the exact same rotation angle, if at all necessary
    if( Index > 0 && bUnwindRotation )
    {
        const float OldValue = Keys[ Index - 1 ].Value;
        float NewValue = Keys[ Index ].Value;

        while( NewValue - OldValue > 180.0f )
        {
            NewValue -= 360.0f;
        }
        while( NewValue - OldValue < -180.0f )
        {
            NewValue += 360.0f;
        }

        Keys[Index].Value = NewValue;
    }
	
    // KeyHandlesToIndices.Add(NewHandle, Index);

    return NewHandle;
}

int32 FRichCurve::GetNumKeys() const
{
    return 0;
}

void FRichCurve::SetKeyTime(FKeyHandle KeyHandle, float NewTime)
{
}

float FRichCurve::GetKeyTime(FKeyHandle KeyHandle) const
{
    return 0;
}

void FRichCurve::DeleteKey(FKeyHandle KeyHandle)
{
}

float FRichCurve::Eval(float InTime, float InDefaultValue) const
{
    return 0;
}

// float FRichCurve::Eval(float InTime, float InDefaultValue) const
// {
// }
