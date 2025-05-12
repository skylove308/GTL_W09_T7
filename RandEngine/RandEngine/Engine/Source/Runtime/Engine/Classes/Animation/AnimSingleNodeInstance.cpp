#include "Animation/AnimationAsset.h"
#include "AnimSingleNodeInstance.h"
#include "Components/Mesh/SkeletalMeshComponent.h"
#include "Userinterface/Console.h"
#include "Animation/AnimSequence.h"
#include "UObject/Casts.h"
#include "Math/JungleMath.h"

UAnimSingleNodeInstance::UAnimSingleNodeInstance()
{
    CurrentTime = 0.0f;
}

UAnimSingleNodeInstance::~UAnimSingleNodeInstance()
{
}

void UAnimSingleNodeInstance::SetAnimationAsset(class UAnimationAsset* NewAsset, bool bInIsLooping, float InPlayRate)
{
    if (NewAsset != CurrentAsset)
    {
        CurrentAsset = NewAsset;
    }
    // Proxy 사용하지 않을 듯
    //FAnimSingleNodeInstanceProxy& Proxy = GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>();

    USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
    if (MeshComponent)
    {
        if (MeshComponent->GetSkeletalMeshAsset() == nullptr)
        {
            // if it does not have SkeletalMesh, we nullify it
            CurrentAsset = nullptr;
        }
        else if (CurrentAsset != nullptr)
        {
            // if we have an asset, make sure their skeleton is valid, otherwise, null it
            if (CurrentAsset->GetSkeleton() == nullptr)
            {
                // clear asset since we do not have matching skeleton
                CurrentAsset = nullptr;
            }
        }
    }

    //Proxy.SetAnimationAsset(NewAsset, GetSkelMeshComponent(), bInIsLooping, InPlayRate);
    bIsLooping = bInIsLooping;
    PlayRate = InPlayRate;

    // if composite, we want to make sure this is valid
    // this is due to protect recursive created composite
    // however, if we support modifying asset outside of viewport, it will have to be called whenever modified
    /*if (UAnimCompositeBase* CompositeBase = Cast<UAnimCompositeBase>(NewAsset))
    {
        CompositeBase->InvalidateRecursiveAsset();
    }*/
}

void UAnimSingleNodeInstance::SetLooping(bool bInIsLooping)
{
    //FAnimSingleNodeInstanceProxy& Proxy = GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>();
    //Proxy.SetLooping(bIsLooping);
    bIsLooping = bInIsLooping;
}

void UAnimSingleNodeInstance::SetPlaying(bool bInIsPlaying)
{
    //FAnimSingleNodeInstanceProxy& Proxy = GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>();
    //Proxy.SetPlaying(bIsPlaying);
    bIsPlaying = bInIsPlaying;
}

bool UAnimSingleNodeInstance::IsPlaying() const
{
    //return GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>().IsPlaying();
    return bIsPlaying;
}

void UAnimSingleNodeInstance::ResetToReferencePose()
{
    USkeleton* Skeleton = OwningComponent->GetSkeletalMesh()->Skeleton;

    const int32 NumBones = Skeleton->BoneTree.Num();

    TArray<FMatrix> LocalTransforms;
    LocalTransforms.SetNum(NumBones);

    for (int32 BoneIdx = 0; BoneIdx < NumBones; BoneIdx++)
    {
        LocalTransforms[BoneIdx] = Skeleton->BoneTree[BoneIdx].BindTransform;
    }

    for (int32 i = 0; i < LocalTransforms.Num(); i++)
    {
        OwningComponent->GetSkeletalMesh()->SetBoneLocalMatrix(i, LocalTransforms[i]);
    }

    OwningComponent->GetSkeletalMesh()->UpdateWorldTransforms();
    OwningComponent->GetSkeletalMesh()->UpdateAndApplySkinning();
}

void UAnimSingleNodeInstance::UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion)
{
    if (!CurrentAsset || !bIsPlaying)
    {
        return;
    }

    USkeleton* Skeleton = OwningComponent->GetSkeletalMesh()->Skeleton;
    UAnimSequence* CurrentSequence = Cast<UAnimSequence>(CurrentAsset);

    if (!CurrentSequence || !Skeleton)
    {
        return;
    }
    if (bUseExternalTime) 
    {
        CurrentTime = ExternalTime;
    }
    else 
    {
        CurrentTime += DeltaSeconds * PlayRate;
    }
   
    const double PlayLength = CurrentSequence->GetDataModel()->GetPlayLength();

    if (PlayLength > 0.0)
    {
        if (bIsLooping)
        {
            CurrentTime = FMath::Fmod(CurrentTime, PlayLength);
            if (CurrentTime < 0.0)
            {
                CurrentTime += PlayLength;
            }
        }
        else
        {
            CurrentTime = FMath::Clamp(CurrentTime,
                (PlayRate < 0) ? 0.0f : 0.0f,
                static_cast<float>(PlayLength)
            );

            if ((PlayRate > 0 && CurrentTime >= PlayLength) ||
                (PlayRate < 0 && CurrentTime <= 0.0f))
            {
                bIsPlaying = false;
            }
        }
    }

    FPoseContext Pose(this);

    FAnimExtractContext Extract(CurrentTime, false);

    CurrentSequence->GetAnimationPose(Pose, Extract);

    //LocalTransforms[GEngineLoop.Boneidx].Print();
    for (int32 i = 0; i < Pose.Pose.BonContainer.BoneLocalTransforms.Num(); i++)
    {
        OwningComponent->GetSkeletalMesh()->SetBoneLocalMatrix(i, Pose.Pose.BonContainer.BoneLocalTransforms[i]);
    }

    OwningComponent->GetSkeletalMesh()->UpdateWorldTransforms();
    OwningComponent->GetSkeletalMesh()->UpdateAndApplySkinning();
}
