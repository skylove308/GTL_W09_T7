#include "FbxLoader.h"
#include "UserInterface/Console.h"
#include "Define.h"
#include "Asset/SkeletalMeshAsset.h"
#include "Asset/StaticMeshAsset.h"
#include "UObject/ObjectFactory.h"
#include "Components/Mesh/StaticMeshRenderData.h"
#include "Components/Mesh/SkeletalMeshRenderData.h"
#include "FObjLoader.h"
#include "Math/JungleMath.h"

FFBXLoader::~FFBXLoader()
{
    if (FFBXManager::SkeletalMeshRenderData)
    {
        delete FFBXManager::SkeletalMeshRenderData;
        FFBXManager::SkeletalMeshRenderData = nullptr;
    }

    if (FFBXManager::StaticMeshRenderData)
    {
        delete FFBXManager::StaticMeshRenderData;
        FFBXManager::StaticMeshRenderData = nullptr;
    }

    if (Manager)
    {
        Manager->Destroy();
        Manager = nullptr;
    }
    if (Importer)
    {
        Importer->Destroy();
        Importer = nullptr;
    }
    if (Scene)
    {
        Scene->Destroy();
        Scene = nullptr;
    }
    if (Mesh)
    {
        Mesh->Destroy();
        Mesh = nullptr;
    }
}

bool FFBXLoader::Initialize()
{
    // Initialize the FBX SDK manager
    Manager = FbxManager::Create();
    if (!Manager)
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create FBX Manager"));
        return false;
    }
    // Create an IO settings object
    FbxIOSettings* IOSettings = FbxIOSettings::Create(Manager, IOSROOT);
    if (!IOSettings)
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create FBX IOSettings"));
        return false;
    }
    Manager->SetIOSettings(IOSettings);

    // Create an importer
    Importer = FbxImporter::Create(Manager, "");
    if (!Importer)
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create FBX Importer"));
        return false;
    }

    // Create a new scene
    Scene = FbxScene::Create(Manager, "MyScene");
    if (!Scene)
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create FBX Scene"));
        return false;
    }

    return true;
}

bool FFBXLoader::LoadFBX(const FString& FilePath)
{
   // Initialize the importer with the file path
   if (!Importer->Initialize(*FilePath, -1, Manager->GetIOSettings()))
   {
       UE_LOG(ELogLevel::Error, TEXT("Failed to initialize FBX importer: %s"), *FString(Importer->GetStatus().GetErrorString()));
       Importer->Destroy();
       Scene->Destroy();
       Manager->Destroy();
       return false;
   }

   // Import the scene from the file
   if (!Importer->Import(Scene))
   {
       UE_LOG(ELogLevel::Error, TEXT("Failed to import FBX scene: %s"), *FString(Importer->GetStatus().GetErrorString()));
       Importer->Destroy();
       Scene->Destroy();
       Manager->Destroy();
       return false;
   }

   // 언리얼 좌표계로 변경
   unrealAxis.ConvertScene(Scene);

   // 삼각형화할 수 있는 노드를 삼각형화 시키기
   FbxGeometryConverter Converter(Manager);
   Converter.Triangulate(Scene, true);

   DumpAllMeshes(Scene->GetRootNode());
   if (!FindMesh(Scene->GetRootNode(), FilePath))
   {
       UE_LOG(ELogLevel::Error, TEXT("Failed to find Mesh in FBX scene"));
       Importer->Destroy();
       Scene->Destroy();
       Manager->Destroy();
       return false;
   }

   // Cleanup
   Importer->Destroy();
   Scene->Destroy();
   Manager->Destroy();

   UE_LOG(ELogLevel::Display, TEXT("FBX file loaded successfully: %s"), *FilePath);

   return true;
}

void FFBXLoader::DumpAllMeshes(FbxNode* node)
{
    if (!node) return;
    if (auto* mesh = node->GetMesh())
    {
        UE_LOG(ELogLevel::Display, TEXT("Found Mesh: %s, CP=%d, Poly=%d"),
            node->GetName(),
            mesh->GetControlPointsCount(),
            mesh->GetPolygonCount());
    }
    int MatCount = node->GetMaterialCount();
    UE_LOG(ELogLevel::Display,
        TEXT("[%s] MaterialCount = %d"),
        node->GetName(),
        MatCount
    );
    for (int i = 0; i < node->GetChildCount(); ++i)
        DumpAllMeshes(node->GetChild(i));
}

