#pragma once
#include "HAL/PlatformType.h"
#include "Container/String.h"
#include "Container/Array.h"
#include "Math/Matrix.h"
#include "Math/Plane.h"
#include "Container/Map.h"
#include "fbxsdk.h"


// Explicitly qualify FbxAxisSystem to resolve ambiguity
inline fbxsdk::FbxAxisSystem unrealAxis(
    fbxsdk::FbxAxisSystem::eZAxis,      // UpVector = Z axis
    fbxsdk::FbxAxisSystem::eParityOdd,  // FrontVector = +X axis
    fbxsdk::FbxAxisSystem::eLeftHanded  // 좌표계 = Left-Handed
);

struct FBone;
struct FSkeletalMeshVertex;
struct FSkeletalMeshBoneWeight;
struct FMaterialSubset;
struct FStaticMeshVertex;
struct FStaticMeshRenderData;
struct FSkeletalMeshRenderData;
class UStaticMesh;
class USkeletalMesh;
class UMaterial;

static FMatrix FbxAMatrixToFMatrix(const FbxAMatrix& InM)
{
    FMatrix Out;

    // Row 0
    Out.M[0][0] = (float)InM.mData[0][0];
    Out.M[0][1] = (float)InM.mData[0][1];
    Out.M[0][2] = (float)InM.mData[0][2];
    Out.M[0][3] = (float)InM.mData[0][3];

    // Row 1
    Out.M[1][0] = (float)InM.mData[1][0];
    Out.M[1][1] = (float)InM.mData[1][1];
    Out.M[1][2] = (float)InM.mData[1][2];
    Out.M[1][3] = (float)InM.mData[1][3];

    // Row 2
    Out.M[2][0] = (float)InM.mData[2][0];
    Out.M[2][1] = (float)InM.mData[2][1];
    Out.M[2][2] = (float)InM.mData[2][2];
    Out.M[2][3] = (float)InM.mData[2][3];

    // Row 3
    Out.M[3][0] = (float)InM.mData[3][0];
    Out.M[3][1] = (float)InM.mData[3][1];
    Out.M[3][2] = (float)InM.mData[3][2];
    Out.M[3][3] = (float)InM.mData[3][3];

    return Out;
}

struct FFBXLoader
{
public:
    FFBXLoader() = default;
    ~FFBXLoader();

    static bool Initialize();
    static bool LoadFBX(const FString& FilePath);
    static void DumpAllMeshes(FbxNode* node);
    static bool FindMesh(FbxNode* Node, const FString& FilePath);

    static bool IsStaticMesh(FbxMesh* Mesh);
    static bool IsSkeletalMesh(FbxMesh* Mesh);

    static void BuildSkeletalBones(FbxMesh* Mesh, TArray<FBone>& OutBones);
    static void BuildBoneWeights(FbxMesh* Mesh, TArray<FSkeletalMeshBoneWeight>& OutWeights);
    static void BuildSkeletalVertexBuffers(FbxMesh* Mesh, TArray<FSkeletalMeshVertex>& OutVerts, TArray<uint32>& OutIndices);
    static void SetupMaterialSubsets(FbxMesh* Mesh, TArray<FMaterialSubset>& OutSubsets);
    static void UpdateSkinningMatrices(const TArray<FMatrix>& GlobalBoneTransforms, TArray<FBone>& Bones);

    static void CopyControlPoints(FbxMesh* Mesh,TArray<FStaticMeshVertex>& OutVerts);
    static void BuildStaticIndexBuffer(FbxMesh* Mesh, TArray<uint32>& OutIndices);
    static void CopyNormals(FbxMesh* Mesh, TArray<FStaticMeshVertex>& OutVerts);
    static void CopyUVs(FbxMesh* Mesh, TArray<FStaticMeshVertex>& OutVerts);
    static void CopyTangents(FbxMesh* Mesh, TArray<FStaticMeshVertex>& OutVerts);
    static void ComputeBoundingBox(const TArray<FStaticMeshVertex>& InVerts, FVector& OutMin, FVector& OutMax);

    static void CopyNormals(FbxMesh* Mesh, TArray<FSkeletalMeshVertex>& OutVerts);
    static void CopyUVs(FbxMesh* Mesh, TArray<FSkeletalMeshVertex>& OutVerts);
    static void CopyTangents(FbxMesh* Mesh, TArray<FSkeletalMeshVertex>& OutVerts);
    static void ComputeBoundingBox(const TArray<FSkeletalMeshVertex>& InVerts, FVector& OutMin, FVector& OutMax);

private:
    inline static FbxManager* Manager = nullptr;
    inline static FbxImporter* Importer = nullptr;
    inline static FbxScene* Scene = nullptr;
    inline static FbxMesh* Mesh = nullptr;

};

struct FFBXManager
{
public:
    static UStaticMesh* CreateStaticMesh(const FString& filePath);
    static UStaticMesh* GetStaticMesh(FWString name);
    static USkeletalMesh* CreateSkeletalMesh(const FString& filePath);
    static USkeletalMesh* GetSkeletalMesh(FWString name);

    inline static TMap<FString, FStaticMeshRenderData*> FbxStaticMeshMap;
    inline static TMap<FWString, UStaticMesh*> StaticMeshMap;
    inline static TMap<FString, UMaterial*> MaterialMap;
    inline static FStaticMeshRenderData* StaticMeshRenderData = nullptr;
    inline static FSkeletalMeshRenderData* SkeletalMeshRenderData = nullptr;
};

