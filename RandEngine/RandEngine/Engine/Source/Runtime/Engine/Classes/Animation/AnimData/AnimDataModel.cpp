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

void UAnimDataModel::SetBoneAnimationTracks(const TArray<FBoneAnimationTrack>& InValue)
{
    BoneAnimationTracks = InValue;
}

void UAnimDataModel::SetCurveData(const FAnimationCurveData& InValue)
{
    CurveData = InValue;
}

void UAnimDataModel::SetPlayLength(const double& InValue)
{
    PlayLength = InValue;
}

void UAnimDataModel::SetNumberOfFrames(const int32& InValue)
{
    NumberOfFrames = InValue;
}

void UAnimDataModel::SetNumberOfKeys(const int32& InValue)
{
    NumberOfKeys = InValue;
}

void UAnimDataModel::SetFrameRate(const FFrameRate& InValue)
{
    FrameRate = InValue;
}

