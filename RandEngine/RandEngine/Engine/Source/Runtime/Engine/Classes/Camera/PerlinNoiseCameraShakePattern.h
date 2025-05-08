#pragma once
#include "Camera/Shakes/SimpleCameraShakePattern.h"

struct FPerlinNoiseShaker
{
    /** Amplitude of the perlin noise. */
    float Amplitude;

    /** Frequency of the sinusoidal oscillation. */
    float Frequency;

    /** Creates a new perlin noise shaker. */
    FPerlinNoiseShaker()
        : Amplitude(1.f)
        , Frequency(1.f)
    {
    }

    /** Advances the shake time and returns the current value */
    float Update(float DeltaTime, float AmplitudeMultiplier, float FrequencyMultiplier, float& InOutCurrentOffset) const;
};

class UPerlinNoiseCameraShakePattern : public USimpleCameraShakePattern
{
    DECLARE_CLASS(UPerlinNoiseCameraShakePattern, USimpleCameraShakePattern)

public:
    UPerlinNoiseCameraShakePattern();

public:
    float LocationAmplitudeMultiplier = 1.f;
    
    float LocationFrequencyMultiplier = 1.f;

    FPerlinNoiseShaker X;

    FPerlinNoiseShaker Y;

    FPerlinNoiseShaker Z;

    float RotationAmplitudeMultiplier = 1.f;

    float RotationFrequencyMultiplier = 1.f;

    FPerlinNoiseShaker Pitch;

    FPerlinNoiseShaker Yaw;

    FPerlinNoiseShaker Roll;

    FPerlinNoiseShaker FOV;

private:
    // UCameraShakePattern interface
    virtual void StartShakePatternImpl() override;
    virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
    //virtual void ScrubShakePatternImpl(const FCameraShakePatternScrubParams& Params, FCameraShakePatternUpdateResult& OutResult) override;

    void UpdatePerlinNoise(float DeltaTime, FCameraShakePatternUpdateResult& OutResult);

private:

    /** Initial perlin noise offset for location oscillation. */
    FVector InitialLocationOffset;
    /** Current perlin noise offset for location oscillation. */
    FVector CurrentLocationOffset;

    /** Initial perlin noise offset for rotation oscillation. */
    FVector InitialRotationOffset;
    /** Current perlin noise offset for rotation oscillation. */
    FVector CurrentRotationOffset;

    /** Initial perlin noise offset for FOV oscillation */
    float InitialFOVOffset;
    /** Current perlin noise offset for FOV oscillation */
    float CurrentFOVOffset;
};

