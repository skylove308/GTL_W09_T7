#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Components/Material/Material.h"
#include "Engine/Animation/Skeleton.h"
#include "Define.h"

struct FSkeletalMeshRenderData;

class USkeletalMesh : public UObject
{
    DECLARE_CLASS(USkeletalMesh, UObject)

public:
    USkeletalMesh();
    virtual ~USkeletalMesh() override;

    virtual UObject* Duplicate(UObject* InOuter) override;

    const TArray<FStaticMaterial*>& GetMaterials() const { return materials; }
    uint32 GetMaterialIndex(FName MaterialSlotName) const;
    void GetUsedMaterials(TArray<UMaterial*>& Out) const;
    FSkeletalMeshRenderData* GetRenderData() const { return SkeletalMeshRenderData; }
    int32 GetBoneIndexByName(const FName& BoneName) const;
    FMatrix GetBoneLocalMatrix(uint32 BoneIndex) const;

    bool SetBoneLocalMatrix(uint32 BoneIndex, const FMatrix& NewLocalMatrix);
    bool SetBoneRotation(uint32 BoneIndex, const FMatrix& RotationMatrix);
    void UpdateWorldTransforms();
    bool UpdateAndApplySkinning();
    bool GetBoneNames(TArray<FName>& OutBoneNames) const;
    //ObjectName은 경로까지 포함
    FWString GetObjectName() const;

    void SetData(FSkeletalMeshRenderData* renderData);

public:
    USkeleton* Skeleton;
private:
    FSkeletalMeshRenderData* SkeletalMeshRenderData = nullptr;
    TArray<FStaticMaterial*> materials;
};
