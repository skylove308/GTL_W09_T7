#pragma once

#include <fbxsdk.h>
#include <functional>
#include <unordered_map> 

// --- 기본 타입 및 컨테이너 ---
#include "Define.h"
#include "Math/Vector.h"      // FVector, FVector2D 포함 가정
#include "Math/Vector4.h"     // FVector4 포함 가정
#include "Math/Matrix.h"      // FMatrix 포함 가정
#include "Container/Array.h" // TArray 포함 가정
#include "Container/Map.h"    // TMap 포함 가정
#include "UObject/NameTypes.h" // FName 포함 가정
#include "Components/Mesh/SkeletalMesh.h"
#include "Asset/SkeletalMeshAsset.h"

#include "SkeletalMeshDebugger.h"

class USkeletalMeshComponent;

namespace FBX
{
    struct FBoneHierarchyNode;
    struct FBXInfo;
    struct MeshRawData;
}

namespace std
{
    // hash_combine 함수 템플릿 정의 (헤더에 위치)
    template <class T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    // std::hash<FSkeletalMeshVertex> 특수화 *선언* (구현은 cpp 파일에)
    template <>
    struct hash<FSkeletalMeshVertex>
    {
        size_t operator()(const FSkeletalMeshVertex& Key) const noexcept;
    };
}

struct FFBXLoader
{
    static bool ParseFBX(const FString& FBXFilePath, FBX::FBXInfo& OutFBXInfo);

    // Convert the Raw data to Cooked data (FSkeletalMeshRenderData)
    static bool ConvertToSkeletalMesh(const TArray<FBX::MeshRawData>& RawMeshData, const FBX::FBXInfo& FullFBXInfo, FSkeletalMeshRenderData& OutSkeletalMesh, USkeleton* OutSkeleton);

    static bool CreateTextureFromFile(const FWString& Filename);

    static void ComputeBoundingBox(const TArray<FSkeletalMeshVertex>& InVertices, FVector& OutMinVector, FVector& OutMaxVector);

private:
    static void CalculateTangent(FSkeletalMeshVertex& PivotVertex, const FSkeletalMeshVertex& Vertex1, const FSkeletalMeshVertex& Vertex2);
};

struct FManagerFBX
{
public:
    static FSkeletalMeshRenderData* LoadFBXSkeletalMeshAsset(const FString& PathFileName, USkeleton* OutSkeleton);

    static void CombineMaterialIndex(FSkeletalMeshRenderData& OutFSkeletalMesh);

    static bool SaveSkeletalMeshToBinary(const FWString& FilePath, const FSkeletalMeshRenderData& SkeletalMesh);

    static bool LoadSkeletalMeshFromBinary(const FWString& FilePath, FSkeletalMeshRenderData& OutSkeletalMesh);

    static UMaterial* CreateMaterial(const FFbxMaterialInfo& MaterialInfo);

    static UMaterial* GetMaterial(const FString& InName);

    static USkeletalMesh* CreateSkeletalMesh(const FString& InFilePath);

    static const TMap<FWString, USkeletalMesh*>& GetSkeletalMeshes();

    static USkeletalMesh* GetSkeletalMesh(const FWString& InName);

private:
    inline static TMap<FString, FSkeletalMeshRenderData*> SkeletalMeshRenderDataMap;
    inline static TMap<FWString, USkeletalMesh*> SkeletalMeshMap;
    inline static TMap<FString, UMaterial*> MaterialMap;
};
