#pragma once

#include "Define.h"
#include "Hal/PlatformType.h"
#include "Container/Array.h"

struct FStaticMeshVertex
{
    float X, Y, Z;    // Position
    float R, G, B, A; // Color
    float NormalX, NormalY, NormalZ;
    float TangentX, TangentY, TangentZ, TangentW;
    float U = 0, V = 0;
    uint32 MaterialIndex;

    int32 ControlPointIndex = -1;
    
    bool operator==(const FStaticMeshVertex& Other) const
    {
        return X == Other.X && Y == Other.Y && Z == Other.Z &&
            NormalX == Other.NormalX && NormalY == Other.NormalY && NormalZ == Other.NormalZ &&
            U == Other.U && V == Other.V &&
            MaterialIndex == Other.MaterialIndex;
    }
};

namespace std
{
    template<>
    struct hash<FStaticMeshVertex>
    {
        size_t operator()(const FStaticMeshVertex& V) const
        {
            size_t h1 = std::hash<float>()(V.X) ^ std::hash<float>()(V.Y) ^ std::hash<float>()(V.Z);
            size_t h2 = std::hash<float>()(V.NormalX) ^ std::hash<float>()(V.NormalY) ^ std::hash<float>()(V.NormalZ);
            size_t h3 = std::hash<float>()(V.U) ^ std::hash<float>()(V.V);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

struct FStaticMeshRenderData
{
    FWString ObjectName;
    FString DisplayName;

    TArray<FStaticMeshVertex> Vertices;
    TArray<UINT> Indices;

    TArray<FObjMaterialInfo> Materials;
    TArray<FMaterialSubset> MaterialSubsets;

    FVector BoundingBoxMin;
    FVector BoundingBoxMax;
};
