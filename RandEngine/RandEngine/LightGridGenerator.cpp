#include "LightGridGenerator.h"
#include "Actors/PointLightActor.h"
#include "Actors/SpotLightActor.h"
#include "World/World.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Engine/EditorEngine.h"
#include "Engine/Engine.h"

FLightGridGenerator::~FLightGridGenerator()
{
    for (TArray<AActor*>& Shell : LightGrid)
    {
        for (AActor* Actor : Shell)
        {
            if (Actor)
            {
                Actor->Destroy();
            }
        }
        Shell.Empty();
    }

    LightGrid.Empty();
}

void FLightGridGenerator::GenerateLight(UWorld* World)
{
    const int MinR = CurrentHalfCountPerAxis;
    const int MaxR = CurrentHalfCountPerAxis;

    const int R = CurrentHalfCountPerAxis;
    const int OuterCube = (2 * R + 1) * (2 * R + 1) * (2 * R + 1);
    const int InnerCube = FMath::Max(0, (2 * R - 1) * (2 * R - 1) * (2 * R - 1));
    const int ApproxShellSize = OuterCube - InnerCube;

    TArray<AActor*> NewShell;
    NewShell.Reserve(ApproxShellSize);


    for (int X = -MaxR; X <= MaxR; ++X)
    {
        for (int Y = -MaxR; Y <= MaxR; ++Y)
        {
            for (int Z = -MaxR; Z <= MaxR; ++Z)
            {
                //if (x == 0 && y == 0 && z == 0) continue;

                // 이전 쉘 내부는 제외
                if (FMath::Abs(X) < MinR && FMath::Abs(Y) < MinR && FMath::Abs(Z) < MinR)
                {
                    continue;
                }

                FVector Pos = GetJitteredPosition(X, Y, Z, Spacing, JitterAmount);

                AActor* Light = nullptr;
                if ((ShellLightCount % 2) == 0)
                {
                    Light = World->SpawnActor<APointLight>();
                    Light->SetActorLabel(FString::Printf(TEXT("PointLight_%d"), ShellLightCount));
                    UPointLightComponent* Comp = Light->GetComponentByClass<UPointLightComponent>();
                    //Comp->SetLightColor(RandomColor());
                }
                else
                {
                    Light = World->SpawnActor<ASpotLight>();
                    Light->SetActorLabel(FString::Printf(TEXT("SpotLight_%d"), ShellLightCount));
                    USpotLightComponent* Comp = Light->GetComponentByClass<USpotLightComponent>();
                    //Comp->SetLightColor(RandomColor());
                }

                if (Light)
                {
                    Light->SetActorLocation(Pos);
                    NewShell.Add(Light);
                }

                ++ShellLightCount;
            }
        }
    }

    if (LightGrid.Num() <= CurrentHalfCountPerAxis)
    {
        LightGrid.SetNum(CurrentHalfCountPerAxis + 1);
    }

    LightGrid[CurrentHalfCountPerAxis] = std::move(NewShell);
    ++CurrentHalfCountPerAxis;
}


void FLightGridGenerator::DeleteLight(UWorld* World)
{
    if (CurrentHalfCountPerAxis <= 0 || LightGrid.Num() == 0)
    {
        return;
    }

    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
    {
        return;
    }

    const int ShellIndex = CurrentHalfCountPerAxis - 1;

    // 유효한 인덱스인지 확인
    if (!LightGrid.IsValidIndex(ShellIndex))
    {
        return;
    }

    TArray<AActor*>& Shell = LightGrid[ShellIndex];

    for (AActor* Light : Shell)
    {
        if (Light)
        {
            Engine->DeselectActor(Light);
            Light->Destroy();
        }
    }

    // TArray는 SetNum을 통해 capacity 보존 가능
    Shell.Empty();

    --CurrentHalfCountPerAxis;
}

void FLightGridGenerator::Reset(UWorld* World)
{
    for (TArray<AActor*>& Shell : LightGrid)
    {
        for (AActor* Light : Shell)
        {
            if (Light)
            {
                Light->Destroy();
            }
        }
        Shell.Empty();
    }
    CurrentHalfCountPerAxis = StartCountPerAxis;
}

FVector FLightGridGenerator::GetJitteredPosition(const int X, const int Y, const int Z, const float Spacing, const float JitterAmount)
{
    auto LCG = [](int Seed) -> float
    {
        Seed = (1103515245 * Seed + 12345) & 0x7fffffff;
        return (Seed % 1000) / 1000.0f;
    };

    const int SeedBase = X * 73856093 ^ Y * 19349663 ^ Z * 83492791;
    const float dx = (LCG(SeedBase + 1) - 0.5f) * 2.0f * JitterAmount;
    const float dy = (LCG(SeedBase + 2) - 0.5f) * 2.0f * JitterAmount;
    const float dz = (LCG(SeedBase + 3) - 0.5f) * 2.0f * JitterAmount;

    return {X * Spacing + dx, Y * Spacing + dy, Z * Spacing + dz};
}

FLinearColor FLightGridGenerator::RandomColor() const
{
    auto HashFloat01 = [](int Seed) -> float
    {
        Seed = (1103515245 * Seed + 12345) & 0x7fffffff;
        return (Seed % 1000) / 1000.0f; // 0.0 ~ 0.999
    };

    const int ColorSeed = ShellLightCount * 1234567; // shellLightCount를 기반 시드로 사용

    const float R = HashFloat01(ColorSeed + 1);
    const float G = HashFloat01(ColorSeed + 2);
    const float B = HashFloat01(ColorSeed + 3);

    return {R,G,B,1.0f};
}

