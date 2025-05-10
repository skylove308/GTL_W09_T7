#include "SkeletalMeshDebugger.h"
#include "Components/Mesh/SkeletalMeshComponent.h"
#include "Launch/EngineLoop.h"
#include "Userinterface/Console.h"

void FSkeletalMeshDebugger::DrawSkeleton(const USkeletalMeshComponent* SkelMeshComp)
{
    if (!SkelMeshComp || !SkelMeshComp->GetSkeletalMesh() || !SkelMeshComp->GetSkeletalMesh()->Skeleton)
    {
        UE_LOG(ELogLevel::Error, TEXT("Invalid SkeletalMeshComponent or missing Skeleton"));
        return;
    }

    const USkeleton* Skeleton = SkelMeshComp->GetSkeletalMesh()->Skeleton;
    const FAnimationPoseData& Pose = Skeleton->CurrentPose;
    const TArray<FBoneNode>& BoneTree = Skeleton->BoneTree;
    const int32 NumBones = Pose.GlobalTransforms.Num();

    if (NumBones == 0 || BoneTree.Num() != NumBones)
    {
        UE_LOG(ELogLevel::Warning, TEXT("Bone count mismatch or empty pose"));
        return;
    }

    UPrimitiveDrawBatch* DrawBatch = &GEngineLoop.PrimitiveDrawBatch;

    constexpr float ScaleFactor = 1.f;
    constexpr int ConeSegments = 12;
    const FVector4 ConeColor = FVector4(0.2f, 1.0f, 0.2f, 1.0f);

    const FMatrix CompTransform = SkelMeshComp->GetRotationMatrix() * SkelMeshComp->GetScaleMatrix() * SkelMeshComp->GetTranslationMatrix();
    const FMatrix ToXFront = FMatrix::CreateRotationMatrix(0, 0, 0.f);
    const FMatrix WorldMatrix = ToXFront * CompTransform;

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        int32 ParentIndex = BoneTree[BoneIndex].ParentIndex;
        if (ParentIndex < 0 || ParentIndex >= NumBones)
            continue;

        FVector PosChild = WorldMatrix.TransformPosition(Pose.GlobalTransforms[BoneIndex].GetTranslationVector() * ScaleFactor);
        FVector PosParent = WorldMatrix.TransformPosition(Pose.GlobalTransforms[ParentIndex].GetTranslationVector() * ScaleFactor);

        float Distance = (PosParent - PosChild).Length();
        float ConeRadius = Distance * 0.1f;

        DrawBatch->AddConeToBatch(PosChild, PosParent, ConeRadius, ConeSegments, ConeColor);
    }
}

void FSkeletalMeshDebugger::DrawSkeletonAABBs(const USkeletalMeshComponent* SkelMeshComp)
{
    if (!SkelMeshComp || !SkelMeshComp->GetSkeletalMesh() || !SkelMeshComp->GetSkeletalMesh()->Skeleton)
    {
        UE_LOG(ELogLevel::Error, TEXT("Invalid SkeletalMeshComponent or missing Skeleton"));
        return;
    }

    const USkeleton* Skeleton = SkelMeshComp->GetSkeletalMesh()->Skeleton;
    const FAnimationPoseData& Pose = Skeleton->CurrentPose;
    const int32 NumBones = Pose.GlobalTransforms.Num();

    if (NumBones == 0)
    {
        UE_LOG(ELogLevel::Warning, TEXT("Empty pose"));
        return;
    }

    UPrimitiveDrawBatch* DrawBatch = &GEngineLoop.PrimitiveDrawBatch;

    constexpr float ScaleFactor = 1.f;
    const FVector4 BoxColor = FVector4(1.0f, 0.2f, 0.2f, 1.0f);

    const FMatrix CompTransform = SkelMeshComp->GetRotationMatrix() * SkelMeshComp->GetScaleMatrix() * SkelMeshComp->GetTranslationMatrix();
    const FMatrix ToXFront = FMatrix::CreateRotationMatrix(0, 0, 0.f);
    const FMatrix WorldMatrix = ToXFront * CompTransform;

    // 평균 본 길이 계산
    const TArray<FBoneNode>& BoneTree = Skeleton->BoneTree;
    float TotalBoneLength = 0.f;
    int32 BoneCountWithParent = 0;
    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        int32 ParentIndex = BoneTree[BoneIndex].ParentIndex;
        if (ParentIndex >= 0 && ParentIndex < NumBones)
        {
            FVector ChildPos = WorldMatrix.TransformPosition(Pose.GlobalTransforms[BoneIndex].GetTranslationVector() * ScaleFactor);
            FVector ParentPos = WorldMatrix.TransformPosition(Pose.GlobalTransforms[ParentIndex].GetTranslationVector() * ScaleFactor);
            TotalBoneLength += (ChildPos - ParentPos).Length();
            ++BoneCountWithParent;
        }
    }
    float AvgBoneLength = (BoneCountWithParent > 0) ? TotalBoneLength / BoneCountWithParent : 1.f;
    float HalfExtent = AvgBoneLength * 0.1f;

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        FVector Position = WorldMatrix.TransformPosition(Pose.GlobalTransforms[BoneIndex].GetTranslationVector() * ScaleFactor);

        FBoundingBox LocalBox;
        LocalBox.MinLocation = FVector(-HalfExtent);
        LocalBox.MaxLocation = FVector(HalfExtent);

        DrawBatch->AddAABBToBatch(LocalBox, Position, FMatrix::Identity);
    }
}