bool FFBXLoader::FindMesh(FbxNode* Node, const FString& FilePath)
{
    if (!Node) return false;

    const FbxNodeAttribute* Attribute = Node->GetNodeAttribute();

    if (Attribute && Attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
    {
        Mesh = Node->GetMesh();

        // Skeletal Mesh
        if (IsSkeletalMesh(Mesh))
        {
            FFBXManager::SkeletalMeshRenderData->ObjectName = FilePath.ToWideString();
            // Build bones and weights
            //BuildSkeletalBones(Mesh, FFBXManager::SkeletalMeshRenderData->Bones);
            ExtractSkeleton(Mesh, FFBXManager::Bones);
            RecalculateGlobalPoses(FFBXManager::Bones);
            //BuildBoneWeights(Mesh, FFBXManager::SkeletalMeshRenderData->BoneWeights);
            BuildSkeletalVertexBuffers(Mesh, FFBXManager::SkeletalMeshRenderData->Vertices, FFBXManager::SkeletalMeshRenderData->Indices);
            ReskinVerticesCPU(Mesh,
                FFBXManager::Bones,
                FFBXManager::SkeletalMeshRenderData->Vertices);
            SetupMaterialSubsets(Mesh, FFBXManager::SkeletalMeshRenderData->MaterialSubsets);
            LoadMaterialInfo(Node);

            //int32 LeftArmIndex = FindBoneByName(FFBXManager::Bones, "mixamorig:Spine");
            //if (LeftArmIndex != INDEX_NONE)
            //{
            //    RotateBones(FFBXManager::Bones, LeftArmIndex, FbxVector4(90, 0, 0)); // Z축으로 30도 회전
            //    ReskinVerticesCPU(Mesh,
            //        FFBXManager::Bones,
            //        FFBXManager::SkeletalMeshRenderData->Vertices);
            //}

            //int32 LeftUpLegIndex = FindBoneByName(FFBXManager::Bones, "mixamorig:RightHand");
            //if (LeftUpLegIndex != INDEX_NONE)
            //{
            //    RotateBones(FFBXManager::Bones, LeftUpLegIndex, FbxVector4(30, 0, 0)); // Z축으로 30도 회전
            //    ReskinVerticesCPU(Mesh,
            //        FFBXManager::Bones,
            //        FFBXManager::SkeletalMeshRenderData->Vertices);
            //}

            //// Update skinning matrices
            //TArray<FMatrix> GlobalBoneTransforms;
            //for (int i = 0; i < FFBXManager::SkeletalMeshRenderData->Bones.Num(); ++i)
            //{
            //    FbxAMatrix GlobalTransform = Node->EvaluateGlobalTransform();
            //    GlobalBoneTransforms.Add(FbxAMatrixToFMatrix(GlobalTransform));
            //}
            //UpdateSkinningMatrices(GlobalBoneTransforms, FFBXManager::SkeletalMeshRenderData->Bones);

        }
        // Static Mesh
        else
        {
            FFBXManager::StaticMeshRenderData->ObjectName = FilePath.ToWideString();
            // 1) 컨트롤 포인트(버텍스 위치) 복사
            CopyControlPoints(Mesh, FFBXManager::StaticMeshRenderData->Vertices);

            // 2) 폴리곤 → 인덱스 빌드
            BuildStaticIndexBuffer(Mesh, FFBXManager::StaticMeshRenderData->Indices);

            // 3) 쉐이딩 데이터 복사
            CopyNormals(Mesh, FFBXManager::StaticMeshRenderData->Vertices);
            CopyUVs(Mesh, FFBXManager::StaticMeshRenderData->Vertices);
            CopyTangents(Mesh, FFBXManager::StaticMeshRenderData->Vertices);

            // 4) 머티리얼 서브셋 설정
            SetupMaterialSubsets(Mesh, FFBXManager::StaticMeshRenderData->MaterialSubsets);
            LoadMaterialInfo(Node);

            // 5) 바운딩 박스 계산
            ComputeBoundingBox(
                FFBXManager::StaticMeshRenderData->Vertices,
                FFBXManager::StaticMeshRenderData->BoundingBoxMin,
                FFBXManager::StaticMeshRenderData->BoundingBoxMax
            );


        }
        
        return true;
    }

    for (int i = 0; i < Node->GetChildCount(); i++)
    {
        if (FindMesh(Node->GetChild(i), FilePath))
        {
            return true;
        }
    }

    return false;
}

bool FFBXLoader::IsStaticMesh(FbxMesh* Mesh)
{
    return Mesh->GetDeformerCount(FbxDeformer::eSkin) == 0;
}

bool FFBXLoader::IsSkeletalMesh(FbxMesh* Mesh)
{
    return Mesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
}

//-----------------------------------------------------------------------------
// 1) 본 배열 채우기: FbxSkin → FbxCluster 순회, inverse bind pose 계산해서 Bones[i].skinningMatrix 에 저장
//-----------------------------------------------------------------------------
void FFBXLoader::BuildSkeletalBones(FbxMesh* Mesh, TArray<FBone>& OutBones)
{
    // 스킨 디포머가 없으면 패스
    if (Mesh->GetDeformerCount(FbxDeformer::eSkin) == 0)
        return;

    // 보통 한 개의 Skin deformer만 처리
    FbxSkin* skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));
    int clusterCount = skin->GetClusterCount();

    OutBones.SetNum(clusterCount);
    for (int c = 0; c < clusterCount; ++c)
    {
        FbxCluster* cluster = skin->GetCluster(c);

        // 메시 바인드(TransformMatrix)와 본 바인드(TransformLinkMatrix)
        FbxAMatrix MeshBind, boneBind;
        cluster->GetTransformMatrix(MeshBind);
        cluster->GetTransformLinkMatrix(boneBind);
        FbxAMatrix invBindPose = boneBind.Inverse() * MeshBind;

        // Matrix4x4 로 변환해서 초기 skinningMatrix 로 저장
        OutBones[c].SkinningMatrix = FbxAMatrixToFMatrix(invBindPose);
    }
}

