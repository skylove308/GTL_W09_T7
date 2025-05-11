// CustomAnimInstance.cpp
#include "AnimInstance.h"
#include "UObject/Casts.h"

UAnimInstance::UAnimInstance()
{
    AnimProxy = new FAnimInstanceProxy();
    AnimProxy->Initialize(this);
}

UAnimInstance::~UAnimInstance()
{
    delete AnimProxy;
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
    AnimProxy->Update(DeltaTime);
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

void UAnimInstance::FAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
    AnimInstance = InAnimInstance;
    ProxyRequiredBones = AnimInstance->RequiredBones;
}

void UAnimInstance::FAnimInstanceProxy::Update(float DeltaTime)
{
}

USkeletalMeshComponent* UAnimInstance::GetSkelMeshComponent() const
{
    //return CastChecked<USkeletalMeshComponent>(GetOuter());
    return OwningComponent;
}
