#pragma once
#include "RealCurve.h"

struct FRichCurveKey
{
	/** Interpolation mode between this key and the next */
	// TEnumAsByte<ERichCurveInterpMode> InterpMode;
	//
	// /** Mode for tangents at this key */
	// TEnumAsByte<ERichCurveTangentMode> TangentMode;
	//
	// /** If either tangent at this key is 'weighted' */
	// TEnumAsByte<ERichCurveTangentWeightMode> TangentWeightMode;

	/** Time at this key */
	float Time;

	/** Value at this key */
	float Value;

	/** If RCIM_Cubic, the arriving tangent at this key */
	float ArriveTangent;

	/** If RCTWM_WeightedArrive or RCTWM_WeightedBoth, the weight of the left tangent */
	float ArriveTangentWeight;

	/** If RCIM_Cubic, the leaving tangent at this key */
	float LeaveTangent;

	/** If RCTWM_WeightedLeave or RCTWM_WeightedBoth, the weight of the right tangent */
	float LeaveTangentWeight;

	FRichCurveKey()
		:
  //   InterpMode(RCIM_Linear)
		// , TangentMode(RCTM_Auto)
		// , TangentWeightMode(RCTWM_WeightedNone),
        Time(0.f)
		, Value(0.f)
		, ArriveTangent(0.f)
		, ArriveTangentWeight(0.f)
		, LeaveTangent(0.f)
		, LeaveTangentWeight(0.f)
	{ }

	FRichCurveKey(float InTime, float InValue)
		:
  //   InterpMode(RCIM_Linear)
		// , TangentMode(RCTM_Auto)
		// , TangentWeightMode(RCTWM_WeightedNone)
		// ,
    Time(InTime)
		, Value(InValue)
		, ArriveTangent(0.f)
		, ArriveTangentWeight(0.f)
		, LeaveTangent(0.f)
		, LeaveTangentWeight(0.f)
	{ }

	// FRichCurveKey(float InTime, float InValue, float InArriveTangent, const float InLeaveTangent, ERichCurveInterpMode InInterpMode)
	// 	: InterpMode(InInterpMode)
	// 	, TangentMode(RCTM_Auto)
	// 	, TangentWeightMode(RCTWM_WeightedNone)
	// 	, Time(InTime)
	// 	, Value(InValue)
	// 	, ArriveTangent(InArriveTangent)
	// 	, ArriveTangentWeight(0.f)
	// 	, LeaveTangent(InLeaveTangent)
	// 	, LeaveTangentWeight(0.f)
	// { }

	/** Conversion constructor */
	// FRichCurveKey(const FInterpCurvePoint<float>& InPoint);
	// FRichCurveKey(const FInterpCurvePoint<FVector2D>& InPoint, int32 ComponentIndex);
	// FRichCurveKey(const FInterpCurvePoint<FVector>& InPoint, int32 ComponentIndex);
	// FRichCurveKey(const FInterpCurvePoint<FTwoVectors>& InPoint, int32 ComponentIndex);

	bool operator==(const FRichCurveKey& Other) const;
	bool operator!=(const FRichCurveKey& Other) const;
};


// 여기서 더 들어가지 말까? 복잡한데
struct FRichCurve : public FRealCurve
{
public:
	virtual FKeyHandle AddKey(float InTime, float InValue, const bool bUnwindRotation = false, FKeyHandle KeyHandle = FKeyHandle()) final override;

    virtual int32 GetNumKeys() const override;
    virtual void SetKeyTime(FKeyHandle KeyHandle, float NewTime) override;
    virtual float GetKeyTime(FKeyHandle KeyHandle) const override;
    virtual void DeleteKey(FKeyHandle KeyHandle) override;
    virtual float Eval(float InTime, float InDefaultValue = 0.0f) const final override;

    TArray<FRichCurveKey> Keys;
};
