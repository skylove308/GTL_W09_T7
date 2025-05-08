#pragma once
#include "GameFramework/Actor.h"

class USkeletalMeshComponent;
class USkeletalMesh;

class ASkeletalMeshActor : public AActor
{
    DECLARE_CLASS(ASkeletalMeshActor, AActor)

public:
    ASkeletalMeshActor();

    virtual UObject* Duplicate(UObject* InOuter) override;
    USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }

    TArray<USceneComponent*> BoneGizmoSceneComponents;
    USceneComponent* BoneGizmoSceneComponent = nullptr;

protected:
    UPROPERTY(USkeletalMeshComponent*, SkeletalMeshComponent, = nullptr);

};
