#pragma once
#include "GameFramework/Actor.h"

class UStaticMeshComponent;
class FEditorViewportClient;

class ATransformGizmo : public AActor
{
    DECLARE_CLASS(ATransformGizmo, AActor)

public:
    ATransformGizmo();

    virtual void Tick(float DeltaTime) override;
    void Initialize(FEditorViewportClient* InViewport);

    FEditorViewportClient* GetAttachedViewport() const { return AttachedViewport; }

    TArray<UStaticMeshComponent*>& GetArrowArr() { return ArrowArr; }
    TArray<UStaticMeshComponent*>& GetDiscArr() { return CircleArr; }
    TArray<UStaticMeshComponent*>& GetScaleArr() { return RectangleArr; }

private:
    // 현재 이 Gizmo가 속한 Viewport
    FEditorViewportClient* AttachedViewport;
    
    TArray<UStaticMeshComponent*> ArrowArr;
    TArray<UStaticMeshComponent*> CircleArr;
    TArray<UStaticMeshComponent*> RectangleArr;
};
