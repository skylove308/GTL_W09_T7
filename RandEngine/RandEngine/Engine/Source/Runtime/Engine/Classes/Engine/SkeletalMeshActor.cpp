#include "SkeletalMeshActor.h"
#include "Components/Mesh/SkeletalMeshComponent.h"
#include "Engine/FbxLoader.h"
#include "UObject/Casts.h"

ASkeletalMeshActor::ASkeletalMeshActor()
{
    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>();
    RootComponent = SkeletalMeshComponent;

    USkeletalMesh* DefaultMesh = FFBXManager::CreateSkeletalMesh("Contents/Mutant.fbx");
    if (DefaultMesh)
    {
        SkeletalMeshComponent->SetSkeletalMesh(DefaultMesh);

        FSkeletalMeshRenderData* RenderData = DefaultMesh->GetRenderData();
        if (!RenderData) return;

        for (int32 i = 0; i < RenderData->SkeletonBones.Num(); ++i)
        {
            if (RenderData->SkeletonBones[i].ParentIndex == -1)
            {
                // Create a bone gizmo for the root bone
                BoneGizmoSceneComponent = AddComponent<USceneComponent>(RenderData->SkeletonBones[i].Name);
                BoneGizmoSceneComponents.Add(BoneGizmoSceneComponent);
                BoneGizmoSceneComponent->SetupAttachment(RootComponent);
                BoneGizmoSceneComponents[i]->SetRelativeLocation(RenderData->SkeletonBones[i].LocalBindPose.GetTranslationVector());
                BoneGizmoSceneComponents[i]->SetRelativeRotation(RenderData->SkeletonBones[i].LocalBindPose.GetRotationVector());
            }
            else
            {
                // Create a bone gizmo for child bones
                BoneGizmoSceneComponent = AddComponent<USceneComponent>(RenderData->SkeletonBones[i].Name);
                BoneGizmoSceneComponents.Add(BoneGizmoSceneComponent);
                BoneGizmoSceneComponent->SetupAttachment(BoneGizmoSceneComponents[RenderData->SkeletonBones[i].ParentIndex]);
                BoneGizmoSceneComponents[i]->SetRelativeLocation(RenderData->SkeletonBones[i].LocalBindPose.GetTranslationVector());
                BoneGizmoSceneComponents[i]->SetRelativeRotation(RenderData->SkeletonBones[i].LocalBindPose.GetRotationVector());
            }
        }
    }
}

UObject* ASkeletalMeshActor::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->SkeletalMeshComponent = Cast<USkeletalMeshComponent>(NewActor->GetComponentByClass<USkeletalMeshComponent>());
    return NewActor;
}
