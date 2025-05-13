#include "FObjLoader.h"

#include "AssetManager.h"
#include "Serializer.h"

#include "Asset/StaticMeshAsset.h"

#include "Components/Material/Material.h"
#include "Components/Mesh/StaticMeshRenderData.h"
#include "UObject/ObjectFactory.h"

bool FObjLoader::ParseOBJ(const FString& ObjFilePath, FObjInfo& OutObjInfo)
{
    std::ifstream OBJ(ObjFilePath.ToWideString());
    if (!OBJ)
    {
        return false;
    }

    OutObjInfo.FilePath = ObjFilePath.ToWideString().substr(0, ObjFilePath.ToWideString().find_last_of(L"\\/") + 1);
    OutObjInfo.ObjectName = ObjFilePath.ToWideString();
    // ObjectName은 wstring 타입이므로, 이를 string으로 변환 (간단한 ASCII 변환의 경우)
    std::wstring wideName = OutObjInfo.ObjectName.substr(ObjFilePath.ToWideString().find_last_of(L"\\/") + 1);;
    std::string fileName(wideName.begin(), wideName.end());

    // 마지막 '.'을 찾아 확장자를 제거
    size_t dotPos = fileName.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        OutObjInfo.DisplayName = fileName.substr(0, dotPos);
    }
    else
    {
        OutObjInfo.DisplayName = fileName;
    }

    /**
     * 블렌더 Export 설정
     *   > General
     *       Forward Axis:  Y
     *       Up Axis:       Z
     *   > Geometry
     *       ✅ Triangulated Mesh
     *   > Materials
     *       ✅ PBR Extensions
     *       Path Mode:     Strip
     */

    std::string Line;

    while (std::getline(OBJ, Line))
    {
        if (Line.empty() || Line[0] == '#')
            continue;

        std::istringstream LineStream(Line);
        std::string Token;
        LineStream >> Token;

        if (Token == "mtllib")
        {
            LineStream >> Line;
            OutObjInfo.MatName = Line;
            continue;
        }

        if (Token == "usemtl")
        {
            LineStream >> Line;
            FString MatName(Line);

            if (!OutObjInfo.MaterialSubsets.IsEmpty())
            {
                FMaterialSubset& LastSubset = OutObjInfo.MaterialSubsets[OutObjInfo.MaterialSubsets.Num() - 1];
                LastSubset.IndexCount = OutObjInfo.VertexIndices.Num() - LastSubset.IndexStart;
            }

            FMaterialSubset MaterialSubset;
            MaterialSubset.MaterialName = MatName;
            MaterialSubset.IndexStart = OutObjInfo.VertexIndices.Num();
            MaterialSubset.IndexCount = 0;
            OutObjInfo.MaterialSubsets.Add(MaterialSubset);
        }

        if (Token == "g" || Token == "o")
        {
            LineStream >> Line;
            OutObjInfo.GroupName.Add(Line);
            OutObjInfo.NumOfGroup++;
        }

        if (Token == "v") // Vertex
        {
            float X, Y, Z;
            LineStream >> X >> Y >> Z;
            OutObjInfo.Vertices.Add(FVector(X, Y * -1.f, Z));
            continue;
        }

        if (Token == "vn") // Normal
        {
            float NormalX, NormalY, NormalZ;
            LineStream >> NormalX >> NormalY >> NormalZ;
            OutObjInfo.Normals.Add(FVector(NormalX, NormalY * -1.f, NormalZ));
            continue;
        }

        if (Token == "vt") // Texture
        {
            float U, V;
            LineStream >> U >> V;
            OutObjInfo.UVs.Add(FVector2D(U, 1.f - V));
            continue;
        }

        if (Token == "f")
        {
            TArray<uint32> FaceVertexIndices;  // 이번 페이스의 정점 인덱스
            TArray<uint32> FaceNormalIndices;  // 이번 페이스의 법선 인덱스
            TArray<uint32> FaceUVIndices; // 이번 페이스의 텍스처 인덱스

            while (LineStream >> Token)
            {
                std::istringstream TokenStream(Token);
                std::string Part;
                TArray<std::string> FacePieces;

                uint32 vertexIndex = 0;
                uint32 textureIndex = UINT32_MAX;
                uint32 normalIndex = UINT32_MAX;

                // v
                if (std::getline(TokenStream, Part, '/'))
                {
                    if (!Part.empty())
                    {
                        vertexIndex = std::stoi(Part) - 1;
                    }
                }

                // vt
                if (std::getline(TokenStream, Part, '/'))
                {
                    if (!Part.empty())
                    {
                        textureIndex = std::stoi(Part) - 1;
                    }
                }

                // vn
                if (std::getline(TokenStream, Part, '/'))
                {
                    if (!Part.empty())
                    {
                        normalIndex = std::stoi(Part) - 1;
                    }
                }

                FaceVertexIndices.Add(vertexIndex);
                FaceUVIndices.Add(textureIndex);
                FaceNormalIndices.Add(normalIndex);
            }

            if (FaceVertexIndices.Num() == 3) // 삼각형
            {
                // 반시계 방향(오른손 좌표계)을 시계 방향(왼손 좌표계)으로 변환: 0-2-1
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[0]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[2]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[1]);

                OutObjInfo.UVIndices.Add(FaceUVIndices[0]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[2]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[1]);

                OutObjInfo.NormalIndices.Add(FaceNormalIndices[0]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[2]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[1]);
            }
            else if (FaceVertexIndices.Num() == 4) // 쿼드
            {
                // 첫 번째 삼각형: 0-2-1
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[0]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[2]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[1]);

                OutObjInfo.UVIndices.Add(FaceUVIndices[0]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[2]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[1]);

                OutObjInfo.NormalIndices.Add(FaceNormalIndices[0]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[2]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[1]);

                // 두 번째 삼각형: 0-3-2
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[0]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[3]);
                OutObjInfo.VertexIndices.Add(FaceVertexIndices[2]);

                OutObjInfo.UVIndices.Add(FaceUVIndices[0]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[3]);
                OutObjInfo.UVIndices.Add(FaceUVIndices[2]);

                OutObjInfo.NormalIndices.Add(FaceNormalIndices[0]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[3]);
                OutObjInfo.NormalIndices.Add(FaceNormalIndices[2]);
            }
        }
    }

    if (!OutObjInfo.MaterialSubsets.IsEmpty())
    {
        FMaterialSubset& LastSubset = OutObjInfo.MaterialSubsets[OutObjInfo.MaterialSubsets.Num() - 1];
        LastSubset.IndexCount = OutObjInfo.VertexIndices.Num() - LastSubset.IndexStart;
    }

    return true;
}

