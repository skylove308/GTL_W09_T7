#include "SkeletalMeshRenderData.h"
#include "Engine/FbxLoader.h"
#include "Engine/FOBJLoader.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "UObject/ObjectFactory.h"
#include "EngineLoop.h"

UObject* USkeletalMesh::Duplicate(UObject* InOuter)
{
    // TODO: Context->CopyResource를 사용해서 Buffer복사
    // ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate());
    return nullptr;
}

uint32 USkeletalMesh::GetMaterialIndex(FName MaterialSlotName) const
{
    for (uint32 materialIndex = 0; materialIndex < materials.Num(); materialIndex++)
    {
        if (materials[materialIndex]->MaterialSlotName == MaterialSlotName)
            return materialIndex;
    }

    return -1;
}

void USkeletalMesh::GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const
{
    for (const FStaticMaterial* Material : materials)
    {
        OutMaterial.Emplace(Material->Material);
    }
}

FWString USkeletalMesh::GetObjectName() const
{
    return RenderData->ObjectName;
}

void USkeletalMesh::SetData(FSkeletalMeshRenderData* InRenderData)
{
    RenderData = InRenderData;

    for (int materialIndex = 0; materialIndex < RenderData->Materials.Num(); materialIndex++)
    {
        FStaticMaterial* newMaterialSlot = new FStaticMaterial();
        UMaterial* newMaterial = FObjManager::CreateMaterial(RenderData->Materials[materialIndex]);

        newMaterialSlot->Material = newMaterial;
        newMaterialSlot->MaterialSlotName = RenderData->Materials[materialIndex].MaterialName;

        materials.Add(newMaterialSlot);
    }
}
