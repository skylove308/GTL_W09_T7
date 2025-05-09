#include "AnimDataModel.h"

const TArray<FBoneAnimationTrack>& UAnimDataModel::GetBoneAnimationTracks() const
{
    return BoneAnimationTracks;
}

const FAnimationCurveData& UAnimDataModel::GetCurveData() const
{
    return CurveData;
}

double UAnimDataModel::GetPlayLength() const
{
    return FrameRate.AsSeconds(NumberOfFrames);
}

int32 UAnimDataModel::GetNumberOfFrames() const
{
    return NumberOfFrames;
}

int32 UAnimDataModel::GetNumberOfKeys() const
{
    return NumberOfKeys;
}

FFrameRate UAnimDataModel::GetFrameRate() const
{
    return FrameRate;
}

