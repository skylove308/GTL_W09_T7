#include "AnimCurveTypes.h"

////////////////////////////////////////////////////
//  FFloatCurve

// we don't want to have = operator. This only copies curves, but leaving naming and everything else intact. 
void FFloatCurve::CopyCurve(const FFloatCurve& SourceCurve)
{
    FloatCurve = SourceCurve.FloatCurve;
}

float FFloatCurve::Evaluate(float CurrentTime) const
{
	return FloatCurve.Eval(CurrentTime);
}

void FFloatCurve::UpdateOrAddKey(float NewKey, float CurrentTime)
{
    FloatCurve.UpdateOrAddKey(CurrentTime, NewKey);
}

void FFloatCurve::GetKeys(TArray<float>& OutTimes, TArray<float>& OutValues) const
{
    const int32 NumKeys = FloatCurve.GetNumKeys();
    // OutTimes.Empty(NumKeys);
    // OutValues.Empty(NumKeys);
    // for (auto It = FloatCurve.GetKeyHandleIterator(); It; ++It)
    // {
    //     const FKeyHandle KeyHandle = *It;
    //     const float KeyTime = FloatCurve.GetKeyTime(KeyHandle);
    //     const float Value = FloatCurve.Eval(KeyTime);
    //
    //     OutTimes.Add(KeyTime);
    //     OutValues.Add(Value);
    // }
}

void FFloatCurve::Resize(float NewLength, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime)
{
    // FloatCurve.ReadjustTimeRange(0, NewLength, bInsert, OldStartTime, OldEndTime);
}

////////////////////////////////////////////////////
//  FVectorCurve

// we don't want to have = operator. This only copies curves, but leaving naming and everything else intact. 
void FVectorCurve::CopyCurve(const FVectorCurve& SourceCurve)
{
	FloatCurves[0] = SourceCurve.FloatCurves[0];
	FloatCurves[1] = SourceCurve.FloatCurves[1];
	FloatCurves[2] = SourceCurve.FloatCurves[2];
}

FVector FVectorCurve::Evaluate(float CurrentTime, float BlendWeight) const
{
	FVector Value;

	Value.X = FloatCurves[(int32)EIndex::X].Eval(CurrentTime)*BlendWeight;
	Value.Y = FloatCurves[(int32)EIndex::Y].Eval(CurrentTime)*BlendWeight;
	Value.Z = FloatCurves[(int32)EIndex::Z].Eval(CurrentTime)*BlendWeight;

	return Value;
}

void FVectorCurve::UpdateOrAddKey(const FVector& NewKey, float CurrentTime)
{
	FloatCurves[(int32)EIndex::X].UpdateOrAddKey(CurrentTime, NewKey.X);
	FloatCurves[(int32)EIndex::Y].UpdateOrAddKey(CurrentTime, NewKey.Y);
	FloatCurves[(int32)EIndex::Z].UpdateOrAddKey(CurrentTime, NewKey.Z);
}

void FVectorCurve::GetKeys(TArray<float>& OutTimes, TArray<FVector>& OutValues) const
{
	// Determine curve with most keys
	int32 MaxNumKeys = 0;
	int32 UsedCurveIndex = INDEX_NONE;
	for (int32 CurveIndex = 0; CurveIndex < 3; ++CurveIndex)
	{
		const int32 NumKeys = FloatCurves[CurveIndex].GetNumKeys();
		if (NumKeys > MaxNumKeys)
		{
			MaxNumKeys = NumKeys;
			UsedCurveIndex = CurveIndex;
		}
	}

	if (UsedCurveIndex != INDEX_NONE)
	{
		// OutTimes.Empty(MaxNumKeys);
		// OutValues.Empty(MaxNumKeys);
		// for (auto It = FloatCurves[UsedCurveIndex].GetKeyHandleIterator(); It; ++It)
		// {
		// 	const FKeyHandle KeyHandle = *It;
		// 	const float KeyTime = FloatCurves[UsedCurveIndex].GetKeyTime(KeyHandle);
		// 	const FVector Value = Evaluate(KeyTime, 1.0f);
		//
		// 	OutTimes.Add(KeyTime);
		// 	OutValues.Add(Value);
		// }
	}
}

void FVectorCurve::Resize(float NewLength, bool bInsert/* whether insert or remove*/, float OldStartTime, float OldEndTime)
{
	// FloatCurves[(int32)EIndex::X].ReadjustTimeRange(0, NewLength, bInsert, OldStartTime, OldEndTime);
	// FloatCurves[(int32)EIndex::Y].ReadjustTimeRange(0, NewLength, bInsert, OldStartTime, OldEndTime);
	// FloatCurves[(int32)EIndex::Z].ReadjustTimeRange(0, NewLength, bInsert, OldStartTime, OldEndTime);
}

int32 FVectorCurve::GetNumKeys() const
{
	int32 MaxNumKeys = 0;
	for (int32 CurveIndex = 0; CurveIndex < 3; ++CurveIndex)
	{
		const int32 NumKeys = FloatCurves[CurveIndex].GetNumKeys();
		MaxNumKeys = FMath::Max(MaxNumKeys, NumKeys);
	}

	return MaxNumKeys;
}
