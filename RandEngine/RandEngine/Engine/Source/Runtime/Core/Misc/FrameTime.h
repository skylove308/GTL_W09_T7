#pragma once
#include "FrameNumber.h"
#include "HAL/PlatformType.h"

class FFrameTime
{
    inline static const float MaxSubframe = 10;

public:
    /**
     * Default constructor initializing to zero
     */
    FFrameTime();

    /**
     * Implicit construction from a single integer, while disallowing implicit conversion from any other numeric type
     */
    FFrameTime(int32 InFrameNumber);

    /**
     * Implicit construction from a type-safe frame number
     */
    FFrameTime(FFrameNumber InFrameNumber);

    /**
     * Construction from a frame number and a sub frame
     */
    FFrameTime(FFrameNumber InFrameNumber, float InSubFrame);

    /**
     * Assignment from a type-safe frame number
     */
    FFrameTime& operator=(FFrameNumber InFrameNumber);


    /**
     * Access this time's frame number
     */
    FORCEINLINE FFrameNumber GetFrame() const
    {
        return FrameNumber;
    }

    /**
     * Access this time's sub frame
     */
    FORCEINLINE float GetSubFrame() const
    {
        return SubFrame;
    }

    /**
     * Return the first frame number less than or equal to this frame time
     */
    FFrameNumber FloorToFrame() const;

    /**
     * Return the next frame number greater than or equal to this frame time
     */
    FFrameNumber CeilToFrame() const;

    /**
     * Round to the nearest frame number
     */
    FFrameNumber RoundToFrame() const;

    /**
     * Retrieve a decimal representation of this frame time
     * Sub frames are always added to the current frame number, so for negative frame times, a time of -10 [sub frame 0.25] will yield a decimal value of -9.75.
     */
    double AsDecimal() const;

    /**
     * Convert a decimal representation to a frame time
     * Note that sub frames are always positive, so negative decimal representations result in an inverted sub frame and floored frame number
     */
    static FFrameTime FromDecimal(double InDecimalFrame);

    FFrameNumber FrameNumber;
    
private:

    /** Must be 0.f <= SubFrame < 1.f */
    float SubFrame;
};
