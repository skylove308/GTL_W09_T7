#pragma once
#include "Container/Array.h"
#include "Container/String.h"
#include "HAL/PlatformType.h"


struct FVector;
struct FObjInfo;
struct FObjLoadResult;
class UStaticMesh;
struct FObjManager;

struct FStaticMeshVertex;
struct FStaticMeshRenderData;

struct FObjLoader
{
    // Obj Parsing (*.obj to FObjInfo)
    static bool ParseOBJ(const FString& ObjFilePath, FObjInfo& OutObjInfo);

    // Material Parsing (*.obj to MaterialInfo)
    static bool ParseMaterial(const FObjInfo& OutObjInfo, FStaticMeshRenderData& OutFStaticMesh);

    static void ExtractMaterial(const FObjInfo& ObjInfo, FStaticMeshRenderData& OutStaticMeshRenderData);
    // Convert the Raw data to Cooked data (FStaticMeshRenderData)
    static bool ExtractStaticMesh(const FObjInfo& RawData, FStaticMeshRenderData& OutStaticMesh);

    static void CombineMaterialIndex(FStaticMeshRenderData& OutFStaticMesh);
    
    static bool CreateTextureFromFile(const FWString& Filename, bool bIsSRGB = true);

    static void ComputeBoundingBox(const TArray<FStaticMeshVertex>& InVertices, FVector& OutMinVector, FVector& OutMaxVector);

private:
    static void CalculateTangent(FStaticMeshVertex& PivotVertex, const FStaticMeshVertex& Vertex1, const FStaticMeshVertex& Vertex2);
};

struct FObjManager
{
public:
    static bool LoadOBJ(const FString& InFilePath, FObjLoadResult& OutResult);

private:
    static bool LoadStaticMeshFromBinary(const FWString& FilePath, FObjLoadResult& OutResult);
    static bool SaveStaticMeshToBinary(const FWString& FilePath, const FStaticMeshRenderData& StaticMesh);
};