//-----------------------------------------------------------------------------
// 2) 정점당 본 가중치 채우기: FbxCluster → BoneWeights
//-----------------------------------------------------------------------------
void FFBXLoader::BuildBoneWeights(FbxMesh* Mesh, TArray<FSkeletalMeshBoneWeight>& OutWeights)
{
    int ctrlCount = Mesh->GetControlPointsCount();
    OutWeights.SetNum(ctrlCount);

    if (Mesh->GetDeformerCount(FbxDeformer::eSkin) == 0)
        return;

    FbxSkin* skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));
    int clusterCount = skin->GetClusterCount();

    for (int c = 0; c < clusterCount; ++c)
    {
        FbxCluster* cluster = skin->GetCluster(c);
        const auto& indices = cluster->GetControlPointIndices();
        const auto& weights = cluster->GetControlPointWeights();
        int count = cluster->GetControlPointIndicesCount();

        for (int i = 0; i < count; ++i)
        {
            int vertID = indices[i];
            float w = weights[i];

            // 최대 4개 슬롯에 채우기
            auto& BW = OutWeights[vertID];
            for (int k = 0; k < 4; ++k)
            {
                if (BW.Weights[k] == 0.0f)
                {
                    BW.BoneIndices[k] = c;
                    BW.Weights[k] = w;
                    break;
                }
            }
        }
    }

    for (auto& BW : OutWeights)
    {
        float total = BW.Weights[0] + BW.Weights[1] + BW.Weights[2] + BW.Weights[3];
        if (total > 0.0f)
        {
            BW.Weights[0] /= total;
            BW.Weights[1] /= total;
            BW.Weights[2] /= total;
            BW.Weights[3] /= total;
        }
    }
}

