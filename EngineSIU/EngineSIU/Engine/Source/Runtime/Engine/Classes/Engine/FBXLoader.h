#pragma once
#include <fbxsdk.h>
#include "Asset/StaticMeshAsset.h"
#include "Container/Map.h"

struct FSkeletonBone
{
    FString Name;
    int32 ParentIndex;
    FbxNode* Node;
    FbxAMatrix LocalBindPose;
    FbxAMatrix GlobalBindPose;
    FbxAMatrix GlobalPose;
};

struct FFbxLoader
{
    static bool LoadFBX(const FString& FBXFilePath, 
        FStaticMeshRenderData& OutMeshData, 
        bool bApplyCPUSkinning = true,
        FbxScene** OutScene = nullptr, 
        FbxMesh** OutMesh = nullptr
    );
    static void ComputeBoundingBox(const TArray<FStaticMeshVertex>& InVertices, FVector& OutMinVector, FVector& OutMaxVector);

private:
    static void InitializeSdk();
    static void DestroySdk();

    static bool LoadScene(FString FilePath, FbxScene*& OutScene);
    static void ProcessNode(FbxNode* Node, FStaticMeshRenderData& MeshData, bool bApplyCPUSkinning);
    static void ProcessMesh(FbxMesh* Mesh, FStaticMeshRenderData& MeshData, bool bApplyCPUSkinning);
    static FVector ConvertPosition(const FbxVector4& Vec);
    static FVector ConvertNormal(const FbxVector4& Vec);
    static FVector2D ConvertUV(const FbxVector2& Vec);

private:
    inline static FbxManager* SdkManager = nullptr;
    inline static FbxImporter* Importer = nullptr;
};

// External skeleton and skinning utilities
void ExtractSkeleton(FbxMesh* Mesh, TArray<FSkeletonBone>& OutBones);
void RecalculateGlobalPoses(TArray<FSkeletonBone>& Bones);
void RotateBone(TArray<FSkeletonBone>& Bones, int32 BoneIndex, const FbxVector4& EulerDegrees);
void ReskinVerticesCPU(
    FbxMesh* Mesh,
    const TArray<FSkeletonBone>& Bones,
    TArray<FStaticMeshVertex>& Vertices,
    const TMap<int32, TArray<TPair<int32, float>>>& ControlPointToBoneWeights,
    const TArray<FStaticMeshVertex>& OriginalVertices);
//void UpdateVertexBuffer(FGraphicsSystem* Graphics, FBufferManager* BufferManager, const FWString& MeshName, const TArray<FStaticMeshVertex>& Vertices);
FbxMesh* ExtractFirstMeshFromScene(FbxNode* Node);

int32 FindBoneByName(const TArray<FSkeletonBone>& Bones, const FString& Name);

void BuildControlPointInfluenceMap(
    FbxMesh* Mesh,
    const TArray<FSkeletonBone>& Bones,
    TMap<int32, TArray<TPair<int32, float>>>& OutMap);
