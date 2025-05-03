#include "FbxLoader.h"

#include <string>


struct FSkeletonBone
{
    FString Name;
    int32 ParentIndex;
    FbxNode* Node;
    FbxAMatrix LocalBindPose;
    FbxAMatrix GlobalPose;
};

static void ExtractSkeleton(FbxMesh* Mesh, TArray<FSkeletonBone>& OutBones)
{
    OutBones.Empty();

    if (Mesh->GetDeformerCount(FbxDeformer::eSkin) == 0)
        return;

    FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));
    const int ClusterCount = Skin->GetClusterCount();

    for (int c = 0; c < ClusterCount; ++c)
    {
        FbxCluster* Cluster = Skin->GetCluster(c);
        FbxNode* BoneNode = Cluster->GetLink();
        if (!BoneNode)
            continue;

        FSkeletonBone Bone;
        Bone.Name = BoneNode->GetName();
        Bone.Node = BoneNode;
        Bone.LocalBindPose = BoneNode->EvaluateLocalTransform();
        Bone.GlobalPose.SetIdentity(); // 초기화

        // Find parent index
        FbxNode* Parent = BoneNode->GetParent();
        Bone.ParentIndex = -1;
        for (int i = 0; i < OutBones.Num(); ++i)
        {
            if (OutBones[i].Node == Parent)
            {
                Bone.ParentIndex = i;
                break;
            }
        }

        OutBones.Add(Bone);
    }
}

static void RecalculateGlobalPoses(TArray<FSkeletonBone>& Bones)
{
    for (int i = 0; i < Bones.Num(); ++i)
    {
        FSkeletonBone& Bone = Bones[i];
        if (Bone.ParentIndex == -1)
        {
            Bone.GlobalPose = Bone.LocalBindPose;
        }
        else
        {
            Bone.GlobalPose = Bones[Bone.ParentIndex].GlobalPose * Bone.LocalBindPose;
        }
    }
}

static void RotateBone(TArray<FSkeletonBone>& Bones, int32 BoneIndex, const FbxVector4& EulerDegrees)
{
    if (!Bones.IsValidIndex(BoneIndex)) return;

    FSkeletonBone& Bone = Bones[BoneIndex];

    FbxAMatrix RotationMatrix;
    RotationMatrix.SetIdentity();
    RotationMatrix.SetR(EulerDegrees); // XYZ Euler 회전값 적용 (도 단위)

    // 기존 로컬 바인드 포즈에 회전 적용
    Bone.LocalBindPose = RotationMatrix * Bone.LocalBindPose;

    // 회전 후 글로벌 포즈 갱신
    RecalculateGlobalPoses(Bones);
}

static void ReskinVerticesCPU(FbxMesh* Mesh, const TArray<FSkeletonBone>& Bones, TArray<FStaticMeshVertex>& Vertices)
{
    if (!Mesh || Bones.Num() == 0) return;

    const int ControlPointsCount = Mesh->GetControlPointsCount();
    FbxVector4* ControlPoints = Mesh->GetControlPoints();

    std::vector<FbxVector4> Skinned(ControlPointsCount);
    for (int i = 0; i < ControlPointsCount; ++i)
        Skinned[i] = ControlPoints[i];

    FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));

    for (int c = 0; c < Skin->GetClusterCount(); ++c)
    {
        FbxCluster* Cluster = Skin->GetCluster(c);
        int* Indices = Cluster->GetControlPointIndices();
        double* Weights = Cluster->GetControlPointWeights();
        int Count = Cluster->GetControlPointIndicesCount();

        FbxNode* BoneNode = Cluster->GetLink();
        int BoneIndex = Bones.IndexOfByPredicate([&](const FSkeletonBone& B) { return B.Node == BoneNode; });
        if (BoneIndex == INDEX_NONE) continue;

        FbxAMatrix TransformMatrix, ReferenceMatrix;
        Cluster->GetTransformMatrix(TransformMatrix);
        Cluster->GetTransformLinkMatrix(ReferenceMatrix);

        FbxAMatrix BoneOffsetMatrix = ReferenceMatrix.Inverse() * TransformMatrix;
        FbxAMatrix FinalMatrix = Bones[BoneIndex].GlobalPose * BoneOffsetMatrix;

        for (int i = 0; i < Count; ++i)
        {
            int ctrlIdx = Indices[i];
            double weight = Weights[i];
            if (ctrlIdx < ControlPointsCount)
            {
                FbxVector4 SkinnedDelta = (FinalMatrix.MultT(ControlPoints[ctrlIdx]) - ControlPoints[ctrlIdx]) * weight;
                Skinned[ctrlIdx] += SkinnedDelta;
            }
        }
    }

    for (auto& V : Vertices)
    {
        if ((int)V.MaterialIndex < ControlPointsCount)
        {
            FVector Pos = FVector((float)Skinned[V.MaterialIndex][0], (float)Skinned[V.MaterialIndex][1], (float)Skinned[V.MaterialIndex][2]);
            V.X = Pos.X;
            V.Y = Pos.Y;
            V.Z = Pos.Z;
        }
    }
}

