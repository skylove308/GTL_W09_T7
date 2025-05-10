#pragma once
#include "Curves/RichCurve.h"
#include "Math/Transform.h"
#include "UObject/NameTypes.h"


struct FAnimCurveBase
{
    // FLinearColor Color;
    FString Comment;
private:
    FName CurveName;
    // int32		CurveTypeFlags;

public:
    FAnimCurveBase() {}
    
    FAnimCurveBase(FName InName) : CurveName(InName)
    {
    }

    /** Get the name of this curve */
    FName GetName() const
    {
        return CurveName;
    }

    /** Set the name of this curve */
    void SetName(FName InName)
    {
        CurveName = InName;
    }

    // FLinearColor GetColor() const { return Color; }
};


struct FFloatCurve : public FAnimCurveBase
{
    FFloatCurve()
        : FAnimCurveBase()
    {
    }

    FFloatCurve(FName InName) : FAnimCurveBase(InName)
    {
    }

    void CopyCurve(const FFloatCurve& SourceCurve);
    float Evaluate(float CurrentTime) const;
    void UpdateOrAddKey(float NewKey, float CurrentTime);
    void GetKeys(TArray<float>& OutTimes, TArray<float>& OutValues) const;
    void Resize(float NewLength, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime);


    FRichCurve FloatCurve;
};

struct FVectorCurve : public FAnimCurveBase
{
    enum class EIndex
    {
        X = 0, 
        Y, 
        Z, 
        Max
    };

    FRichCurve	FloatCurves[3];

    FVectorCurve(){}
    
    FVectorCurve(FName InName)
    : FAnimCurveBase(InName)
    {
    }

    void CopyCurve(const FVectorCurve& SourceCurve);
    FVector Evaluate(float CurrentTime, float BlendWeight) const;
    void UpdateOrAddKey(const FVector& NewKey, float CurrentTime);
    void GetKeys(TArray<float>& OutTimes, TArray<FVector>& OutValues) const;
    bool DoesContainKey() const { return (FloatCurves[0].GetNumKeys() > 0 || FloatCurves[1].GetNumKeys() > 0 || FloatCurves[2].GetNumKeys() > 0);}
    void Resize(float NewLength, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime);
    int32 GetNumKeys() const;
};

struct FTransformCurve: public FAnimCurveBase
{
    FVectorCurve	TranslationCurve;
    /** Rotation curve - right now we use euler because quat also doesn't provide linear interpolation - curve editor can't handle quat interpolation
     * If you hit gimbal lock, you should add extra key to fix it. This will cause gimbal lock. 
     * @TODO: Eventually we'll need FRotationCurve that would contain rotation curve - that will interpolate as slerp or as quaternion 
     */
    FVectorCurve	RotationCurve;
    FVectorCurve	ScaleCurve;

    FTransformCurve(){}

    FTransformCurve(FName InName) : FAnimCurveBase(InName)
    {
    }

    void CopyCurve(const FTransformCurve& SourceCurve);
    FTransform Evaluate(float CurrentTime, float BlendWeight) const;
    void UpdateOrAddKey(const FTransform& NewKey, float CurrentTime);
    void GetKeys(TArray<float>& OutTimes, TArray<FTransform>& OutValues) const;
    void Resize(float NewLength, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime);

    const FVectorCurve* GetVectorCurveByIndex(int32 Index) const;
    FVectorCurve* GetVectorCurveByIndex(int32 Index);
};
