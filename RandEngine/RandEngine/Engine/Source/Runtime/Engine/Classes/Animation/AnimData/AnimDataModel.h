#pragma once
#include "IAnimationDataModel.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

// Animation 실제 데이터
class UAnimDataModel : public UObject, public IAnimationDataModel
{
    DECLARE_CLASS(UAnimDataModel, UObject)
public:
    UAnimDataModel() = default;
    
    virtual const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const override;
    virtual const FAnimationCurveData& GetCurveData() const override;

    double GetPlayLength() const override;
    int32 GetNumberOfFrames() const override;
    int32 GetNumberOfKeys() const override;
    FFrameRate GetFrameRate() const override;
    // ...


    void SetBoneAnimationTracks(const TArray<FBoneAnimationTrack>& InValue);
    void SetCurveData(const FAnimationCurveData& InValue);
    void SetPlayLength(const double& InValue);
    void SetNumberOfFrames(const int32& InValue);
    void SetNumberOfKeys(const int32& InValue);
    void SetFrameRate(const FFrameRate& InValue);

private:
    TArray<FBoneAnimationTrack> BoneAnimationTracks;
    float PlayLength;
    FFrameRate FrameRate;
    int32 NumberOfFrames;
    int32 NumberOfKeys;
    FAnimationCurveData CurveData;
};
