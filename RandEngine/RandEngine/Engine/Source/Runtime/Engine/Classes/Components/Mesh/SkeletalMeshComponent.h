#pragma once
#include "SkinnedMeshComponent.h"

class UAnimInstance;
class UAnimationAsset;
class UAnimSingleNodeInstance;

namespace EAnimationMode
{
    enum Type : int
    {
        AnimationBlueprint,
        AnimationSingleNode,
        // This is custom type, engine leaves AnimInstance as it is
        AnimationCustomMode,
    };
}

class USkeletalMeshComponent : public USkinnedMeshComponent
{
    DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

public:
    USkeletalMeshComponent();

    virtual UObject* Duplicate(UObject* InOuter) override;

    void SetselectedSubMeshIndex(const int& value) { selectedSubMeshIndex = value; }
    int GetselectedSubMeshIndex() const { return selectedSubMeshIndex; };

    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;

    virtual int CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const override;
    
    USkeletalMesh* GetSkeletalMeshAsset() const;

    // Methods for animation
    void SetAnimationMode(EAnimationMode::Type InAnimationMode);
    EAnimationMode::Type GetAnimationMode() const;
    UAnimSingleNodeInstance* GetSingleNodeInstance() const;
    void PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping);
    void SetAnimation(UAnimationAsset* NewAnimToPlay);
    void Play(bool bLooping);
    void Stop();
    bool IsPlaying() const;
public:
    UAnimInstance* AnimScriptInstance;
    uint8 bEnableAnimation : 1;
protected:
    int selectedSubMeshIndex = -1;

    // Only from Animasset currently
    EAnimationMode::Type AnimationMode;
};
