#pragma once
#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimTypes.h"
#include "Misc/FrameRate.h"
#include "UObject/NameTypes.h"


struct FBoneAnimationTrack
{
    FName Name;                        // Bone 이름
    FRawAnimSequenceTrack InternalTrackData; // 실제 애니메이션 데이터
};

struct FAnimationCurveData
{

    /** Float-based animation curves */
    TArray<FFloatCurve>	FloatCurves;

    /** FTransform-based animation curves, used for animation layer editing */
    TArray<FTransformCurve>	TransformCurves;
};

class IAnimationDataModel
{
public:
    /**
    * @return	Total length of play-able animation data 
    */
    virtual double GetPlayLength() const = 0;
	
    /**
    * @return	Total number of frames of animation data stored 
    */
    virtual int32 GetNumberOfFrames() const = 0;

    /**
    * @return	Total number of animation data keys stored 
    */
    virtual int32 GetNumberOfKeys() const = 0;

    /**
    * @return	Frame rate at which the animation data is key-ed 
    */
    virtual FFrameRate GetFrameRate() const = 0;

    /**
    * @return	Array containing all bone animation tracks 
    */
    virtual const TArray<FBoneAnimationTrack>& GetBoneAnimationTracks() const = 0;
    virtual const FAnimationCurveData& GetCurveData() const = 0;
    // virtual FTransform EvaluateBoneTrackTransform(FName TrackName, const FFrameTime& FrameTime, const EAnimInterpolationType& Interpolation) const = 0;
    // virtual FTransform GetBoneTrackTransform(FName TrackName, const FFrameNumber& FrameNumber) const = 0;
    // virtual void GetBoneTrackTransforms(FName TrackName, const TArray<FFrameNumber>& FrameNumbers, TArray<FTransform>& OutTransforms) const = 0;
    // virtual void GetBoneTrackTransforms(FName TrackName, TArray<FTransform>& OutTransforms) const = 0;
    // virtual void GetBoneTracksTransform(const TArray<FName>& TrackNames, const FFrameNumber& FrameNumber, TArray<FTransform>& OutTransforms) const = 0;
};
