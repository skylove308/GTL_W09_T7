#include "FrameTime.h"

#include "Math/MathUtility.h"

FFrameTime::FFrameTime()
	: FrameNumber(0), SubFrame(0.f)
{}


FFrameTime::FFrameTime(int32 InFrameNumber)
	: FrameNumber(InFrameNumber), SubFrame(0.f)
{}


FFrameTime::FFrameTime(FFrameNumber InFrameNumber)
	: FrameNumber(InFrameNumber), SubFrame(0.f)
{}


FFrameTime::FFrameTime(FFrameNumber InFrameNumber, float InSubFrame)
	: FrameNumber(InFrameNumber), SubFrame(InSubFrame)
{
	// Hack to ensure that SubFrames are in a sensible range of precision to work around
	// problems with FloorToXYZ returning the wrong thing for very small negative numbers
	SubFrame = FMath::Clamp(SubFrame + 0.5f - 0.5f, 0.f, MaxSubframe);
}


FFrameTime& FFrameTime::operator=(FFrameNumber InFrameNumber)
{
    FrameNumber = InFrameNumber;
    SubFrame    = 0.f;
    return *this;
}

FFrameNumber FFrameTime::FloorToFrame() const
{
    return FrameNumber;
}


FFrameNumber FFrameTime::CeilToFrame() const
{
    return SubFrame == 0.f ? FrameNumber : FrameNumber + 1;
}


FFrameNumber FFrameTime::RoundToFrame() const
{
    return SubFrame < .5f ? FrameNumber : FrameNumber + 1;
}


double FFrameTime::AsDecimal() const
{
    return double(FrameNumber.Value) + SubFrame;
}

FFrameTime FFrameTime::FromDecimal(double InDecimalFrame)
{
    int32 NewFrame = static_cast<int32>(FMath::Clamp(FMath::FloorToDouble(InDecimalFrame), (double)std::_Min_limit<int32>(), (double)std::_Min_limit<int32>()));

    // Ensure fractional parts above the highest sub frame float precision do not round to 0.0
    double Fraction = InDecimalFrame - FMath::FloorToDouble(InDecimalFrame);
    return FFrameTime(NewFrame, FMath::Clamp((float)Fraction, 0.0f, MaxSubframe));
}
