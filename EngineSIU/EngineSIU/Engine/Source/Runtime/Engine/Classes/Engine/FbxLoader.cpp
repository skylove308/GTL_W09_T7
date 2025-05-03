#include "FbxLoader.h"
#include "UserInterface/Console.h"
#include "Define.h"
#include "Asset/SkeletalMeshAsset.h"
#include "Asset/StaticMeshAsset.h"
#include "UObject/ObjectFactory.h"
#include "Components/Mesh/StaticMeshRenderData.h"

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
   if (!FindMesh(Scene->GetRootNode()))
   {
       UE_LOG(ELogLevel::Error, TEXT("Failed to find Mesh in FBX scene"));
       Importer->Destroy();
       Scene->Destroy();
       Manager->Destroy();
       return false;
   }

   // Cleanup
   Importer->Destroy();


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

bool FFBXLoader::FindMesh(FbxNode* Node)
{
    if (!Node) return false;

    const FbxNodeAttribute* Attribute = Node->GetNodeAttribute();

    if (Attribute && Attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
    {
        Mesh = Node->GetMesh();

        // Skeletal Mesh
        if (IsSkeletalMesh(Mesh))
        {
            // Build bones and weights
            BuildSkeletalBones(Mesh, FFBXManager::SkeletalMeshRenderData->Bones);
            BuildBoneWeights(Mesh, FFBXManager::SkeletalMeshRenderData->BoneWeights);
            BuildSkeletalVertexBuffers(Mesh, FFBXManager::SkeletalMeshRenderData->Vertices, FFBXManager::SkeletalMeshRenderData->Indices);

            CopyNormals(Mesh, FFBXManager::SkeletalMeshRenderData->Vertices);
            CopyUVs(Mesh, FFBXManager::SkeletalMeshRenderData->Vertices);
            CopyTangents(Mesh, FFBXManager::SkeletalMeshRenderData->Vertices);

            SetupMaterialSubsets(Mesh, FFBXManager::SkeletalMeshRenderData->MaterialSubsets);

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
                            MatInfo.TextureInfos[SlotIdx].TexturePath = ((FString)Texture->GetFileName()).ToWideString();
                        }
                    }
                }

                FFBXManager::SkeletalMeshRenderData->Materials.Add(MatInfo);
            }



            // Update skinning matrices
            TArray<FMatrix> GlobalBoneTransforms;
            for (int i = 0; i < FFBXManager::SkeletalMeshRenderData->Bones.Num(); ++i)
            {
                FbxAMatrix GlobalTransform = Node->EvaluateGlobalTransform();
                GlobalBoneTransforms.Add(FbxAMatrixToFMatrix(GlobalTransform));
            }
            UpdateSkinningMatrices(GlobalBoneTransforms, FFBXManager::SkeletalMeshRenderData->Bones);

        }
        // Static Mesh
        else
        {
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

            int MaterialCount = Node->GetMaterialCount();
            for (int m = 0; m < MaterialCount; ++m)
            {
                FbxSurfaceMaterial * Material = Node->GetMaterial(m);
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
                            MatInfo.TextureInfos[SlotIdx].TexturePath = ((FString)Texture->GetFileName()).ToWideString();
                        }
                    }
                }

               FFBXManager::StaticMeshRenderData->Materials.Add(MatInfo);
            }

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
        if (FindMesh(Node->GetChild(i)))
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
}

//-----------------------------------------------------------------------------
// 3) 정점·인덱스 버퍼 채우기 (버텍스 포지션만 예시, 쉐이딩 데이터는 필요시 추가)
//-----------------------------------------------------------------------------
void FFBXLoader::BuildSkeletalVertexBuffers(FbxMesh* Mesh, TArray<FSkeletalMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
    int ctrlCount = Mesh->GetControlPointsCount();
    OutVerts.SetNum(ctrlCount);

    // 컨트롤 포인트 → 버텍스 포지션
    for (int i = 0; i < ctrlCount; ++i)
    {
        auto P = Mesh->GetControlPointAt(i);
        OutVerts[i].X = (float)P[0];
        OutVerts[i].Y = (float)P[1];
        OutVerts[i].Z = (float)P[2];
        // 나머지(노말·UV·본가중치)는 BuildBoneWeights 후 셰이더에서 활용
    }

    // 폴리곤 → 인덱스 리스트
    int polyCount = Mesh->GetPolygonCount();
    for (int p = 0; p < polyCount; ++p)
    {
        for (int v = 0; v < Mesh->GetPolygonSize(p); ++v)
        {
            int idx = Mesh->GetPolygonVertex(p, v);
            OutIndices.Add(idx);
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

UStaticMesh* FFBXManager::GetStaticMesh(FWString name)
{
    return StaticMeshMap[name];
}
