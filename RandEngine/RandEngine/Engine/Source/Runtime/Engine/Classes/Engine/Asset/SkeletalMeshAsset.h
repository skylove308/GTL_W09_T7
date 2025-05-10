#pragma once

#include "Define.h"
#include "Hal/PlatformType.h"
#include "Container/Array.h"

struct FSkeletalMeshVertex
{
    FVector Position;
    float R, G, B, A; // Color
    FVector Normal;
    float TangentX, TangentY, TangentZ, TangentW;
    FVector2D TexCoord;
    uint32 MaterialIndex;

    // TODO: FVector Tangent; // 탄젠트 계산 활성화 시 주석 해제 및 CalculateTangent 구현 수정 필요
    uint32 BoneIndices[MAX_BONE_INFLUENCES] = { 0 };
    float BoneWeights[MAX_BONE_INFLUENCES] = { 0.0f };

    // 비교 연산자 (정점 중복 제거용)
    bool operator==(const FSkeletalMeshVertex& Other) const
    {
        if (Position != Other.Position ||
            Normal != Other.Normal ||
            TexCoord != Other.TexCoord) // || Tangent != Other.Tangent) // 탄젠트 추가 시 비교
        {
            return false;
        }
        for (int i = 0; i < MAX_BONE_INFLUENCES; ++i)
        {
            // 가중치는 부동소수점 비교 주의
            if (BoneIndices[i] != Other.BoneIndices[i] || !FMath::IsNearlyEqual(BoneWeights[i], Other.BoneWeights[i]))
            {
                return false;
            }
        }
        return true;
    }
    bool operator!=(const FSkeletalMeshVertex& Other) const
    {
        return !(*this == Other);
    }
};

struct FSkeletalMeshBoneWeight
{
    uint8  BoneIndices[4];  // 이 정점에 영향을 주는 본의 인덱스(최대 4개)
    float  Weights[4];      // 각 본에 대한 가중치 (합이 1.0이 되도록)
};

struct FSkeletalHierarchyData
{
    FString NodeName;
    TArray<FSkeletalHierarchyData> Children;
};

struct FFbxMaterialInfo
{
    FName MaterialName = NAME_None;
    FString UUID;

    FLinearColor BaseColorFactor = FLinearColor::White;
    FLinearColor EmissiveFactor = FLinearColor::Black;
    FVector SpecularFactor = FVector(1.0f, 1.0f, 1.0f);
    float MetallicFactor = 0.0f;
    float RoughnessFactor = 0.8f;
    float SpecularPower = 32.0f; // Shininess
    float OpacityFactor = 1.0f;

    FWString BaseColorTexturePath;
    FWString NormalTexturePath;
    FWString MetallicTexturePath;
    FWString RoughnessTexturePath;
    FWString SpecularTexturePath;
    FWString EmissiveTexturePath;
    FWString AmbientOcclusionTexturePath;
    FWString OpacityTexturePath;

    bool bHasBaseColorTexture = false;
    bool bHasNormalTexture = false;
    bool bHasMetallicTexture = false;
    bool bHasRoughnessTexture = false;
    bool bHasSpecularTexture = false;
    bool bHasEmissiveTexture = false;
    bool bHasAmbientOcclusionTexture = false;
    bool bHasOpacityTexture = false;

    bool bIsTransparent = false;
    bool bUsePBRWorkflow = true; // 기본적으로 PBR 선호

    FFbxMaterialInfo() = default;
};

struct FMeshSubset
{
    uint32 IndexStart = 0;
    uint32 IndexCount = 0;
    uint32 MaterialIndex = 0;
    // FString MaterialName; // for debug
};

struct FSkeletalMeshRenderData
{
    FString MeshName;
    FString FilePath;

    TArray<FSkeletalMeshVertex> BindPoseVertices; // 최종 고유 정점 배열 (바인드 포즈)
    TArray<uint32> Indices;                       // 정점 인덱스 배열

    TArray<FFbxMaterialInfo> Materials;           // 이 메시에 사용된 재질 정보 배열
    TArray<FMeshSubset> Subsets;                  // 재질별 인덱스 범위 정보

    // DirectX 버퍼 포인터 (생성 후 채워짐)
    ID3D11Buffer* DynamicVertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;

    FBoundingBox Bounds;                          // 메시의 AABB

    FSkeletalMeshRenderData() = default;
    ~FSkeletalMeshRenderData() { ReleaseBuffers(); }

    FSkeletalMeshRenderData(const FSkeletalMeshRenderData&) = delete;
    FSkeletalMeshRenderData& operator=(const FSkeletalMeshRenderData&) = delete;

    FSkeletalMeshRenderData(FSkeletalMeshRenderData&& Other) noexcept
        : MeshName(std::move(Other.MeshName)), // std::move 사용
        FilePath(std::move(Other.FilePath)),
        BindPoseVertices(std::move(Other.BindPoseVertices)),
        Indices(std::move(Other.Indices)),
        Materials(std::move(Other.Materials)),
        Subsets(std::move(Other.Subsets)), // Subsets 이동 추가
        DynamicVertexBuffer(Other.DynamicVertexBuffer),
        IndexBuffer(Other.IndexBuffer),
        Bounds(Other.Bounds)
    {
        Other.DynamicVertexBuffer = nullptr;
        Other.IndexBuffer = nullptr;
    }

    FSkeletalMeshRenderData& operator=(FSkeletalMeshRenderData&& Other) noexcept
    {
        if (this != &Other)
        {
            ReleaseBuffers();
            MeshName = std::move(Other.MeshName);
            FilePath = std::move(Other.FilePath);
            BindPoseVertices = std::move(Other.BindPoseVertices);
            Indices = std::move(Other.Indices);
            Materials = std::move(Other.Materials);
            Subsets = std::move(Other.Subsets); // Subsets 이동 추가
            DynamicVertexBuffer = Other.DynamicVertexBuffer;
            IndexBuffer = Other.IndexBuffer;
            Bounds = Other.Bounds;
            Other.DynamicVertexBuffer = nullptr;
            Other.IndexBuffer = nullptr;
        }
        return *this;
    }

    void ReleaseBuffers()
    {
        if (DynamicVertexBuffer) { DynamicVertexBuffer->Release(); DynamicVertexBuffer = nullptr; }
        if (IndexBuffer) { IndexBuffer->Release(); IndexBuffer = nullptr; }
    }

    void CalculateBounds()
    {
        if (BindPoseVertices.IsEmpty())
        {
            Bounds.MinLocation = FVector::ZeroVector;
            Bounds.MaxLocation = FVector::ZeroVector;
            return;
        }
        Bounds.MinLocation = BindPoseVertices[0].Position;
        Bounds.MaxLocation = BindPoseVertices[0].Position;
        for (int32 i = 1; i < BindPoseVertices.Num(); ++i)
        {
            Bounds.MinLocation = FVector::Min(Bounds.MinLocation, BindPoseVertices[i].Position);
            Bounds.MaxLocation = FVector::Max(Bounds.MaxLocation, BindPoseVertices[i].Position);
        }
    }
};