//-----------------------------------------------------------------------------
// 3) 정점·인덱스 버퍼 채우기 (버텍스 포지션만 예시, 쉐이딩 데이터는 필요시 추가)
//-----------------------------------------------------------------------------
void FFBXLoader::BuildSkeletalVertexBuffers(FbxMesh* Mesh, TArray<FSkeletalMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
    OutVerts.Empty();
    OutIndices.Empty();

    // UV 세트 이름 가져오기
    FbxStringList uvSetNames;
    Mesh->GetUVSetNames(uvSetNames);
    const char* uvSetName = nullptr;
    if (uvSetNames.GetCount() > 0)
    {
        uvSetName = uvSetNames.GetStringAt(0);
    }

    // 탄젠트 레이어 가져오기 (있다면)
    FbxGeometryElementTangent* TanElem = nullptr;
    if (Mesh->GetElementTangentCount() > 0)
        TanElem = Mesh->GetElementTangent(0);

    TArray<FSkeletalMeshBoneWeight> const& BoneWeights = FFBXManager::SkeletalMeshRenderData->BoneWeights;

    int polyCount = Mesh->GetPolygonCount();
    int vertexCounter = 0; // DirectArray 인덱스용(탄젠트, 노말, UV 모두 ByPolygonVertex/direct 가정)

    // 각 폴리곤의 각 코너(버텍스)마다 고유한 FSkeletalMeshVertex를 생성
    for (int p = 0; p < polyCount; ++p)
    {
        int polySize = Mesh->GetPolygonSize(p);
        for (int v = 0; v < polySize; ++v)
        {
            // 새 인덱스
            uint32 newIdx = OutVerts.Num();
            // 기본값으로 초기화된 정점 구조체 추가
            OutVerts.Add(FSkeletalMeshVertex());
            FSkeletalMeshVertex& Vert = OutVerts[newIdx];

            // 1) 위치
            int cpIndex = Mesh->GetPolygonVertex(p, v);
            Vert.ControlPointIndex = cpIndex;
            FbxVector4 P = Mesh->GetControlPointAt(cpIndex);
            Vert.X = static_cast<float>(P[0]);
            Vert.Y = static_cast<float>(P[1]);
            Vert.Z = static_cast<float>(P[2]);

            // 2) 노말
            FbxVector4 N;
            Mesh->GetPolygonVertexNormal(p, v, N);
            Vert.NormalX = static_cast<float>(N[0]);
            Vert.NormalY = static_cast<float>(N[1]);
            Vert.NormalZ = static_cast<float>(N[2]);

            // 3) UV
            if (uvSetName)
            {
                FbxVector2 UV;
                bool unmapped = false;
                Mesh->GetPolygonVertexUV(p, v, uvSetName, UV, unmapped);
                Vert.U = static_cast<float>(UV[0]);
                Vert.V = 1.0f - static_cast<float>(UV[1]);
            }


            // e) 탄젠트 (NormalMap 용)
            if (TanElem &&
                TanElem->GetMappingMode() == FbxGeometryElement::eByPolygonVertex &&
                TanElem->GetReferenceMode() == FbxGeometryElement::eDirect)
            {
                auto T = TanElem->GetDirectArray().GetAt(vertexCounter);
                Vert.TangentX = (float)T[0];
                Vert.TangentY = (float)T[1];
                Vert.TangentZ = (float)T[2];
                // W 성분(역정규화) 필요하면 T[3] 사용
                Vert.TangentW = (float)T[3];
            }

            // 인덱스 추가
            OutIndices.Add(newIdx);
        }
    }
}

//-----------------------------------------------------------------------------
// 4) 머티리얼 서브셋 설정 (FbxGeometryElementMaterial)
//-----------------------------------------------------------------------------
void FFBXLoader::SetupMaterialSubsets(FbxMesh* Mesh, TArray<FMaterialSubset>& OutSubsets)
{
    auto* MatElem = Mesh->GetElementMaterial();
    if (!MatElem) return;

    int polyCount = Mesh->GetPolygonCount();
    TMap<int, FMaterialSubset> subsetMap;

    // 폴리곤 단위로 ID 집계
    for (int p = 0; p < polyCount; ++p)
    {
        int MatID = MatElem->GetIndexArray().GetAt(p);
        auto& Sub = subsetMap.FindOrAdd(MatID);
        if (Sub.MaterialIndex == INDEX_NONE)
        {
            Sub.MaterialIndex = MatID;
            Sub.IndexStart = OutSubsets.Num(); // 나중엔 정확히 재계산해도 무방
            Sub.IndexCount = 0;
        }
        Sub.IndexCount += Mesh->GetPolygonSize(p);
    }

    // TMap → TArray
    OutSubsets.Empty();
    for (auto& Pair : subsetMap)
        OutSubsets.Add(Pair.Value);
}

