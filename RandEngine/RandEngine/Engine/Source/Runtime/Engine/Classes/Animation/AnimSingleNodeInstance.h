#pragma once

#include "Animation/AnimInstance.h"
#include "UObject/ObjectMacros.h"

class UAnimSingleNodeInstance : public UAnimInstance
{
    DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)

public:
    UAnimSingleNodeInstance();
    ~UAnimSingleNodeInstance();

    void SetAnimationAsset(UAnimationAsset* NewAsset, bool bInIsLooping, float InPlayRate = 1.f);

    void SetLooping(bool bIsLooping);

    void SetPlaying(bool bIsPlaying);

    bool IsPlaying() const;

    virtual void UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion) override;

    void SetUseExternalTime(bool bUse) { bUseExternalTime = bUse; }

    void SetExternalTime(float NewTime) { ExternalTime = NewTime; }

    float GetCurrentAnimationTime() const { return CurrentTime; }
public:
    UAnimationAsset* CurrentAsset = nullptr;
    int32 frame = 0;
    bool bIsLooping;
    bool bIsPlaying;
    float CurrentTime;
    float PlayRate;
    bool bUseExternalTime = false;
    float ExternalTime = 0.f;
};

