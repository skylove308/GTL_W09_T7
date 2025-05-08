#pragma once
#include "SimpleCameraShakePattern.h"

enum class EInitialWaveOscillatorOffsetType : uint8
{
    /** Start with random offset (default). */
    Random,
    /** Start with zero offset. */
    Zero
};

struct FWaveOscillator
{
    /** Amplitude of the sinusoidal oscillation. */
    float Amplitude;

    /** Frequency of the sinusoidal oscillation. */
    float Frequency;
	
    /** Creates a new wave oscillator. */
    FWaveOscillator()
        : Amplitude(1.f)
        , Frequency(1.f)
    {}

    /** Sets the initial offset and returns the initial value of the oscillator */
    float Initialize(float& OutInitialOffset) const;

    /** Advances the oscillation time and returns the current value */
    float Update(float DeltaTime, float AmplitudeMultiplier, float FrequencyMultiplier, float& InOutCurrentOffset) const;
};

class UWaveOscillatorCameraShakePattern : public USimpleCameraShakePattern
{
    DECLARE_CLASS(UWaveOscillatorCameraShakePattern, USimpleCameraShakePattern)
    
public:
    UWaveOscillatorCameraShakePattern();

public:

	/** Amplitude multiplier for all location oscillation */
	float LocationAmplitudeMultiplier = 1.f;

	/** Frequency multiplier for all location oscillation */
	float LocationFrequencyMultiplier = 1.f;

	/** Oscillation in the X axis. */
	FWaveOscillator X;

	/** Oscillation in the Y axis. */
	FWaveOscillator Y;

	/** Oscillation in the Z axis. */
	FWaveOscillator Z;

	/** Amplitude multiplier for all rotation oscillation */
	float RotationAmplitudeMultiplier = 1.f;

	/** Frequency multiplier for all rotation oscillation */
	float RotationFrequencyMultiplier = 1.f;

	/** Pitch oscillation. */
	FWaveOscillator Pitch;

	/** Yaw oscillation. */
	FWaveOscillator Yaw;

	/** Roll oscillation. */
	FWaveOscillator Roll;

	/** FOV oscillation. */
	FWaveOscillator FOV;

private:

	// UCameraShakePattern interface
	virtual void StartShakePatternImpl() override;
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) override;
	
	void UpdateOscillators(float DeltaTime, FCameraShakePatternUpdateResult& OutResult);

private:

	/** Initial sinusoidal offset for location oscillation. */
	FVector InitialLocationOffset;
	/** Current sinusoidal offset for location oscillation. */
	FVector CurrentLocationOffset;

	/** Initial sinusoidal offset for rotation oscillation. */
	FVector InitialRotationOffset;
	/** Current sinusoidal offset for rotation oscillation. */
	FVector CurrentRotationOffset;

	/** Initial sinusoidal offset for FOV oscillation */
	float InitialFOVOffset;
	/** Current sinusoidal offset for FOV oscillation */
	float CurrentFOVOffset;
};