void FFBXLoader::LoadMaterialInfo(FbxNode* Node)
{
    int MaterialCount = Node->GetMaterialCount();
    for (int m = 0; m < MaterialCount; ++m)
    {
        FbxSurfaceMaterial* Material = Node->GetMaterial(m);
        FObjMaterialInfo MatInfo;

        if (Material)
        {
            MatInfo.MaterialName = Material->GetName();
            FbxProperty DiffuseProperty = Material->FindProperty(FbxSurfaceMaterial::sDiffuse);
            if (DiffuseProperty.IsValid())
            {
                int TextureCount = DiffuseProperty.GetSrcObjectCount<FbxFileTexture>();
                if (TextureCount > 0)
                {
                    FbxFileTexture* Texture = DiffuseProperty.GetSrcObject<FbxFileTexture>(0);
                    const uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Diffuse);
                    constexpr uint32 TexturesNum = static_cast<uint32>(EMaterialTextureSlots::MTS_MAX);
                    MatInfo.TextureInfos.SetNum(TexturesNum);
                    MatInfo.TextureInfos[SlotIdx].TexturePath = ((FString)Texture->GetFileName()).ToWideString();
                    if (FObjLoader::CreateTextureFromFile(MatInfo.TextureInfos[SlotIdx].TexturePath, true))
                    {
                        MatInfo.TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Diffuse);
                    }
                }
            }
        }
        FFBXManager::SkeletalMeshRenderData->Materials.Add(MatInfo);
    }
}

//-----------------------------------------------------------------------------
// 5) 스킨닝 매트릭스 매 프레임 갱신
//    GlobalBoneTransforms[i] 는 애니메이션 시스템이 계산한 월드 본 트랜스폼
//-----------------------------------------------------------------------------
void FFBXLoader::UpdateSkinningMatrices(const TArray<FMatrix>& GlobalBoneTransforms, TArray<FBone>& Bones)
{
    int count = FMath::Min(GlobalBoneTransforms.Num(), Bones.Num());
    for (int i = 0; i < count; ++i)
    {
        // 최종 skinningMatrix = GlobalTransform * inverseBindPose
        Bones[i].SkinningMatrix = GlobalBoneTransforms[i] * Bones[i].SkinningMatrix;
    }
}

void FFBXLoader::ExtractSkeleton(FbxMesh* Mesh, TArray<FSkeletonBone>& OutBones)
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
        Bone.LocalBindPose = FbxAMatrixToFMatrix(BoneNode->EvaluateLocalTransform());
        Bone.GlobalPose = FMatrix::Identity; // 초기화

        // Find parent index
        FbxNode* Parent = BoneNode->GetParent();
        FString ParentName = Parent ? Parent->GetName() : TEXT("");

        Bone.ParentIndex = -1;
        for (int i = 0; i < OutBones.Num(); ++i)
        {
            if (OutBones[i].Name == ParentName)
            {
                Bone.ParentIndex = i;
                break;
            }
        }

        OutBones.Add(Bone);
    }

}

void FFBXLoader::RecalculateGlobalPoses(TArray<FSkeletonBone>& Bones)
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

void FFBXLoader::RotateBones(TArray<FSkeletonBone>& Bones, int32 BoneIndex, const FbxVector4& EulerDegrees)
{
    if (!Bones.IsValidIndex(BoneIndex)) return;

    FSkeletonBone& Bone = Bones[BoneIndex];

    FMatrix Rotation = JungleMath::CreateRotationMatrix(FVector(
        (float)EulerDegrees[0],
        (float)EulerDegrees[1],
        (float)EulerDegrees[2]));

    // 기존 로컬 바인드 포즈에 회전 적용
    Bone.LocalBindPose = Rotation * Bone.LocalBindPose;

    // 회전 후 글로벌 포즈 갱신
    RecalculateGlobalPoses(Bones);
}