bool FObjLoader::ParseMaterial(const FObjInfo& OutObjInfo, FStaticMeshRenderData& OutFStaticMesh)
{
    // Subset
    OutFStaticMesh.MaterialSubsets = OutObjInfo.MaterialSubsets;

    std::ifstream MtlFile(OutObjInfo.FilePath + OutObjInfo.MatName.ToWideString());
    if (!MtlFile.is_open())
    {
        return false;
    }

    std::string Line;
    int32 MaterialIndex = -1;

    while (std::getline(MtlFile, Line))
    {
        if (Line.empty() || Line[0] == '#')
            continue;

        std::istringstream LineStream(Line);
        std::string Token;
        LineStream >> Token;

        // Create new material if token is 'newmtl'
        if (Token == "newmtl")
        {
            LineStream >> Line;
            MaterialIndex++;

            FObjMaterialInfo Material;
            Material.MaterialName = Line;
            
            constexpr uint32 TexturesNum = static_cast<uint32>(EMaterialTextureSlots::MTS_MAX);
            Material.TextureInfos.SetNum(TexturesNum);

            OutFStaticMesh.Materials.Add(Material);
        }

        if (Token == "Kd")
        {
            float x, y, z;
            LineStream >> x >> y >> z;
            OutFStaticMesh.Materials[MaterialIndex].DiffuseColor = FVector(x, y, z);
        }
        if (Token == "Ks")
        {
            float x, y, z;
            LineStream >> x >> y >> z;
            OutFStaticMesh.Materials[MaterialIndex].SpecularColor = FVector(x, y, z);
        }
        if (Token == "Ka")
        {
            float x, y, z;
            LineStream >> x >> y >> z;
            OutFStaticMesh.Materials[MaterialIndex].AmbientColor = FVector(x, y, z);
        }
        if (Token == "Ke")
        {
            float x, y, z;
            LineStream >> x >> y >> z;
            OutFStaticMesh.Materials[MaterialIndex].EmissiveColor = FVector(x, y, z);
        }
        if (Token == "Ns")
        {
            float x;
            LineStream >> x;
            OutFStaticMesh.Materials[MaterialIndex].SpecularExponent = x;
        }
        if (Token == "Ni")
        {
            float x;
            LineStream >> x;
            OutFStaticMesh.Materials[MaterialIndex].IOR = x;
        }
        if (Token == "d" || Token == "Tr")
        {
            float x;
            LineStream >> x;
            OutFStaticMesh.Materials[MaterialIndex].Transparency = (Token == "Tr") ? x : 1.f - x;
            OutFStaticMesh.Materials[MaterialIndex].bTransparent = true;
        }
        if (Token == "illum")
        {
            uint32 x;
            LineStream >> x;
            OutFStaticMesh.Materials[MaterialIndex].IlluminanceModel = x;
        }
        if (Token == "Pm")
        {
            float x;
            LineStream >> x;
            OutFStaticMesh.Materials[MaterialIndex].Metallic = x;
        }
        if (Token == "Pr")
        {
            float x;
            LineStream >> x;
            OutFStaticMesh.Materials[MaterialIndex].Roughness = x;
        }

        if (Token == "map_Kd")
        {
            const uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Diffuse);
            
            LineStream >> Line;
            OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath))
            {
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = true;
                OutFStaticMesh.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Diffuse);
            }
        }
        if (Token == "map_Bump")
        {
            const uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Normal);
            
            std::string Option;
            while (LineStream >> Option)
            {
                if (Option == "-bm")
                {
                    float BumpMultiplier;
                    LineStream >> BumpMultiplier;
                    OutFStaticMesh.Materials[MaterialIndex].BumpMultiplier = BumpMultiplier;
                }
                else
                {
                    OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Option;

                    FWString TexturePath = OutObjInfo.FilePath + OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
                    if (CreateTextureFromFile(TexturePath, false))
                    {
                        OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                        OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = false;
                        OutFStaticMesh.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Normal);
                    }
                }
            }
        }
        if (Token == "map_Ks")
        {
            const uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Specular);
            
            LineStream >> Line;
            OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath))
            {
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = true;
                OutFStaticMesh.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Specular);
            }
        }
        if (Token == "map_Ns")
        {
            const uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Shininess);
            
            LineStream >> Line;
            OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath, false))
            {
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = false;
                OutFStaticMesh.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Shininess);
            }
        }
        if (Token == "map_Ka")
        {
            const uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Ambient);
            
            LineStream >> Line;
            OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath))
            {
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = true;
                OutFStaticMesh.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Ambient);
            }
        }
        if (Token == "map_Ke")
        {
            const uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Emissive);
            
            LineStream >> Line;
            OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath))
            {
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = true;
                OutFStaticMesh.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Emissive);
            }
        }
        if (Token == "map_Pm")
        {
            const uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Metallic);
            
            LineStream >> Line;
            OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath, false))
            {
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = false;
                OutFStaticMesh.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Metallic);
            }
        }
        if (Token == "map_Pr")
        {
            const uint32 SlotIdx = static_cast<uint32>(EMaterialTextureSlots::MTS_Roughness);
            
            LineStream >> Line;
            OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName = Line;

            FWString TexturePath = OutObjInfo.FilePath + OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TextureName.ToWideString();
            if (CreateTextureFromFile(TexturePath, false))
            {
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].TexturePath = TexturePath;
                OutFStaticMesh.Materials[MaterialIndex].TextureInfos[SlotIdx].bIsSRGB = false;
                OutFStaticMesh.Materials[MaterialIndex].TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Roughness);
            }
        }
        // TODO: map_d 또는 map_Tr은 나중에 필요할때 구현
    }

    return true;
}

