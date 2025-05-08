#pragma once
#include "GameFramework/Actor.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ObjectTypes.h"


class UGizmoBaseComponent;
class UGizmoArrowComponent;
class USceneComponent;
class UPrimitiveComponent;
class FEditorViewportClient;
class UStaticMeshComponent;

class AEditorPlayer : public AActor
{
    DECLARE_CLASS(AEditorPlayer, AActor)

    AEditorPlayer() = default;

    virtual void Tick(float DeltaTime) override;

    void Input();
    bool PickGizmo(FVector& RayOrigin, FEditorViewportClient* InActiveViewport);
    void ProcessGizmoIntersection(UStaticMeshComponent* Component, const FVector& PickPosition, FEditorViewportClient* InActiveViewport, bool& bIsPickedGizmo);
    void PickActor(const FVector& pickPosition);
    void AddControlMode();
    void AddCoordiMode();

private:
    static int RayIntersectsObject(const FVector& PickPosition, USceneComponent* Component, float& HitDistance, int& IntersectCount);
    void ScreenToViewSpace(int32 ScreenX, int32 ScreenY, std::shared_ptr<FEditorViewportClient> ActiveViewport, FVector& RayOrigin);
    void PickedObjControl();
    void ControlRotation(USceneComponent* Component, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY);
    void ControlScale(USceneComponent* Component, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY);

    bool bLeftMouseDown = false;

    POINT m_LastMousePos;
    EControlMode ControlMode = CM_TRANSLATION;
    ECoordMode CoordMode = CDM_WORLD;

public:
    void SetMode(EControlMode Mode) { ControlMode = Mode; }
    EControlMode GetControlMode() const { return ControlMode; }
    ECoordMode GetCoordMode() const { return CoordMode; }
};

class APlayer : public AActor
{
    DECLARE_CLASS(APlayer, AActor)

public:
    APlayer() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void Tick(float DeltaTime) override;
};