void FFBXLoader::ReskinVerticesCPU(FbxMesh* Mesh, const TArray<FSkeletonBone>& Bones, TArray<FSkeletalMeshVertex>& Vertices)
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
        FString BoneName = BoneNode->GetName();
        int BoneIndex = Bones.IndexOfByPredicate([&](const FSkeletonBone& B) { return B.Name == BoneName; });
        if (BoneIndex == INDEX_NONE) continue;

        FbxAMatrix TransformMatrix, ReferenceMatrix;
        Cluster->GetTransformMatrix(TransformMatrix);
        Cluster->GetTransformLinkMatrix(ReferenceMatrix);

        FbxAMatrix BoneOffsetMatrix = ReferenceMatrix.Inverse() * TransformMatrix;
        FbxAMatrix FinalMatrix = FMatrixToFbxAMatrix(Bones[BoneIndex].GlobalPose) * BoneOffsetMatrix;

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
        if ((int)V.ControlPointIndex < ControlPointsCount)
        {
            //FVector Pos = FVector((float)Skinned[V.MaterialIndex][0], (float)Skinned[V.MaterialIndex][1], (float)Skinned[V.MaterialIndex][2]);
            //V.X = Pos.X;
            //V.Y = Pos.Y;
            //V.Z = Pos.Z;
            FbxVector4 P = Skinned[V.ControlPointIndex];
            V.X = (float)P[0];
            V.Y = (float)P[1];
            V.Z = (float)P[2];
        }
    }
}

int32 FFBXLoader::FindBoneByName(const TArray<FSkeletonBone>& Bones, const FString& Name)
{
    for (int32 i = 0; i < Bones.Num(); ++i)
    {
        if (Bones[i].Name == Name)
            return i;
    }
    return -1;
}

void FFBXLoader::CopyControlPoints(FbxMesh* Mesh, TArray<FStaticMeshVertex>& OutVerts)
{
    int32 ControlPointCount = Mesh->GetControlPointsCount();
    OutVerts.SetNum(ControlPointCount);
    const FbxVector4* ControlPoints = Mesh->GetControlPoints();
    for (int32 i = 0; i < ControlPointCount; ++i)
    {
        auto& V = OutVerts[i];
        V.X = static_cast<float>(ControlPoints[i][0]);
        V.Y = static_cast<float>(ControlPoints[i][1]);
        V.Z = static_cast<float>(ControlPoints[i][2]);
    }
}

void FFBXLoader::BuildStaticIndexBuffer(FbxMesh* Mesh, TArray<uint32>& OutIndices)
{
    OutIndices.Empty();
    int32 PolygonCount = Mesh->GetPolygonCount();
    for (int32 PolyIndex = 0; PolyIndex < PolygonCount; ++PolyIndex)
    {
        int32 PolySize = Mesh->GetPolygonSize(PolyIndex);
        for (int32 Corner = 0; Corner < PolySize; ++Corner)
        {
            int32 ControlPointIndex = Mesh->GetPolygonVertex(PolyIndex, Corner);
            OutIndices.Add(static_cast<uint32>(ControlPointIndex));
        }
    }
}

void FFBXLoader::CopyNormals(FbxMesh* Mesh, TArray<FStaticMeshVertex>& OutVerts)
{
    if (Mesh->GetElementNormalCount() < 1) return;
    auto* NormalElem = Mesh->GetElementNormal(0);
    if (NormalElem->GetMappingMode() == FbxGeometryElement::eByPolygonVertex &&
        NormalElem->GetReferenceMode() == FbxGeometryElement::eDirect)
    {
        int32 VertexCounter = 0;
        int32 PolygonCount = Mesh->GetPolygonCount();
        for (int32 PolyIndex = 0; PolyIndex < PolygonCount; ++PolyIndex)
        {
            int32 PolySize = Mesh->GetPolygonSize(PolyIndex);
            for (int32 Corner = 0; Corner < PolySize; ++Corner, ++VertexCounter)
            {
                int32 CPI = Mesh->GetPolygonVertex(PolyIndex, Corner);
                FbxVector4 N = NormalElem->GetDirectArray().GetAt(VertexCounter);
                OutVerts[CPI].NormalX = static_cast<float>(N[0]);
                OutVerts[CPI].NormalY = static_cast<float>(N[1]);
                OutVerts[CPI].NormalZ = static_cast<float>(N[2]);
            }
        }
    }
}