static FbxMesh* ExtractFirstMeshFromScene(FbxNode* Node)
{
    if (!Node) return nullptr;
    if (Node->GetMesh()) return Node->GetMesh();

    for (int i = 0; i < Node->GetChildCount(); ++i)
    {
        FbxMesh* Mesh = ExtractFirstMeshFromScene(Node->GetChild(i));
        if (Mesh) return Mesh;
    }

    return nullptr;
}

bool FFbxLoader::LoadFBX(const FString& FBXFilePath, FStaticMeshRenderData& OutMeshData, bool bApplyCPUSkinning, FbxScene** OutScene = nullptr, FbxMesh** OutMesh = nullptr)
{
    InitializeSdk();

    FbxScene* Scene = nullptr;
    if (!LoadScene(FBXFilePath, Scene))
    {
        DestroySdk();
        return false;
    }

    FbxNode* RootNode = Scene->GetRootNode();
    if (RootNode)
    {
        ProcessNode(RootNode, OutMeshData, bApplyCPUSkinning);
    }

    ComputeBoundingBox(OutMeshData.Vertices, OutMeshData.BoundingBoxMin, OutMeshData.BoundingBoxMax);

    if (OutScene)
    {
        *OutScene = Scene;
    }
    if (OutMesh)
    {
        *OutMesh = ExtractFirstMeshFromScene(Scene->GetRootNode());
    }

    //DestroySdk(); 호출에게 넘긴 Scene을 사용해야 함.
    return true;
}

void FFbxLoader::InitializeSdk()
{
    if (!SdkManager)
    {
        SdkManager = FbxManager::Create();
        FbxIOSettings* ioSettings = FbxIOSettings::Create(SdkManager, IOSROOT);
        SdkManager->SetIOSettings(ioSettings);
    }
}

void FFbxLoader::DestroySdk()
{
    if (Importer)
    {
        Importer->Destroy();
        Importer = nullptr;
    }
    if (SdkManager)
    {
        SdkManager->Destroy();
        SdkManager = nullptr;
    }
}

bool FFbxLoader::LoadScene(FString FilePath, FbxScene*& OutScene)
{
    Importer = FbxImporter::Create(SdkManager, "");
    std::string Utf8Path = FilePath.ToAnsiString();
    if (!Importer->Initialize(Utf8Path.c_str(), -1, SdkManager->GetIOSettings()))
    {
        return false;
    }

    OutScene = FbxScene::Create(SdkManager, "Scene");
    if (!Importer->Import(OutScene))
    {
        return false;
    }

    return true;
}

void FFbxLoader::ProcessNode(FbxNode* Node, FStaticMeshRenderData& MeshData, bool bApplyCPUSkinning)
{
    if (FbxMesh* Mesh = Node->GetMesh())
    {
        ProcessMesh(Mesh, MeshData, bApplyCPUSkinning);
    }

    for (int i = 0; i < Node->GetChildCount(); ++i)
    {
        ProcessNode(Node->GetChild(i), MeshData, bApplyCPUSkinning);
    }
}