void FObjLoader::ExtractMaterial(const FObjInfo& ObjInfo, FStaticMeshRenderData& OutStaticMeshRenderData)
{
    if (ObjInfo.MaterialSubsets.Num() == 0)
    {
        return;
    }
    
    if (!FObjLoader::ParseMaterial(ObjInfo, OutStaticMeshRenderData))
    {
        return;
    }
    
    CombineMaterialIndex(OutStaticMeshRenderData);
}

bool FObjLoader::ExtractStaticMesh(const FObjInfo& RawData, FStaticMeshRenderData& OutStaticMesh)
{
    OutStaticMesh.ObjectName = RawData.ObjectName;
    OutStaticMesh.DisplayName = RawData.DisplayName;

    // 고유 정점을 기반으로 FVertexSimple 배열 생성
    TMap<std::string, uint32> IndexMap; // 중복 체크용

    for (int32 i = 0; i < RawData.VertexIndices.Num(); i++)
    {
        const uint32 VertexIndex = RawData.VertexIndices[i];
        const uint32 UVIndex = RawData.UVIndices[i];
        const uint32 NormalIndex = RawData.NormalIndices[i];

        uint32 MaterialIndex = 0;
        for (int32 j = 0; j < OutStaticMesh.MaterialSubsets.Num(); j++)
        {
            const FMaterialSubset& Subset = OutStaticMesh.MaterialSubsets[j];
            if (Subset.IndexStart <= i && i < Subset.IndexStart + Subset.IndexCount)
            {
                MaterialIndex = Subset.MaterialIndex;
                break;
            }
        }

        // 키 생성 (v/vt/vn 조합)
        std::string Key = std::to_string(VertexIndex) + "/" + std::to_string(UVIndex) + "/" + std::to_string(NormalIndex);

        uint32 FinalIndex;
        if (IndexMap.Contains(Key))
        {
            FinalIndex = IndexMap[Key];
        }
        else
        {
            FStaticMeshVertex StaticMeshVertex = {};
            StaticMeshVertex.MaterialIndex = MaterialIndex;
            StaticMeshVertex.X = RawData.Vertices[VertexIndex].X;
            StaticMeshVertex.Y = RawData.Vertices[VertexIndex].Y;
            StaticMeshVertex.Z = RawData.Vertices[VertexIndex].Z;

            StaticMeshVertex.R = 0.7f; StaticMeshVertex.G = 0.7f; StaticMeshVertex.B = 0.7f; StaticMeshVertex.A = 1.0f; // 기본 색상

            if (UVIndex != UINT32_MAX && UVIndex < RawData.UVs.Num())
            {
                StaticMeshVertex.U = RawData.UVs[UVIndex].X;
                StaticMeshVertex.V = RawData.UVs[UVIndex].Y;
            }

            if (NormalIndex != UINT32_MAX && NormalIndex < RawData.Normals.Num())
            {
                StaticMeshVertex.NormalX = RawData.Normals[NormalIndex].X;
                StaticMeshVertex.NormalY = RawData.Normals[NormalIndex].Y;
                StaticMeshVertex.NormalZ = RawData.Normals[NormalIndex].Z;
            }

            FinalIndex = OutStaticMesh.Vertices.Num();
            IndexMap[Key] = FinalIndex;
            OutStaticMesh.Vertices.Add(StaticMeshVertex);
        }

        OutStaticMesh.Indices.Add(FinalIndex);
    }

    // Tangent
    for (int32 i = 0; i < OutStaticMesh.Indices.Num(); i += 3)
    {
        FStaticMeshVertex& Vertex0 = OutStaticMesh.Vertices[OutStaticMesh.Indices[i]];
        FStaticMeshVertex& Vertex1 = OutStaticMesh.Vertices[OutStaticMesh.Indices[i + 1]];
        FStaticMeshVertex& Vertex2 = OutStaticMesh.Vertices[OutStaticMesh.Indices[i + 2]];

        CalculateTangent(Vertex0, Vertex1, Vertex2);
        CalculateTangent(Vertex1, Vertex2, Vertex0);
        CalculateTangent(Vertex2, Vertex0, Vertex1);
    }

    // Calculate StaticMesh BoundingBox
    ComputeBoundingBox(OutStaticMesh.Vertices, OutStaticMesh.BoundingBoxMin, OutStaticMesh.BoundingBoxMax);
    
    return true;
}

