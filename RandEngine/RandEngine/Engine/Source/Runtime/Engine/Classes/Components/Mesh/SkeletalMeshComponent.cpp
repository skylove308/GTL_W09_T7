#include "SkeletalMeshComponent.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "Engine/FbxLoader.h"
#include "Engine/SkeletalMeshActor.h"

USkeletalMeshComponent::USkeletalMeshComponent()
{
    SkeletalMesh = nullptr;
    selectedSubMeshIndex = -1;
}

UObject* USkeletalMeshComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));
    // TODO: 이후 애니메이션 상태 등 복사 시 추가 구현
    return NewComponent;
}

void USkeletalMeshComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);

    if (SkeletalMesh)
    {
        FString PathFString = FString(SkeletalMesh->GetRenderData()->ObjectName.c_str());
        OutProperties.Add(TEXT("SkeletalMeshPath"), PathFString);
    }
    else
    {
        OutProperties.Add(TEXT("SkeletalMeshPath"), TEXT("None"));
    }
}

void USkeletalMeshComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);

    const FString* MeshPath = InProperties.Find(TEXT("SkeletalMeshPath"));
    if (MeshPath)
    {
        if (*MeshPath != TEXT("None"))
        {
            if (USkeletalMesh* LoadedMesh = FFBXManager::CreateSkeletalMesh(*MeshPath))
            {
                SetSkeletalMesh(LoadedMesh);
                UE_LOG(ELogLevel::Display, TEXT("Set SkeletalMesh '%s' for %s"), **MeshPath, *GetName());
            }
            else
            {
                UE_LOG(ELogLevel::Warning, TEXT("Could not load SkeletalMesh '%s' for %s"), **MeshPath, *GetName());
                SetSkeletalMesh(nullptr);
            }
        }
        else
        {
            SetSkeletalMesh(nullptr);
        }
    }
}

int USkeletalMeshComponent::CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const
{
    if (!SkeletalMesh) return 0;
    if (!AABB.Intersect(InRayOrigin, InRayDirection, OutHitDistance)) return 0;

    const auto* RenderData = SkeletalMesh->GetRenderData();
    const auto& Vertices = RenderData->Vertices;
    const auto& Indices = RenderData->Indices;

    OutHitDistance = FLT_MAX;
    int HitCount = 0;

    for (int i = 0; i + 2 < Indices.Num(); i += 3)
    {
        const FVector v0(Vertices[Indices[i + 0]].X, Vertices[Indices[i + 0]].Y, Vertices[Indices[i + 0]].Z);
        const FVector v1(Vertices[Indices[i + 1]].X, Vertices[Indices[i + 1]].Y, Vertices[Indices[i + 1]].Z);
        const FVector v2(Vertices[Indices[i + 2]].X, Vertices[Indices[i + 2]].Y, Vertices[Indices[i + 2]].Z);

        float HitDistance = 0.f;
        if (IntersectRayTriangle(InRayOrigin, InRayDirection, v0, v1, v2, HitDistance))
        {
            OutHitDistance = FMath::Min(OutHitDistance, HitDistance);
            ++HitCount;
        }
    }
    return HitCount;
}

void USkeletalMeshComponent::RotateBone(FString BoneName, FRotator Rotation)
{
    if (!SkeletalMesh) return;

    FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetRenderData();
    if (!RenderData) return;

    // 1. BoneName에 해당하는 인덱스 찾기
    int32 BoneIndex = -1;
    for (int32 i = 0; i < RenderData->SkeletonBones.Num(); ++i)
    {
        if (RenderData->SkeletonBones[i].Name == BoneName)
        {
            BoneIndex = i;
            break;
        }
    }

    if (BoneIndex == -1)
    {
        UE_LOG(ELogLevel::Warning, TEXT("Bone not found: %s"), *BoneName);
        return;
    }

    /*Rotation = Cast<ASkeletalMeshActor>(GetOwner())->BoneGizmoSceneComponents[BoneIndex]->GetRelativeRotation();*/
    FFBXLoader::RotateBones(RenderData->SkeletonBones, BoneIndex, FbxVector4(Rotation.Roll, Rotation.Pitch, Rotation.Yaw));

    FFBXLoader::RecalculateGlobalPoses(RenderData->SkeletonBones);

    // 스킨 다시 계산
    FFBXLoader::ReskinVerticesCPU(
        FFBXLoader::Mesh, // 필요시 원본 FbxMesh 포인터 보존 필요
        RenderData->SkeletonBones,
        RenderData->Vertices
    );
}

