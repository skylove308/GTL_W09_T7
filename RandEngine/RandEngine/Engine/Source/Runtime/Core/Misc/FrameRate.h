#pragma once
#include "FrameTime.h"
#include "HAL/PlatformType.h"

// 간단하게만 가져옴.
struct FFrameRate
{    
    /**
     * Default construction to a frame rate of 60000 frames per second (0.0166 ms)
     */
    FFrameRate()
        : Numerator(60000), Denominator(1)
    {}

    FFrameRate(uint32 InNumerator, uint32 InDenominator)
        : Numerator(InNumerator), Denominator(InDenominator)
    {}

    /**
     * The numerator of the framerate represented as a number of frames per second (e.g. 60 for 60 fps)
     */
    int32 Numerator; 

    /**
     * The denominator of the framerate represented as a number of frames per second (e.g. 1 for 60 fps)
     */
    int32 Denominator;

    	/**
	 * Verify that this frame rate is valid to use
	 */
	bool IsValid() const
	{
		return Denominator > 0;
	}

	/**
	 * Get the decimal representation of this framerate's interval
	 * 
	 * @return The time in seconds for a single frame under this frame rate
	 */
	double AsInterval() const;

	/**
	 * Get the decimal representation of this framerate
	 * 
	 * @return The number of frames per second
	 */
	double AsDecimal() const;

    /**
     * Convert the specified frame number to a floating-point number of seconds based on this framerate
     * 
     * @param FrameNumber         The frame number to convert
     * @return The number of seconds that the specified frame number represents
     */
    double AsSeconds(FFrameTime FrameNumber) const;
    
	/**
	 * Check whether this frame rate is a multiple of another
	 */
	bool IsMultipleOf(FFrameRate Other) const;

	/**
	 * Check whether this frame rate is a factor of another
	 */
	bool IsFactorOf(FFrameRate Other) const;

	/**
	 * Get the reciprocal of this frame rate
	 */
	FFrameRate Reciprocal() const
	{
		return FFrameRate(Denominator, Numerator);
	}

	friend inline bool operator==(const FFrameRate& A, const FFrameRate& B)
	{
		return A.Numerator == B.Numerator && A.Denominator == B.Denominator;
	}

	friend inline bool operator!=(const FFrameRate& A, const FFrameRate& B)
	{
		return A.Numerator != B.Numerator || A.Denominator != B.Denominator;
	}

	friend inline FFrameRate operator*(FFrameRate A, FFrameRate B)
	{
		return FFrameRate(A.Numerator * B.Numerator, A.Denominator * B.Denominator);
	}

	friend inline FFrameRate operator/(FFrameRate A, FFrameRate B)
	{
		return FFrameRate(A.Numerator * B.Denominator, A.Denominator * B.Numerator);
	}
};

inline double FFrameRate::AsInterval() const
{
	return double(Denominator) / double(Numerator);
}

inline double FFrameRate::AsDecimal() const
{
	return double(Numerator) / double(Denominator);
}

inline double FFrameRate::AsSeconds(FFrameTime FrameTime) const
{
    const int64  IntegerPart  = FrameTime.GetFrame().Value * int64(Denominator);
    const double FloatPart    = FrameTime.GetSubFrame()    * double(Denominator);

    return (double(IntegerPart) + FloatPart) / Numerator;
}

inline bool FFrameRate::IsMultipleOf(FFrameRate Other) const
{
	int64 CommonValueA = int64(Numerator) * Other.Denominator;
	int64 CommonValueB = int64(Other.Numerator) * Denominator;

	return CommonValueA <= CommonValueB && CommonValueB % CommonValueA == 0;
}

inline bool FFrameRate::IsFactorOf(FFrameRate Other) const
{
	return Other.IsMultipleOf(*this);
}