void FObjLoader::CombineMaterialIndex(FStaticMeshRenderData& OutFStaticMesh)
{
    for (int32 i = 0; i < OutFStaticMesh.MaterialSubsets.Num(); i++)
    {
        FString MatName = OutFStaticMesh.MaterialSubsets[i].MaterialName;
        for (int32 j = 0; j < OutFStaticMesh.Materials.Num(); j++)
        {
            if (OutFStaticMesh.Materials[j].MaterialName == MatName)
            {
                OutFStaticMesh.MaterialSubsets[i].MaterialIndex = j;
                break;
            }
        }
    }
}

bool FObjLoader::CreateTextureFromFile(const FWString& Filename, bool bIsSRGB)
{
    if (FEngineLoop::ResourceManager.GetTexture(Filename))
    {
        return true;
    }

    HRESULT hr = FEngineLoop::ResourceManager.LoadTextureFromFile(FEngineLoop::GraphicDevice.Device, Filename.c_str(), bIsSRGB);

    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

void FObjLoader::ComputeBoundingBox(const TArray<FStaticMeshVertex>& InVertices, FVector& OutMinVector, FVector& OutMaxVector)
{
    FVector MinVector = { FLT_MAX, FLT_MAX, FLT_MAX };
    FVector MaxVector = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (int32 i = 0; i < InVertices.Num(); i++)
    {
        MinVector.X = std::min(MinVector.X, InVertices[i].X);
        MinVector.Y = std::min(MinVector.Y, InVertices[i].Y);
        MinVector.Z = std::min(MinVector.Z, InVertices[i].Z);

        MaxVector.X = std::max(MaxVector.X, InVertices[i].X);
        MaxVector.Y = std::max(MaxVector.Y, InVertices[i].Y);
        MaxVector.Z = std::max(MaxVector.Z, InVertices[i].Z);
    }

    OutMinVector = MinVector;
    OutMaxVector = MaxVector;
}

void FObjLoader::CalculateTangent(FStaticMeshVertex& PivotVertex, const FStaticMeshVertex& Vertex1, const FStaticMeshVertex& Vertex2)
{
    const float s1 = Vertex1.U - PivotVertex.U;
    const float t1 = Vertex1.V - PivotVertex.V;
    const float s2 = Vertex2.U - PivotVertex.U;
    const float t2 = Vertex2.V - PivotVertex.V;
    const float E1x = Vertex1.X - PivotVertex.X;
    const float E1y = Vertex1.Y - PivotVertex.Y;
    const float E1z = Vertex1.Z - PivotVertex.Z;
    const float E2x = Vertex2.X - PivotVertex.X;
    const float E2y = Vertex2.Y - PivotVertex.Y;
    const float E2z = Vertex2.Z - PivotVertex.Z;

    const float Denominator = s1 * t2 - s2 * t1;
    FVector Tangent(1, 0, 0);
    FVector BiTangent(0, 1, 0);
    FVector Normal(PivotVertex.NormalX, PivotVertex.NormalY, PivotVertex.NormalZ);
    
    if (FMath::Abs(Denominator) > SMALL_NUMBER)
    {
        // 정상적인 계산 진행
        const float f = 1.f / Denominator;
        
        const float Tx = f * (t2 * E1x - t1 * E2x);
        const float Ty = f * (t2 * E1y - t1 * E2y);
        const float Tz = f * (t2 * E1z - t1 * E2z);
        Tangent = FVector(Tx, Ty, Tz).GetSafeNormal();

        const float Bx = f * (-s2 * E1x + s1 * E2x);
        const float By = f * (-s2 * E1y + s1 * E2y);
        const float Bz = f * (-s2 * E1z + s1 * E2z);
        BiTangent = FVector(Bx, By, Bz).GetSafeNormal();
    }
    else
    {
        // 대체 탄젠트 계산 방법
        // 방법 1: 다른 방향에서 탄젠트 계산 시도
        FVector Edge1(E1x, E1y, E1z);
        FVector Edge2(E2x, E2y, E2z);
    
        // 기하학적 접근: 두 에지 사이의 각도 이등분선 사용
        Tangent = (Edge1.GetSafeNormal() + Edge2.GetSafeNormal()).GetSafeNormal();
    
        // 만약 두 에지가 평행하거나 반대 방향이면 다른 방법 사용
        if (Tangent.IsNearlyZero())
        {
            // TODO: 기본 축 방향 중 하나 선택 (메시의 주 방향에 따라 선택)
            Tangent = FVector(1.0f, 0.0f, 0.0f);
        }
    }

    Tangent = (Tangent - Normal * FVector::DotProduct(Normal, Tangent)).GetSafeNormal();
    
    const float Sign = (FVector::DotProduct(FVector::CrossProduct(Normal, Tangent), BiTangent) < 0.f) ? -1.f : 1.f;

    PivotVertex.TangentX = Tangent.X;
    PivotVertex.TangentY = Tangent.Y;
    PivotVertex.TangentZ = Tangent.Z;
    PivotVertex.TangentW = Sign;
}

bool FObjManager::LoadOBJ(const FString& InFilePath, FObjLoadResult& OutResult)
{
    FWString BinaryPath = (InFilePath + ".bin").ToWideString();
    if (std::ifstream(BinaryPath).good())
    {
        if (LoadStaticMeshFromBinary(BinaryPath, OutResult))
        {
            return true;
        }
    }

    // Parse Obj Info
    FObjInfo NewObjInfo;
    if (!FObjLoader::ParseOBJ(InFilePath, NewObjInfo))
    {
        return false;
    }
    FStaticMeshRenderData* NewStaticMeshRenderData = new FStaticMeshRenderData();
    
    // 중요 - Extract Material -> Extract Static Mesh
    FObjLoader::ExtractMaterial(NewObjInfo, *NewStaticMeshRenderData);
    
    if (!FObjLoader::ExtractStaticMesh(NewObjInfo, *NewStaticMeshRenderData))
    {
        delete NewStaticMeshRenderData;
        return false;
    }
    
    for (int MaterialIndex = 0; MaterialIndex < NewStaticMeshRenderData->Materials.Num(); MaterialIndex++)
    {
        UMaterial* Material = FObjectFactory::ConstructObject<UMaterial>(nullptr);
        Material->SetMaterialInfo(NewStaticMeshRenderData->Materials[MaterialIndex]);
        OutResult.Materials.Add(Material);
    }

    UStaticMesh* StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr); // TODO: 추후 AssetManager를 생성해서 관리.
    StaticMesh->SetData(NewStaticMeshRenderData, OutResult.Materials);
    OutResult.StaticMesh = StaticMesh;

    SaveStaticMeshToBinary(BinaryPath, *NewStaticMeshRenderData);
    
    return true;
}

