#pragma once
#include <fbxsdk.h>
#include "Asset/StaticMeshAsset.h"

struct FSkeletonBone
{
    FString Name;
    int32 ParentIndex;
    FbxNode* Node;
    FbxAMatrix LocalBindPose;
};

struct FFbxLoader
{
    static bool LoadFBX(const FString& FBXFilePath, FStaticMeshRenderData& OutMeshData, bool bApplyCPUSkinning = true);

private:
    static void InitializeSdk();
    static void DestroySdk();

    static bool LoadScene(FString FilePath, FbxScene*& OutScene);
    static void ProcessNode(FbxNode* Node, FStaticMeshRenderData& MeshData, bool bApplyCPUSkinning);
    static void ProcessMesh(FbxMesh* Mesh, FStaticMeshRenderData& MeshData, bool bApplyCPUSkinning);
    static FVector ConvertPosition(const FbxVector4& Vec);
    static FVector ConvertNormal(const FbxVector4& Vec);
    static FVector2D ConvertUV(const FbxVector2& Vec);
    static void ComputeBoundingBox(const TArray<FStaticMeshVertex>& InVertices, FVector& OutMinVector, FVector& OutMaxVector);

private:
    inline static FbxManager* SdkManager = nullptr;
    inline static FbxImporter* Importer = nullptr;
};
