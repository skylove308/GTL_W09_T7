
#include "SimpleCameraShakePattern.h"

void USimpleCameraShakePattern::GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const
{
    if (Duration > 0.f)
    {
        OutInfo.Duration = Duration;
    }
    else
    {
        OutInfo.DurationType = ECameraShakeDurationType::Infinite;
    }

    OutInfo.BlendIn = BlendInTime;
    OutInfo.BlendOut = BlendOutTime;
}

void USimpleCameraShakePattern::StartShakePatternImpl()
{
    State.Start(this);
}

bool USimpleCameraShakePattern::IsFinishedImpl() const
{
    return !State.IsPlaying();
}

void USimpleCameraShakePattern::StopShakePatternImpl(bool bImmediately)
{
    State.Stop(bImmediately);
}

void USimpleCameraShakePattern::TeardownShakePatternImpl()
{
    State = FCameraShakeState();
}
