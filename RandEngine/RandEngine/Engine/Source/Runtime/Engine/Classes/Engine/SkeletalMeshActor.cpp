#include "SkeletalMeshActor.h"

#include "AssetManager.h"
#include "Components/Mesh/SkeletalMeshComponent.h"
#include "Engine/FbxLoader.h"
#include "UObject/Casts.h"

ASkeletalMeshActor::ASkeletalMeshActor()
{
    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>();
    RootComponent = SkeletalMeshComponent;
    //SkeletalMeshComponent->SetSkeletalMesh(UAssetManager::Get().GetSkeletalMesh(L"Contents/Sharkry_NoTwist.fbx"));
    SkeletalMeshComponent->SetSkeletalMesh(UAssetManager::Get().GetSkeletalMesh(L"Contents/Rumba Dancing.fbx"));
    
    {
        FSkeletalMeshRenderData* RenderData = SkeletalMeshComponent->GetSkeletalMesh()->GetRenderData();
        if (!RenderData) return;
        USkeleton* Skeleton = SkeletalMeshComponent->GetSkeletalMesh()->Skeleton;
        for (int32 i = 0; i < Skeleton->BoneTree.Num(); ++i)
        {
            if (Skeleton->BoneTree[i].ParentIndex == -1)
            {
                // Create a bone gizmo for the root bone
                BoneGizmoSceneComponent = AddComponent<USceneComponent>(Skeleton->BoneTree[i].Name);
                BoneGizmoSceneComponents.Add(BoneGizmoSceneComponent);
                BoneGizmoSceneComponent->SetupAttachment(RootComponent);
                BoneGizmoSceneComponents[i]->SetRelativeLocation(Skeleton->BoneTree[i].BindTransform.GetTranslationVector());
                BoneGizmoSceneComponents[i]->SetRelativeRotation(Skeleton->BoneTree[i].BindTransform.GetRotationVector());
            }
            else
            {
                // Create a bone gizmo for child bones
                BoneGizmoSceneComponent = AddComponent<USceneComponent>(Skeleton->BoneTree[i].Name);
                BoneGizmoSceneComponents.Add(BoneGizmoSceneComponent);
                BoneGizmoSceneComponent->SetupAttachment(BoneGizmoSceneComponents[Skeleton->BoneTree[i].ParentIndex]);
                BoneGizmoSceneComponents[i]->SetRelativeLocation(Skeleton->BoneTree[i].BindTransform.GetTranslationVector());
                BoneGizmoSceneComponents[i]->SetRelativeRotation(Skeleton->BoneTree[i].BindTransform.GetRotationVector());
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