bool FObjManager::SaveStaticMeshToBinary(const FWString& FilePath, const FStaticMeshRenderData& StaticMesh)
{
    std::ofstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        assert("CAN'T SAVE STATIC MESH BINARY FILE");
        return false;
    }

    // Object Name
    Serializer::WriteFWString(File, StaticMesh.ObjectName);

    // Display Name
    Serializer::WriteFString(File, StaticMesh.DisplayName);

    // Vertices
    Serializer::WriteArray(File, StaticMesh.Vertices);

    // Indices
    Serializer::WriteArray(File, StaticMesh.Indices);

    // Materials
    uint32 MaterialCount = StaticMesh.Materials.Num();
    File.write(reinterpret_cast<const char*>(&MaterialCount), sizeof(MaterialCount));
    for (const FObjMaterialInfo& Material : StaticMesh.Materials)
    {
        Serializer::WriteFString(File, Material.MaterialName);
        
        File.write(reinterpret_cast<const char*>(&Material.TextureFlag), sizeof(Material.TextureFlag));
        
        File.write(reinterpret_cast<const char*>(&Material.bTransparent), sizeof(Material.bTransparent));
        File.write(reinterpret_cast<const char*>(&Material.DiffuseColor), sizeof(Material.DiffuseColor));
        File.write(reinterpret_cast<const char*>(&Material.SpecularColor), sizeof(Material.SpecularColor));
        File.write(reinterpret_cast<const char*>(&Material.AmbientColor), sizeof(Material.AmbientColor));
        File.write(reinterpret_cast<const char*>(&Material.EmissiveColor), sizeof(Material.EmissiveColor));
        
        File.write(reinterpret_cast<const char*>(&Material.SpecularExponent), sizeof(Material.SpecularExponent));
        File.write(reinterpret_cast<const char*>(&Material.IOR), sizeof(Material.IOR));
        File.write(reinterpret_cast<const char*>(&Material.Transparency), sizeof(Material.Transparency));
        File.write(reinterpret_cast<const char*>(&Material.BumpMultiplier), sizeof(Material.BumpMultiplier));
        File.write(reinterpret_cast<const char*>(&Material.IlluminanceModel), sizeof(Material.IlluminanceModel));

        File.write(reinterpret_cast<const char*>(&Material.Metallic), sizeof(Material.Metallic));
        File.write(reinterpret_cast<const char*>(&Material.Roughness), sizeof(Material.Roughness));

        for (uint8 i = 0; i < static_cast<uint8>(EMaterialTextureSlots::MTS_MAX); ++i)
        {
            Serializer::WriteFString(File, Material.TextureInfos[i].TextureName);
            Serializer::WriteFWString(File, Material.TextureInfos[i].TexturePath);
            File.write(reinterpret_cast<const char*>(&Material.TextureInfos[i].bIsSRGB), sizeof(Material.TextureInfos[i].bIsSRGB));
        }
    }

    // Material Subsets
    uint32 SubsetCount = StaticMesh.MaterialSubsets.Num();
    File.write(reinterpret_cast<const char*>(&SubsetCount), sizeof(SubsetCount));
    for (const FMaterialSubset& Subset : StaticMesh.MaterialSubsets)
    {
        Serializer::WriteFString(File, Subset.MaterialName);
        File.write(reinterpret_cast<const char*>(&Subset.IndexStart), sizeof(Subset.IndexStart));
        File.write(reinterpret_cast<const char*>(&Subset.IndexCount), sizeof(Subset.IndexCount));
        File.write(reinterpret_cast<const char*>(&Subset.MaterialIndex), sizeof(Subset.MaterialIndex));
    }

    // Bounding Box
    File.write(reinterpret_cast<const char*>(&StaticMesh.BoundingBoxMin), sizeof(FVector));
    File.write(reinterpret_cast<const char*>(&StaticMesh.BoundingBoxMax), sizeof(FVector));

    File.close();
    return true;
}

