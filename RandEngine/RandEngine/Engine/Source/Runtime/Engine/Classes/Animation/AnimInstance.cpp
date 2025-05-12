// CustomAnimInstance.cpp
#include "AnimInstance.h"
#include "Animation/AnimTypes.h"
#include "UObject/Casts.h"


UAnimInstance::UAnimInstance()
{
}

UAnimInstance::~UAnimInstance()
{
}

void UAnimInstance::Initialize()
{
    // 뼈 데이터 초기화
    if (OwningComponent)
    {
        //RequiredBones.BoneNames = OwningComponent->GetBoneNames();
        //RequiredBones.ParentIndices = OwningComponent->GetBoneParentIndices();
    }
}

void UAnimInstance::Update(float DeltaTime)
{
    //AnimProxy->Update(DeltaTime);
    TriggerAnimNotifies(DeltaTime);
}

void UAnimInstance::PostEvaluate()
{
}

void UAnimInstance::TriggerAnimNotifies(float DeltaTime)
{
    /*for (auto& Notify : NotifyQueue.TriggeredNotifies)
    {
        HandleNotify(Notify);
    }
    NotifyQueue.TriggeredNotifies.Empty();*/
}

//void UAnimInstance::HandleNotify(const FAnimNotifyEvent& NotifyEvent)
//{
//}

USkeletalMeshComponent* UAnimInstance::GetSkelMeshComponent() const
{
    //return CastChecked<USkeletalMeshComponent>(GetOuter());
    return OwningComponent;
}

void UAnimInstance::UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion)
{
}
