#pragma once
#include "Launch/Define.h"
class UWorld;
class AActor;


namespace ELightGridGenerator
{
    enum Mode
    {
        Generate,
        Delete,
        Reset
    };
}

class FLightGridGenerator
{
public:
    FLightGridGenerator() = default;
    ~FLightGridGenerator();
    void GenerateLight(UWorld* World);
    void DeleteLight(UWorld* World);
    void Reset(UWorld* World);
private:
    TArray<TArray<AActor*>> LightGrid;
    int CurrentHalfCountPerAxis = StartCountPerAxis;
    const int StartCountPerAxis = 0;
    const float Spacing = 10.0f;                       //Light간의 간격
    const float JitterAmount = 0.0f;                   //Light 위치 랜덤 값. 매번 같은 랜덤.
    int ShellLightCount = 0;
    static FVector GetJitteredPosition(int X, int Y, int Z, float Spacing, float JitterAmount);
    FLinearColor RandomColor() const;
};

