#include "FbxLoader.h"

#include <string>



void ExtractSkeleton(FbxMesh* Mesh, TArray<FSkeletonBone>& OutBones)
{
    OutBones.Empty();

    if (!Mesh) return;

    FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));
    if (!Skin) return;

    FbxCluster::ELinkMode LinkMode = FbxCluster::eTotalOne;
    int ClusterCount = Skin->GetClusterCount();

    TMap<FbxNode*, int32> NodeToIndex;

    // 1. 수집 단계
    for (int i = 0; i < ClusterCount; ++i)
    {
        FbxCluster* Cluster = Skin->GetCluster(i);
        FbxNode* Node = Cluster->GetLink(); // 본 노드

        if (!Node || NodeToIndex.Contains(Node))
            continue;

        FSkeletonBone Bone;
        Bone.Name = Node->GetName();
        Bone.Node = Node;

        FbxNode* Parent = Node->GetParent();
        Bone.ParentIndex = Parent && NodeToIndex.Contains(Parent)
            ? NodeToIndex[Parent]
            : -1;

        // 2. 바인드 포즈 저장
        Bone.LocalBindPose = Node->EvaluateLocalTransform();
        Bone.GlobalBindPose = Node->EvaluateGlobalTransform(); //  추가

        OutBones.Add(Bone);
        NodeToIndex.Add(Node, OutBones.Num() - 1);
    }

    // 3. 디버그용 본 트리 출력
    for (int i = 0; i < OutBones.Num(); ++i)
    {
        const FSkeletonBone& Bone = OutBones[i];
        UE_LOG(ELogLevel::Display, TEXT("[%2d] %s (Parent: %d)"), i, *Bone.Name, Bone.ParentIndex);
    }
}


void RecalculateGlobalPoses(TArray<FSkeletonBone>& Bones)
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

void RotateBone(TArray<FSkeletonBone>& Bones, int32 BoneIndex, const FbxVector4& EulerDegrees)
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

void ReskinVerticesCPU(
    FbxMesh* Mesh,
    const TArray<FSkeletonBone>& Bones,
    TArray<FStaticMeshVertex>& Vertices,
    const TMap<int32, TArray<TPair<int32, float>>>& ControlPointToBoneWeights)
{
    for (FStaticMeshVertex& Vertex : Vertices)
    {
        const int32 CtrlIdx = Vertex.ControlPointIndex;

        if (!ControlPointToBoneWeights.Contains(CtrlIdx))
            continue;

        const TArray<TPair<int32, float>>& Influences = ControlPointToBoneWeights[CtrlIdx];

        FbxVector4 Skinned = FbxVector4(0, 0, 0, 1);
        float TotalWeight = 0.f;

        FbxVector4 OrigPos(Vertex.X, Vertex.Y, Vertex.Z, 1.0);

        for (const TPair<int32, float>& Influence : Influences)
        {
            int BoneIndex = Influence.Key;
            float Weight = Influence.Value;

            if (!Bones.IsValidIndex(BoneIndex)) continue;

            const FbxAMatrix& GlobalPose = Bones[BoneIndex].GlobalPose;
            const FbxAMatrix& BindPose = Bones[BoneIndex].GlobalBindPose;
            FbxAMatrix OffsetMatrix = GlobalPose * BindPose.Inverse();

            Skinned += OffsetMatrix.MultT(OrigPos) * Weight;
            TotalWeight += Weight;
        }

        if (TotalWeight > 0.0)
        {
            Skinned /= TotalWeight;
            Vertex.X = static_cast<float>(Skinned[0]);
            Vertex.Y = static_cast<float>(Skinned[1]);
            Vertex.Z = static_cast<float>(Skinned[2]);
        }
    }
}


FbxMesh* ExtractFirstMeshFromScene(FbxNode* Node)
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

int32 FindBoneByName(const TArray<FSkeletonBone>& Bones, const FString& Name)
{
    for (int32 i = 0; i < Bones.Num(); ++i)
    {
        if (Bones[i].Name == Name)
            return i;
    }
    return -1;
}

void BuildControlPointInfluenceMap(FbxMesh* Mesh, const TArray<FSkeletonBone>& Bones, TMap<int32, TArray<TPair<int32, float>>>& OutMap)
{
    OutMap.Empty();

    if (!Mesh) return;

    FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));
    if (!Skin) return;

    for (int ClusterIndex = 0; ClusterIndex < Skin->GetClusterCount(); ++ClusterIndex)
    {
        FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
        FbxNode* BoneNode = Cluster->GetLink();

        int32 BoneIndex = Bones.IndexOfByPredicate([BoneNode](const FSkeletonBone& B) {
            return B.Node == BoneNode;
            });

        if (BoneIndex == INDEX_NONE)
            continue;

        int* Indices = Cluster->GetControlPointIndices();
        double* Weights = Cluster->GetControlPointWeights();
        int Count = Cluster->GetControlPointIndicesCount();

        for (int i = 0; i < Count; ++i)
        {
            int CtrlIdx = Indices[i];
            float Weight = static_cast<float>(Weights[i]);

            OutMap.FindOrAdd(CtrlIdx).Add({ BoneIndex, Weight });
        }
    }
}

bool FFbxLoader::LoadFBX(const FString& FBXFilePath, FStaticMeshRenderData& OutMeshData, bool bApplyCPUSkinning, FbxScene** OutScene, FbxMesh** OutMesh)
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
                    //SkinnedPositions[ctrlIdx] += (ClusterMatrices[c].MultT(ControlPoints[ctrlIdx]) - ControlPoints[ctrlIdx]) * weight;
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
            vertex.ControlPointIndex = ctrlPointIndex;
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