void FFBXLoader::CopyUVs(FbxMesh* Mesh, TArray<FStaticMeshVertex>& OutVerts)
{
    if (Mesh->GetElementUVCount() < 1) return;
    auto* UVElem = Mesh->GetElementUV(0);
    if (UVElem->GetMappingMode() == FbxGeometryElement::eByPolygonVertex &&
        (UVElem->GetReferenceMode() == FbxGeometryElement::eDirect ||
            UVElem->GetReferenceMode() == FbxGeometryElement::eIndexToDirect))
    {
        int32 VertexCounter = 0;
        int32 PolygonCount = Mesh->GetPolygonCount();
        for (int32 PolyIndex = 0; PolyIndex < PolygonCount; ++PolyIndex)
        {
            int32 PolySize = Mesh->GetPolygonSize(PolyIndex);
            for (int32 Corner = 0; Corner < PolySize; ++Corner, ++VertexCounter)
            {
                int32 UVIndex = (UVElem->GetReferenceMode() == FbxGeometryElement::eDirect)
                    ? VertexCounter
                    : UVElem->GetIndexArray().GetAt(VertexCounter);
                FbxVector2 UV = UVElem->GetDirectArray().GetAt(UVIndex);
                int32 CPI = Mesh->GetPolygonVertex(PolyIndex, Corner);
                OutVerts[CPI].U = static_cast<float>(UV[0]);
                OutVerts[CPI].V = static_cast<float>(UV[1]);
            }
        }
    }
}

void FFBXLoader::CopyTangents(FbxMesh* Mesh, TArray<FStaticMeshVertex>& OutVerts)
{
    if (Mesh->GetElementTangentCount() < 1) return;
    auto* TanElem = Mesh->GetElementTangent(0);
    if (TanElem->GetMappingMode() == FbxGeometryElement::eByPolygonVertex &&
        TanElem->GetReferenceMode() == FbxGeometryElement::eDirect)
    {
        int32 VertexCounter = 0;
        int32 PolygonCount = Mesh->GetPolygonCount();
        for (int32 PolyIndex = 0; PolyIndex < PolygonCount; ++PolyIndex)
        {
            int32 PolySize = Mesh->GetPolygonSize(PolyIndex);
            for (int32 Corner = 0; Corner < PolySize; ++Corner, ++VertexCounter)
            {
                int32 CPI = Mesh->GetPolygonVertex(PolyIndex, Corner);
                FbxVector4 T = TanElem->GetDirectArray().GetAt(VertexCounter);
                OutVerts[CPI].TangentX = static_cast<float>(T[0]);
                OutVerts[CPI].TangentY = static_cast<float>(T[1]);
                OutVerts[CPI].TangentZ = static_cast<float>(T[2]);
                OutVerts[CPI].TangentW = static_cast<float>(T[3]);
            }
        }
    }
}

void FFBXLoader::ComputeBoundingBox(const TArray<FStaticMeshVertex>& InVerts, FVector& OutMin, FVector& OutMax)
{
    if (InVerts.Num() == 0)
    {
        OutMin = OutMax = FVector::ZeroVector;
        return;
    }
    const auto& First = InVerts[0];
    OutMin = OutMax = FVector(First.X, First.Y, First.Z);
    for (int32 i = 1; i < InVerts.Num(); ++i)
    {
        const auto& V = InVerts[i];
        OutMin.X = FMath::Min(OutMin.X, V.X);
        OutMin.Y = FMath::Min(OutMin.Y, V.Y);
        OutMin.Z = FMath::Min(OutMin.Z, V.Z);
        OutMax.X = FMath::Max(OutMax.X, V.X);
        OutMax.Y = FMath::Max(OutMax.Y, V.Y);
        OutMax.Z = FMath::Max(OutMax.Z, V.Z);
    }
}

UStaticMesh* FFBXManager::CreateStaticMesh(const FString& filePath)
{
    FFBXLoader::Initialize();

    StaticMeshRenderData = new FStaticMeshRenderData();

    if (!FFBXLoader::LoadFBX(filePath))
    {
        delete StaticMeshRenderData;
        return nullptr;
    }

    UStaticMesh* StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr);
    StaticMesh->SetData(StaticMeshRenderData);
    return StaticMesh;
}

USkeletalMesh* FFBXManager::CreateSkeletalMesh(const FString& filePath)
{
    FFBXLoader::Initialize();

    SkeletalMeshRenderData = new FSkeletalMeshRenderData();

    if (!FFBXLoader::LoadFBX(filePath))
    {
        delete SkeletalMeshRenderData;
        return nullptr;
    }

    USkeletalMesh* SkeletalMesh = FObjectFactory::ConstructObject<USkeletalMesh>(nullptr);
    SkeletalMesh->SetData(SkeletalMeshRenderData);
    return SkeletalMesh;
}
