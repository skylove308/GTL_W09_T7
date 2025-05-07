#pragma once

#include "Define.h"
#include "Hal/PlatformType.h"
#include "Container/Array.h"

struct FSkeletalMeshVertex 
{
    float X, Y, Z;    // Position
    float R, G, B, A; // Color
    float NormalX, NormalY, NormalZ;
    float TangentX, TangentY, TangentZ, TangentW;
    float U = 0, V = 0;
    uint32 MaterialIndex;
    int32 ControlPointIndex = -1;
};

struct FSkeletalMeshBoneWeight
{
    uint8  BoneIndices[4];  // 이 정점에 영향을 주는 본의 인덱스(최대 4개)
    float  Weights[4];      // 각 본에 대한 가중치 (합이 1.0이 되도록)
};

struct FBone 
{
    FMatrix SkinningMatrix;
};

struct FSkeletalHierarchyData
{
    FString NodeName;
    TArray<FSkeletalHierarchyData> Children;
};

struct FSkeletonBone
{
    FString Name;
    int32 ParentIndex;
    FMatrix LocalBindPose;
    FMatrix GlobalPose;
};

struct FSkeletalMeshRenderData 
{
    FWString ObjectName;
    FString DisplayName;

    TArray<FSkeletalMeshVertex> Vertices;
    TArray<UINT> Indices;

    TArray<FObjMaterialInfo> Materials;
    TArray<FMaterialSubset> MaterialSubsets;

    FVector BoundingBoxMin;
    FVector BoundingBoxMax;

    TArray<FSkeletonBone> SkeletonBones;
    TArray<FSkeletalMeshBoneWeight> BoneWeights;

    FSkeletalHierarchyData RootSkeletal;
};
