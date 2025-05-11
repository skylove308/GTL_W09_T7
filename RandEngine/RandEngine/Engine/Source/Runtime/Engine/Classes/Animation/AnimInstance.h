// CustomAnimInstance.h
#pragma once

#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimTypes.h"

class USkeleton;
class USkeletalMeshComponent;
//struct FAnimNotifyEvent;
struct FAnimNotifyQueue
{
    //TArray<FAnimNotifyEvent> ActiveNotifies;
    //TArray<FAnimNotifyEvent> TriggeredNotifies;
};

class UAnimInstance : public UObject
{
    DECLARE_CLASS(UAnimInstance, UObject)
public:
    // 기본 생성자/소멸자
    UAnimInstance();
    virtual ~UAnimInstance();

    // 초기화/업데이트 인터페이스
    void Initialize();
    void Update(float DeltaTime);
    void PostEvaluate();

    void TriggerAnimNotifies(float DeltaTime);
    //void HandleNotify(const FAnimNotifyEvent& NotifyEvent);

    // 뼈/커브 데이터
    struct FBoneContainer
    {
        TArray<FName> BoneNames;
        TArray<int32> ParentIndices;
    };

    const FBoneContainer& GetRequiredBones() const { return RequiredBones; }

    USkeletalMeshComponent* GetSkelMeshComponent() const;
    // [TEMP]
    virtual void UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion);

    void SetCurrentSkeleton(USkeleton* InSkeleton) { CurrentSkeleton = InSkeleton; }

    void SetOwningComponent(USkeletalMeshComponent* InComponent) { OwningComponent = InComponent; }

public:
    USkeleton* CurrentSkeleton = nullptr;

protected:
    // 필수 데이터
    FBoneContainer RequiredBones;
    TMap<FName, float> AnimationCurves;
    FAnimNotifyQueue NotifyQueue;

    // 컴포넌트 참조
    USkeletalMeshComponent* OwningComponent = nullptr;

    // 프록시 시스템
    class FAnimInstanceProxy
    {
    public:
        void Initialize(UAnimInstance* InAnimInstance);
        void Update(float DeltaTime);

        UAnimInstance* AnimInstance = nullptr;
        FBoneContainer ProxyRequiredBones;
    };

    FAnimInstanceProxy* AnimProxy = nullptr;
};