void FFbxLoader::ProcessMesh(FbxMesh* Mesh, FStaticMeshRenderData& MeshData, bool bApplyCPUSkinning)
{
    const int ControlPointsCount = Mesh->GetControlPointsCount();
    FbxVector4* ControlPoints = Mesh->GetControlPoints();

    // [Optional] CPU Skinning 적용
    std::vector<FbxVector4> SkinnedPositions(ControlPointsCount);
    for (int i = 0; i < ControlPointsCount; ++i)
        SkinnedPositions[i] = ControlPoints[i];

    if (bApplyCPUSkinning && Mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
    {
        FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));
        int ClusterCount = Skin->GetClusterCount();

        std::vector<FbxAMatrix> ClusterMatrices(ClusterCount);

        for (int c = 0; c < ClusterCount; ++c)
        {
            FbxCluster* Cluster = Skin->GetCluster(c);

            FbxAMatrix BindPoseMatrix, ReferenceMatrix, TransformMatrix;
            Cluster->GetTransformMatrix(TransformMatrix);
            Cluster->GetTransformLinkMatrix(ReferenceMatrix);
            BindPoseMatrix = ReferenceMatrix.Inverse() * TransformMatrix;
            ClusterMatrices[c] = BindPoseMatrix;
        }

        for (int c = 0; c < ClusterCount; ++c)
        {
            FbxCluster* Cluster = Skin->GetCluster(c);
            int* indices = Cluster->GetControlPointIndices();
            double* weights = Cluster->GetControlPointWeights();
            int idxCount = Cluster->GetControlPointIndicesCount();

            for (int i = 0; i < idxCount; ++i)
            {
                int ctrlIdx = indices[i];
                double weight = weights[i];
                if (ctrlIdx < ControlPointsCount)
                {
                    SkinnedPositions[ctrlIdx] += (ClusterMatrices[c].MultT(ControlPoints[ctrlIdx]) - ControlPoints[ctrlIdx]) * weight;
                }
            }
        }
    }

    int PolygonCount = Mesh->GetPolygonCount();
    FbxGeometryElementUV* UVElement = Mesh->GetElementUV();

    for (int polyIndex = 0; polyIndex < PolygonCount; ++polyIndex)
    {
        for (int vertIndex = 0; vertIndex < 3; ++vertIndex)
        {
            int ctrlPointIndex = Mesh->GetPolygonVertex(polyIndex, vertIndex);
            FbxVector4 pos = SkinnedPositions[ctrlPointIndex];

            FStaticMeshVertex vertex = {};
            FVector position = ConvertPosition(pos);
            vertex.X = position.X;
            vertex.Y = position.Y;
            vertex.Z = position.Z;

            vertex.R = vertex.G = vertex.B = 0.7f;
            vertex.A = 1.0f;

            FbxVector4 normal;
            Mesh->GetPolygonVertexNormal(polyIndex, vertIndex, normal);
            FVector normalVec = ConvertNormal(normal);
            vertex.NormalX = normalVec.X;
            vertex.NormalY = normalVec.Y;
            vertex.NormalZ = normalVec.Z;

            if (UVElement && UVElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
            {
                int uvIndex = Mesh->GetTextureUVIndex(polyIndex, vertIndex);
                if (uvIndex >= 0)
                {
                    FbxVector2 uv = UVElement->GetDirectArray().GetAt(uvIndex);
                    FVector2D uvVec = ConvertUV(uv);
                    vertex.U = uvVec.X;
                    vertex.V = uvVec.Y;
                }
            }

            vertex.MaterialIndex = 0; // 임시. 머티리얼 인덱스 설정 필요

            MeshData.Indices.Add(MeshData.Vertices.Num());
            MeshData.Vertices.Add(vertex);
        }
    }
}

FVector FFbxLoader::ConvertPosition(const FbxVector4& Vec)
{
    return FVector((float)Vec[0], (float)Vec[1], (float)Vec[2]);
}

FVector FFbxLoader::ConvertNormal(const FbxVector4& Vec)
{
    return FVector((float)Vec[0], (float)Vec[1], (float)Vec[2]).GetSafeNormal();
}

FVector2D FFbxLoader::ConvertUV(const FbxVector2& Vec)
{
    return FVector2D((float)Vec[0], 1.0f - (float)Vec[1]);
}

void FFbxLoader::ComputeBoundingBox(const TArray<FStaticMeshVertex>& InVertices, FVector& OutMinVector, FVector& OutMaxVector)
{
    FVector MinVector = { FLT_MAX, FLT_MAX, FLT_MAX };
    FVector MaxVector = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (const auto& V : InVertices)
    {
        MinVector.X = std::min(MinVector.X, V.X);
        MinVector.Y = std::min(MinVector.Y, V.Y);
        MinVector.Z = std::min(MinVector.Z, V.Z);

        MaxVector.X = std::max(MaxVector.X, V.X);
        MaxVector.Y = std::max(MaxVector.Y, V.Y);
        MaxVector.Z = std::max(MaxVector.Z, V.Z);
    }

    OutMinVector = MinVector;
    OutMaxVector = MaxVector;
}
