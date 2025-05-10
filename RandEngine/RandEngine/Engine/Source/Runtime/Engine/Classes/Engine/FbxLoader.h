#pragma once
#include "Define.h"

// --- 기본 타입 및 컨테이너 ---
#include "Math/Vector.h"      // FVector, FVector2D 포함 가정
#include "Math/Vector4.h"     // FVector4 포함 가정
#include "Math/Matrix.h"      // FMatrix 포함 가정
#include "Container/Array.h" // TArray 포함 가정
#include "Container/Map.h"    // TMap 포함 가정
#include "UObject/NameTypes.h" // FName 포함 가정
#include "Components/Mesh/SkeletalMesh.h"
#include "Asset/SkeletalMeshAsset.h"
#include <fbxsdk.h>

#include <functional>
#include <unordered_map> 
#include "SkeletalMeshDebugger.h" // FSkeletalMeshDebugger 클래스 선언을 사용하기 위해 포함

class USkeletalMeshComponent;

// --- FBX 로딩 관련 네임스페이스 ---
namespace FBX
{
    struct FBoneHierarchyNode;
    // --- 데이터 구조체 정의 ---
    // 재질 정보 구조체

    struct FBXInfo;
    struct MeshRawData;
} // namespace FBX


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
        // 함수 선언만 남기고 세미콜론으로 끝냅니다.
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

    static UMaterial* CreateMaterial(const FFbxMaterialInfo& materialInfo);

    static TMap<FString, UMaterial*>& GetMaterials() { return materialMap; }

    static UMaterial* GetMaterial(const FString& name);

    static int GetMaterialNum() { return materialMap.Num(); }

    static USkeletalMesh* CreateSkeletalMesh(const FString& filePath);

    static const TMap<FWString, USkeletalMesh*>& GetSkeletalMeshes();

    static USkeletalMesh* GetSkeletalMesh(const FWString& name);

    static int GetSkeletalMeshNum() { return SkeletalMeshMap.Num(); }

private:
    inline static TMap<FString, FSkeletalMeshRenderData*> FBXSkeletalMeshMap;
    inline static TMap<FWString, USkeletalMesh*> SkeletalMeshMap;
    inline static TMap<FString, UMaterial*> materialMap;
};
