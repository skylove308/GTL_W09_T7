#pragma once
#include "Math/Quat.h"
#include "Math/Vector.h"
#include "Container/Array.h"

#include "Animation/AnimInstance.h"

enum class EAnimInterpolationType : uint8
{
    Linear,
    Step,
};

struct FRawAnimSequenceTrack
{
    TArray<FVector> PosKeys;   // 위치 키프레임
    TArray<FQuat>   RotKeys;   // 회전 키프레임 (Quaternion)
    TArray<FVector> ScaleKeys; // 스케일 키프레임
};

struct FBoneContainer
{
    TArray<FMatrix> BoneLocalTransforms;
};

struct FCompactPose
{
    FBoneContainer BonContainer;
};

struct FPoseContext
{
public:
    UAnimInstance* AnimInstance; // Originally from FAnimationBaseContext
    FCompactPose Pose;
    bool bIsAdditivePose;

    FPoseContext()
        :AnimInstance(nullptr), bIsAdditivePose(false)
    {
    }

    FPoseContext(UAnimInstance* InAnimInstance, bool bInExpectsAdditive = false)
        :AnimInstance(InAnimInstance), bIsAdditivePose(bInExpectsAdditive)
    {
    }

    void ResetToRefPose(const TArray<FMatrix>& RefPoses)
    {
        Pose.BonContainer.BoneLocalTransforms.Empty();

        for (const FMatrix& RefPose : RefPoses)
        {
            Pose.BonContainer.BoneLocalTransforms.Add(RefPose);
        }
    }

};

struct FDeltaTimeRecord
{
public:
    void Set(float InPrevious, float InDelta)
    {
        Previous = InPrevious;
        Delta = InDelta;
        bPreviousIsValid = true;
    }
    void SetPrevious(float InPrevious) { Previous = InPrevious; bPreviousIsValid = true; }
    float GetPrevious() const { return Previous; }
    bool IsPreviousValid() const { return bPreviousIsValid; }

    float Delta = 0.f;
private:
    float Previous = 0.f;
    bool  bPreviousIsValid = false; // Will be set to true when Previous has been set
};

struct FPoseCurve
{
    // The name of the curve
    FName				Name;
    // PoseIndex of pose asset it's dealing with
    // used to extract pose value fast
    int32				PoseIndex;
    // Curve Value
    float				Value;

    FPoseCurve()
        : Name(NAME_None)
        , PoseIndex(INDEX_NONE)
        , Value(0.f)
    {
    }

    FPoseCurve(int32 InPoseIndex, FName InName, float InValue)
        : Name(InName)
        , PoseIndex(InPoseIndex)
        , Value(InValue)
    {
    }
};

struct FAnimExtractContext
{
    double CurrentTime;
    /** Is root motion being extracted? */
    bool bExtractRootMotion;
    /** Delta time range required for root motion extraction **/
    FDeltaTimeRecord DeltaTimeRecord;
    bool bLooping;
    /**
     * Pose Curve Values to extract pose from pose assets.
     * This is used by pose asset extraction
     */
    // 어떻게 사용할지? 당장은 애매함
    TArray<FPoseCurve> PoseCurves;
    /**
     * The BonesRequired array is a list of bool flags to determine
     * if a bone is required to be retrieved. This is currently used
     * by several animation nodes to optimize evaluation time.
     */
    TArray<bool> BonesRequired;
    /**
     * The optional interpolation mode override.
     * If not set, it will simply use the interpolation mode provided by the asset.
     * One example where this could be used is if you want to force sampling the animation with Step interpolation
     * even when the animation sequence asset is set to Linear interpolation.
     */
    EAnimInterpolationType InterpolationOverride;

    FAnimExtractContext(double InCurrentTime = 0.0, bool InbExtractRootMotion = false, FDeltaTimeRecord InDeltaTimeRecord = {}, bool InbLooping = false)
        : CurrentTime(InCurrentTime)
        , bExtractRootMotion(InbExtractRootMotion)
        , DeltaTimeRecord(InDeltaTimeRecord)
        , bLooping(InbLooping)
        , PoseCurves()
        , BonesRequired()
        , InterpolationOverride()
    {
    }

    bool IsBoneRequired(int32 BoneIndex) const
    {
        if (BoneIndex >= BonesRequired.Num())
        {
            return true;
        }

        return BonesRequired[BoneIndex];
    }
};
