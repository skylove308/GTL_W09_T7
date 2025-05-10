#pragma once
#include "Container/Array.h"
#include "Math/Quat.h"
#include "Math/Vector.h"

struct FRawAnimSequenceTrack
{
    TArray<FVector> PosKeys;   // 위치 키프레임
    TArray<FQuat>   RotKeys;   // 회전 키프레임 (Quaternion)
    TArray<FVector> ScaleKeys; // 스케일 키프레임
    
};