bool FObjManager::LoadStaticMeshFromBinary(const FWString& FilePath, FObjLoadResult& OutResult)
{
    std::ifstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        assert("CAN'T OPEN STATIC MESH BINARY FILE");
        return false;
    }

    TArray<TPair<FWString, bool>> Textures;

    UStaticMesh* StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr); // TODO: 추후 AssetManager를 생성해서 관리.
    FStaticMeshRenderData* StaticMeshRenderData = new FStaticMeshRenderData();
    // Object Name
    Serializer::ReadFWString(File, StaticMeshRenderData->ObjectName);

    //// Path Name
    //Serializer::ReadFWString(File, StaticMeshRenderData->PathName);

    // Display Name
    Serializer::ReadFString(File, StaticMeshRenderData->DisplayName);

    // Vertices
    Serializer::ReadArray(File, StaticMeshRenderData->Vertices);

    // Indices
    Serializer::ReadArray(File, StaticMeshRenderData->Indices);

    // Material
    uint32 MaterialCount = 0;
    File.read(reinterpret_cast<char*>(&MaterialCount), sizeof(MaterialCount));
    StaticMeshRenderData->Materials.SetNum(MaterialCount);
    for (FObjMaterialInfo& MaterialInfo : StaticMeshRenderData->Materials)
    {
        Serializer::ReadFString(File, MaterialInfo.MaterialName);
        File.read(reinterpret_cast<char*>(&MaterialInfo.TextureFlag), sizeof(MaterialInfo.TextureFlag));
    
        File.read(reinterpret_cast<char*>(&MaterialInfo.bTransparent), sizeof(MaterialInfo.bTransparent));
        File.read(reinterpret_cast<char*>(&MaterialInfo.DiffuseColor), sizeof(MaterialInfo.DiffuseColor));
        File.read(reinterpret_cast<char*>(&MaterialInfo.SpecularColor), sizeof(MaterialInfo.SpecularColor));
        File.read(reinterpret_cast<char*>(&MaterialInfo.AmbientColor), sizeof(MaterialInfo.AmbientColor));
        File.read(reinterpret_cast<char*>(&MaterialInfo.EmissiveColor), sizeof(MaterialInfo.EmissiveColor));
    
        File.read(reinterpret_cast<char*>(&MaterialInfo.SpecularExponent), sizeof(MaterialInfo.SpecularExponent));
        File.read(reinterpret_cast<char*>(&MaterialInfo.IOR), sizeof(MaterialInfo.IOR));
        File.read(reinterpret_cast<char*>(&MaterialInfo.Transparency), sizeof(MaterialInfo.Transparency));
        File.read(reinterpret_cast<char*>(&MaterialInfo.BumpMultiplier), sizeof(MaterialInfo.BumpMultiplier));
        File.read(reinterpret_cast<char*>(&MaterialInfo.IlluminanceModel), sizeof(MaterialInfo.IlluminanceModel));

        File.read(reinterpret_cast<char*>(&MaterialInfo.Metallic), sizeof(MaterialInfo.Metallic));
        File.read(reinterpret_cast<char*>(&MaterialInfo.Roughness), sizeof(MaterialInfo.Roughness));

        uint8 TextureNum = static_cast<uint8>(EMaterialTextureSlots::MTS_MAX);
        MaterialInfo.TextureInfos.SetNum(TextureNum);
        for (uint8 i = 0; i < TextureNum; ++i)
        {
            Serializer::ReadFString(File, MaterialInfo.TextureInfos[i].TextureName);
            Serializer::ReadFWString(File, MaterialInfo.TextureInfos[i].TexturePath);
            File.read(reinterpret_cast<char*>(&MaterialInfo.TextureInfos[i].bIsSRGB), sizeof(MaterialInfo.TextureInfos[i].bIsSRGB));

            Textures.AddUnique({MaterialInfo.TextureInfos[i].TexturePath, MaterialInfo.TextureInfos[i].bIsSRGB});
        }

        UMaterial* Material = FObjectFactory::ConstructObject<UMaterial>(nullptr);
        Material->SetMaterialInfo(MaterialInfo);
        OutResult.Materials.Add(Material);
    }

    // Material Subset
    uint32 SubsetCount = 0;
    File.read(reinterpret_cast<char*>(&SubsetCount), sizeof(SubsetCount));
    StaticMeshRenderData->MaterialSubsets.SetNum(SubsetCount);
    for (FMaterialSubset& Subset : StaticMeshRenderData->MaterialSubsets)
    {
        Serializer::ReadFString(File, Subset.MaterialName);
        File.read(reinterpret_cast<char*>(&Subset.IndexStart), sizeof(Subset.IndexStart));
        File.read(reinterpret_cast<char*>(&Subset.IndexCount), sizeof(Subset.IndexCount));
        File.read(reinterpret_cast<char*>(&Subset.MaterialIndex), sizeof(Subset.MaterialIndex));
    }

    // Bounding Box
    File.read(reinterpret_cast<char*>(&StaticMeshRenderData->BoundingBoxMin), sizeof(FVector));
    File.read(reinterpret_cast<char*>(&StaticMeshRenderData->BoundingBoxMax), sizeof(FVector));

    File.close();
    OutResult.StaticMesh = StaticMesh;
    StaticMesh->SetData(StaticMeshRenderData, OutResult.Materials);


    // Texture Load
    if (Textures.Num() > 0)
    {
        for (const TPair<FWString, bool>& Texture : Textures)
        {
            if (FEngineLoop::ResourceManager.GetTexture(Texture.Key) == nullptr)
            {
                FEngineLoop::ResourceManager.LoadTextureFromFile(FEngineLoop::GraphicDevice.Device, Texture.Key.c_str(), Texture.Value);
            }
        }
    }

    return true;
}
