#include "StaticMeshRenderData.h"

#include "Engine/AssetManager.h"
#include "Engine/FObjLoader.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"

#include "Engine/Asset/StaticMeshAsset.h"




UObject* UStaticMesh::Duplicate(UObject* InOuter)
{
    // TODO: Context->CopyResource를 사용해서 Buffer복사
    // ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate());
    return nullptr;
}

uint32 UStaticMesh::GetMaterialIndex(FName MaterialSlotName) const
{
    for (uint32 materialIndex = 0; materialIndex < Materials.Num(); materialIndex++) {
        if (Materials[materialIndex]->MaterialSlotName == MaterialSlotName)
            return materialIndex;
    }

    return -1;
}

void UStaticMesh::GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const
{
    for (const FStaticMaterial* Material : Materials)
    {
        OutMaterial.Emplace(Material->Material);
    }
}

FWString UStaticMesh::GetOjbectName() const
{
    return RenderData->ObjectName;
}

void UStaticMesh::SetData(FStaticMeshRenderData* InRenderData, TArray<UMaterial*> InMaterials)
{
    RenderData = InRenderData;

    for (auto& material : Materials)
    {
        delete material;
    } 
    Materials.Empty();
    
    for (int materialIndex = 0; materialIndex < RenderData->Materials.Num(); materialIndex++)
    {
        FStaticMaterial* NewMaterialSlot = new FStaticMaterial();

        if (InMaterials.Num() > materialIndex)
        {
            UMaterial* Material = InMaterials[materialIndex];
            NewMaterialSlot->Material = Material;
        }
        else
        {
            UMaterial* Material = UAssetManager::Get().GetMaterial(RenderData->Materials[materialIndex].MaterialName);
            NewMaterialSlot->Material = Material;   
        }
        NewMaterialSlot->MaterialSlotName = RenderData->Materials[materialIndex].MaterialName;

        Materials.Add(NewMaterialSlot);
    }
}
