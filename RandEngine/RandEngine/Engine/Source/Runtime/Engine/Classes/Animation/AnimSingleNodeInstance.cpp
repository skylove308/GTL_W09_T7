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

    CurrentTime += DeltaSeconds * PlayRate;
    const double PlayLength = CurrentSequence->GetDataModel()->GetPlayLength();
    if (PlayLength > 0.0)
    {
        CurrentTime = FMath::Fmod(CurrentTime, PlayLength);
        if (CurrentTime < 0.0)
        {
            CurrentTime += PlayLength;
        }
    }

    const TArray<FBoneAnimationTrack>& BoneTracks = CurrentSequence->GetDataModel()->GetBoneAnimationTracks();

    if (frame >= BoneTracks[0].InternalTrackData.PosKeys.Num())
    {
        frame = 0;
    }
    
    const int32 NumBones = Skeleton->BoneTree.Num();

    TArray<FMatrix> LocalTransforms;
    LocalTransforms.SetNum(NumBones);
    for (int32 BoneIdx = 0; BoneIdx < NumBones; BoneIdx++)
    {
        LocalTransforms[BoneIdx] = Skeleton->ReferenceSkeleton.RefBonePose[BoneIdx];
    }

    const FFrameRate FrameRate = CurrentSequence->GetDataModel()->GetFrameRate();
    const float FrameTime = CurrentTime * FrameRate.AsDecimal();
    
    for (const FBoneAnimationTrack& Track : BoneTracks)
    {
        const int32 BoneIndex = Skeleton->GetBoneIndex(Track.Name);

        FString tt = BoneTracks[BoneIndex].Name.ToString();
        FString nn = Skeleton->BoneTree[BoneIndex].Name.ToString();
        std::cout << GetData(tt) << GetData(nn);
        if (BoneIndex == INDEX_NONE)
        {
            continue;
        }
    
        // 키 프레임 보간
        const int32 PrevKey = FMath::Clamp(FMath::FloorToInt(FrameTime), 0, Track.InternalTrackData.PosKeys.Num() - 1);
        const int32 NextKey = FMath::Clamp(PrevKey + 1, 0, Track.InternalTrackData.PosKeys.Num() - 1);
    
        const float Alpha = FMath::Clamp(FrameTime - PrevKey, 0.f, 1.f);
        // 변환 요소 보간
        const FVector Translation = FMath::Lerp(
            Track.InternalTrackData.PosKeys[PrevKey],
            Track.InternalTrackData.PosKeys[NextKey],
            Alpha
        );
        //
        const FQuat Rotation = FQuat::Slerp(
            Track.InternalTrackData.RotKeys[PrevKey],
            Track.InternalTrackData.RotKeys[NextKey],
            Alpha
        );
        //
        const FVector Scale = FMath::Lerp(
            Track.InternalTrackData.ScaleKeys[PrevKey],
            Track.InternalTrackData.ScaleKeys[NextKey],
            Alpha
        );

        LocalTransforms[BoneIndex] = JungleMath::CreateModelMatrix(Translation, Rotation, Scale);
    }
    for (int32 i = 0; i < LocalTransforms.Num(); i++)
    {
        OwningComponent->GetSkeletalMesh()->SetBoneLocalMatrix(i, LocalTransforms[i]);
    }
    OwningComponent->GetSkeletalMesh()->UpdateWorldTransforms();
    //Skeleton->UpdateCurrentPose(LocalTransforms);
    OwningComponent->GetSkeletalMesh()->UpdateAndApplySkinning();
    frame++;
}
