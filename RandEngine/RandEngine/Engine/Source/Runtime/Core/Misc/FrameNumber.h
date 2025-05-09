#pragma once
#include "HAL/PlatformType.h"

struct FFrameNumber
{
    constexpr FFrameNumber()
    : Value(0)
    {}

    /**
     * Implicit construction from a signed integer frame number, whilst disallowing any construction from other types.
     */
    constexpr FFrameNumber(int32 InValue)
        : Value(InValue)
    {}

    friend FFrameNumber operator+(FFrameNumber A, FFrameNumber B) { return FFrameNumber(A.Value + B.Value); }
    friend FFrameNumber operator-(FFrameNumber A, FFrameNumber B) { return FFrameNumber(A.Value - B.Value); }
    
    /**
     * The value of the frame number
     */
    int32 Value;
};
