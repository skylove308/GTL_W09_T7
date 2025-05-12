#include "Player.h"

#include "UnrealClient.h"
#include "World/World.h"
#include "BaseGizmos/GizmoArrowComponent.h"
#include "BaseGizmos/GizmoCircleComponent.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Components/Light/LightComponent.h"
#include "Components/Mesh/SkeletalMeshComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Math/JungleMath.h"
#include "Math/MathUtility.h"
#include "PropertyEditor/ShowFlags.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"
#include "Engine/EditorEngine.h"
#include "SubWindow/SkeletalSubEngine.h"
#include "SubWindow/SubEngine.h"
#include "Engine/SkeletalMeshActor.h"
#include "Components/Mesh/SkeletalMesh.h"

void AEditorPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (GetOuter() == GEngine)
        ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient().get();
    else
    {
        UObject* Outer = GetOuter();
        ActiveViewport = Cast<USubEngine>(Outer)->ViewportClient;
    }
    Input();
}

void AEditorPlayer::Input()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    if (io.WantCaptureKeyboard) return;

    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
    {
        if (!bLeftMouseDown)
        {   
            bLeftMouseDown = true;

            POINT mousePos;
            GetCursorPos(&mousePos);
            GetCursorPos(&m_LastMousePos);
            if (GetOuter() == GEngine)
                ScreenToClient(GEngineLoop.AppWnd, &mousePos);
            else
                ScreenToClient(*Cast<USubEngine>(GetOuter())->Wnd, &mousePos);
            /*
            uint32 UUID = FEngineLoop::GraphicDevice.GetPixelUUID(mousePos);
            // TArray<UObject*> objectArr = GetWorld()->GetObjectArr();
            for ( const USceneComponent* obj : TObjectRange<USceneComponent>())
            {
                if (obj->GetUUID() != UUID) continue;

                UE_LOG(ELogLevel::Display, *obj->GetName());
            }
            */

            FVector pickPosition;

            // std::shared_ptr<FEditorViewportClient> ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
            ScreenToViewSpace(mousePos.x, mousePos.y, ActiveViewport, pickPosition);
            bool res = PickGizmo(pickPosition, ActiveViewport);
            if (!res) PickActor(pickPosition);
        }
        else
        {
            PickedObjControl();
        }
    }
    else
    {
        if (bLeftMouseDown)
        {
            bLeftMouseDown = false;
            // std::shared_ptr<FEditorViewportClient> ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
            ActiveViewport->SetPickedGizmoComponent(nullptr);
        }
    }
}

void AEditorPlayer::ProcessGizmoIntersection(UStaticMeshComponent* Component, const FVector& PickPosition, FEditorViewportClient* InActiveViewport, bool& bIsPickedGizmo)
{
    int maxIntersect = 0;
    float minDistance = FLT_MAX;
    float Distance = 0.0f;
    int currentIntersectCount = 0;
    if (!Component) return;
    if (RayIntersectsObject(PickPosition, Component, Distance, currentIntersectCount))
    {
        if (Distance < minDistance)
        {
            minDistance = Distance;
            maxIntersect = currentIntersectCount;
            //GetWorld()->SetPickingGizmo(iter);
            InActiveViewport->SetPickedGizmoComponent(Component);
            bIsPickedGizmo = true;
        }
        else if (abs(Distance - minDistance) < FLT_EPSILON && currentIntersectCount > maxIntersect)
        {
            maxIntersect = currentIntersectCount;
            //GetWorld()->SetPickingGizmo(iter);
            InActiveViewport->SetPickedGizmoComponent(Component);
            bIsPickedGizmo = true;
        }
    }
}

bool AEditorPlayer::PickGizmo(FVector& pickPosition, FEditorViewportClient* InActiveViewport)
{
    bool isPickedGizmo = false;
    AActor*  SelectedActor = nullptr;
    if (GEngine == GetOuter())
    {
        SelectedActor = Cast<UEditorEngine>(GEngine)->GetSelectedActor();
    }
    else
    {
        SelectedActor = Cast<USkeletalSubEngine>(GetOuter())->SkeletalMeshActor;
    }
    if (SelectedActor)
    {
        if (ControlMode == CM_TRANSLATION)
        {
            for (UStaticMeshComponent* iter : InActiveViewport->GetGizmoActor()->GetArrowArr())
            {
                ProcessGizmoIntersection(iter, pickPosition, InActiveViewport, isPickedGizmo);

            }
        }
        else if (ControlMode == CM_ROTATION)
        {
            for (UStaticMeshComponent* iter : InActiveViewport->GetGizmoActor()->GetDiscArr())
            {
                ProcessGizmoIntersection(iter, pickPosition, InActiveViewport, isPickedGizmo);

            }
        }
        else if (ControlMode == CM_SCALE)
        {
            for (UStaticMeshComponent* iter : InActiveViewport->GetGizmoActor()->GetScaleArr())
            {
                ProcessGizmoIntersection(iter, pickPosition, InActiveViewport, isPickedGizmo);
            }
        }
    }
    return isPickedGizmo;
}

void AEditorPlayer::PickActor(const FVector& pickPosition)
{
    if (!(ShowFlags::GetInstance().CurrentFlags & EEngineShowFlags::SF_Primitives)) return;

    USceneComponent* Possible = nullptr;
    int maxIntersect = 0;
    float minDistance = FLT_MAX;
    for (const auto iter : TObjectRange<UPrimitiveComponent>())
    {
        UPrimitiveComponent* pObj;
        if (iter->IsA<UPrimitiveComponent>() || iter->IsA<ULightComponentBase>())
        {
            pObj = static_cast<UPrimitiveComponent*>(iter);
        }
        else
        {
            continue;
        }

        if (pObj && !pObj->IsA<UGizmoBaseComponent>())
        {
            float Distance = 0.0f;
            int currentIntersectCount = 0;
            if (RayIntersectsObject(pickPosition, pObj, Distance, currentIntersectCount))
            {
                if (Distance < minDistance)
                {
                    minDistance = Distance;
                    maxIntersect = currentIntersectCount;
                    Possible = pObj;
                }
                else if (abs(Distance - minDistance) < FLT_EPSILON && currentIntersectCount > maxIntersect)
                {
                    maxIntersect = currentIntersectCount;
                    Possible = pObj;
                }
            }
        }
    }
    if (Possible)
    {
        if (GEngine == GetOuter())
        {
            Cast<UEditorEngine>(GEngine)->SelectActor(Possible->GetOwner());
            Cast<UEditorEngine>(GEngine)->SelectComponent(Possible);
        }
        else
        {
            Cast<USubEngine>(GetOuter())->SelectedActor=Possible->GetOwner();
            Cast<USubEngine>(GetOuter())->SelectedComponent=Possible;
        }
    }
    else
    {
        if (GEngine == GetOuter())
        {
            Cast<UEditorEngine>(GEngine)->DeselectActor(Cast<UEditorEngine>(GEngine)->GetSelectedActor());
            Cast<UEditorEngine>(GEngine)->DeselectComponent(Cast<UEditorEngine>(GEngine)->GetSelectedComponent());
        }
        else
        {
            Cast<USubEngine>(GetOuter())->SelectedActor=nullptr;
            Cast<USubEngine>(GetOuter())->SelectedComponent= nullptr;
        }
    }
}

void AEditorPlayer::AddControlMode()
{
    ControlMode = static_cast<EControlMode>((ControlMode + 1) % CM_END);
}

void AEditorPlayer::AddCoordiMode()
{
    CoordMode = static_cast<ECoordMode>((CoordMode + 1) % CDM_END);
}

void AEditorPlayer::ScreenToViewSpace(int32 ScreenX, int32 ScreenY, FEditorViewportClient* ActiveViewport, FVector& RayOrigin)
{
    FRect Rect = ActiveViewport->GetViewport()->GetRect();
    
    float ViewportX = static_cast<float>(ScreenX) - Rect.TopLeftX;
    float ViewportY = static_cast<float>(ScreenY) - Rect.TopLeftY;

    FMatrix ProjectionMatrix = ActiveViewport->GetProjectionMatrix();
    
    RayOrigin.X = ((2.0f * ViewportX / Rect.Width) - 1) / ProjectionMatrix[0][0];
    RayOrigin.Y = -((2.0f * ViewportY / Rect.Height) - 1) / ProjectionMatrix[1][1];
    
    if (ActiveViewport->IsOrthographic())
    {
        RayOrigin.Z = 0.0f;  // 오쏘 모드에서는 unproject 시 near plane 위치를 기준
    }
    else
    {
        RayOrigin.Z = 1.0f;  // 퍼스펙티브 모드: near plane
    }
}

int AEditorPlayer::RayIntersectsObject(const FVector& PickPosition, USceneComponent* Component, float& HitDistance, int& IntersectCount)
{
    FMatrix WorldMatrix = Component->GetWorldMatrix();
    FMatrix ViewMatrix = ActiveViewport->GetViewMatrix();
    
    bool bIsOrtho = ActiveViewport->IsOrthographic();
    

    if (bIsOrtho)
    {
        // 오쏘 모드: ScreenToViewSpace()에서 계산된 pickPosition이 클립/뷰 좌표라고 가정
        FMatrix inverseView = FMatrix::Inverse(ViewMatrix);
        // pickPosition을 월드 좌표로 변환
        FVector worldPickPos = inverseView.TransformPosition(PickPosition);  
        // 오쏘에서는 픽킹 원점은 unproject된 픽셀의 위치
        FVector rayOrigin = worldPickPos;
        // 레이 방향은 카메라의 정면 방향 (평행)
        FVector orthoRayDir = ActiveViewport->OrthogonalCamera.GetForwardVector().GetSafeNormal();

        // 객체의 로컬 좌표계로 변환
        FMatrix LocalMatrix = FMatrix::Inverse(WorldMatrix);
        FVector LocalRayOrigin = LocalMatrix.TransformPosition(rayOrigin);
        FVector LocalRayDir = (LocalMatrix.TransformPosition(rayOrigin + orthoRayDir) - LocalRayOrigin).GetSafeNormal();
        
        IntersectCount = Component->CheckRayIntersection(LocalRayOrigin, LocalRayDir, HitDistance);
        return IntersectCount;
    }
    else
    {
        FMatrix inverseMatrix = FMatrix::Inverse(WorldMatrix * ViewMatrix);
        FVector cameraOrigin = { 0,0,0 };
        FVector pickRayOrigin = inverseMatrix.TransformPosition(cameraOrigin);
        // 퍼스펙티브 모드의 기존 로직 사용
        FVector transformedPick = inverseMatrix.TransformPosition(PickPosition);
        FVector rayDirection = (transformedPick - pickRayOrigin).GetSafeNormal();
        
        IntersectCount = Component->CheckRayIntersection(pickRayOrigin, rayDirection, HitDistance);

        if (IntersectCount > 0)
        {
            FVector LocalHitPoint = pickRayOrigin + rayDirection * HitDistance;

            FVector WorldHitPoint = WorldMatrix.TransformPosition(LocalHitPoint);

            FVector WorldRayOrigin;
            FMatrix InverseView = FMatrix::Inverse(ViewMatrix);
            WorldRayOrigin = InverseView.TransformPosition(cameraOrigin);

            float WorldDistance = FVector::Distance(WorldRayOrigin, WorldHitPoint);

            HitDistance = WorldDistance;
        }
        return IntersectCount;
    }
}

void AEditorPlayer::PickedObjControl()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    AActor* SelectedActor = nullptr;
    if (GetOuter() == GEngine)
        SelectedActor = Engine->GetSelectedActor();
    else
        SelectedActor =Cast<USubEngine>(GetOuter())->SelectedActor;
    
    // FEditorViewportClient* ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient().get();
    if (SelectedActor && ActiveViewport->GetPickedGizmoComponent())
    {
        POINT CurrentMousePos;
        GetCursorPos(&CurrentMousePos);
        const float DeltaX = static_cast<float>(CurrentMousePos.x - m_LastMousePos.x);
        const float DeltaY = static_cast<float>(CurrentMousePos.y - m_LastMousePos.y);

        
        USceneComponent* TargetComponent;
        if (GEngine == GetOuter())
            TargetComponent = Engine->GetSelectedComponent();
        else
            TargetComponent =  Cast<USubEngine>(GetOuter())->SelectedComponent;
        if (!TargetComponent)
        {
            if (AActor* SelectedActor = Engine->GetSelectedActor())
            {
                TargetComponent = SelectedActor->GetRootComponent();
            }
            else
            {
                return;
            }
        }
        
        UGizmoBaseComponent* Gizmo = Cast<UGizmoBaseComponent>(ActiveViewport->GetPickedGizmoComponent());
        switch (ControlMode)
        {
        case CM_TRANSLATION:
            // ControlTranslation(TargetComponent, Gizmo, deltaX, deltaY);
            // SLevelEditor에 있음
            break;
        case CM_SCALE:
            ControlScale(TargetComponent, Gizmo, DeltaX, DeltaY);
            break;
        case CM_ROTATION:
            ControlRotation(TargetComponent, Gizmo, DeltaX, DeltaY);
            break;
        default:
            break;
        }
        m_LastMousePos = CurrentMousePos;
    }
}

void AEditorPlayer::ControlRotation(USceneComponent* Component, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY)
{
    // const auto ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
    const FViewportCamera* ViewTransform = ActiveViewport->GetViewportType() == LVT_Perspective
                                                        ? &ActiveViewport->PerspectiveCamera
                                                        : &ActiveViewport->OrthogonalCamera;

    FVector CameraForward = ViewTransform->GetForwardVector();
    FVector CameraRight = ViewTransform->GetRightVector();
    FVector CameraUp = ViewTransform->GetUpVector();

    FQuat CurrentRotation = Component->GetWorldRotation().ToQuaternion();

    FQuat RotationDelta = FQuat();

    if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleX)
    {
        float RotationAmount = (CameraUp.Z >= 0 ? -1.0f : 1.0f) * DeltaY * 0.01f;
        RotationAmount = RotationAmount + (CameraRight.X >= 0 ? 1.0f : -1.0f) * DeltaX * 0.01f;

        FVector Axis = FVector::ForwardVector;
        if (CoordMode == CDM_LOCAL)
        {
            Axis = Component->GetForwardVector();
        }

        RotationDelta = FQuat(Axis, RotationAmount);
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleY)
    {
        float RotationAmount = (CameraRight.X >= 0 ? 1.0f : -1.0f) * DeltaX * 0.01f;
        RotationAmount = RotationAmount + (CameraUp.Z >= 0 ? 1.0f : -1.0f) * DeltaY * 0.01f;

        FVector Axis = FVector::RightVector;
        if (CoordMode == CDM_LOCAL)
        {
            Axis = Component->GetRightVector();
        }

        RotationDelta = FQuat(Axis, RotationAmount);
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleZ)
    {
        float RotationAmount = (CameraForward.X <= 0 ? -1.0f : 1.0f) * DeltaX * 0.01f;

        FVector Axis = FVector::UpVector;
        if (CoordMode == CDM_LOCAL)
        {
            Axis = Component->GetUpVector();
        }
        
        RotationDelta = FQuat(Axis, RotationAmount);
    }

    // Bone Rotating for SkeletalComponent
    if (ASkeletalMeshActor* SkeletalMeshActor = Cast<ASkeletalMeshActor>(Component->GetOwner()))
    {
        USkeletalMeshComponent* SkeletalComp = SkeletalMeshActor->GetComponentByClass<USkeletalMeshComponent>();
        USkeletalMesh* SkeletalMesh = SkeletalComp->GetSkeletalMesh();

        if (!SkeletalMesh)
        {
            return;
        };

        USkeleton* Skeleton = SkeletalMesh->Skeleton;

        if (!Skeleton)
        {
            return;
        }

        int32 BoneIndex = SkeletalMeshActor->BoneGizmoSceneComponents.Find(Component);

        if (BoneIndex == -1) 
        {
            Component->SetWorldRotation(RotationDelta * CurrentRotation);
            return;
        }

        FMatrix CurrentLocalMatrix = SkeletalMesh->GetBoneLocalMatrix(BoneIndex);
        FVector LocalPos = CurrentLocalMatrix.GetTranslationVector();
        FRotator LocalRot = FRotator(CurrentLocalMatrix.ToQuat() * RotationDelta);
        FVector LocalScale = CurrentLocalMatrix.GetScaleVector();

        FMatrix NewLocalMatrix =
            FMatrix::GetScaleMatrix(LocalScale) *
            FMatrix::GetRotationMatrix(LocalRot) *
            FMatrix::GetTranslationMatrix(LocalPos);

        if (SkeletalMesh->SetBoneLocalMatrix(BoneIndex, NewLocalMatrix))
        {
            SkeletalMesh->UpdateWorldTransforms();
            SkeletalMesh->UpdateAndApplySkinning();
        } 
        //Component->SetWorldRotation(RotationDelta * CurrentRotation);
        Component->SetRelativeRotation(LocalRot);
        return;
    }

    // 쿼터니언의 곱 순서는 delta * current 가 맞음.
    Component->SetWorldRotation(RotationDelta * CurrentRotation); 
}

void AEditorPlayer::ControlScale(USceneComponent* Component, UGizmoBaseComponent* Gizmo, float DeltaX, float DeltaY)
{
    // const auto ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();
    const FViewportCamera* ViewTransform = ActiveViewport->GetViewportType() == LVT_Perspective
                                                        ? &ActiveViewport->PerspectiveCamera
                                                        : &ActiveViewport->OrthogonalCamera;
    FVector CameraRight = ViewTransform->GetRightVector();
    FVector CameraUp = ViewTransform->GetUpVector();
    
    // 월드 좌표계에서 카메라 방향을 고려한 이동
    if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleX)
    {
        // 카메라의 오른쪽 방향을 X축 이동에 사용
        FVector moveDir = CameraRight * DeltaX * 0.05f;
        Component->AddScale(FVector(moveDir.X, 0.0f, 0.0f));
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleY)
    {
        // 카메라의 오른쪽 방향을 Y축 이동에 사용
        FVector moveDir = CameraRight * DeltaX * 0.05f;
        Component->AddScale(FVector(0.0f, moveDir.Y, 0.0f));
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleZ)
    {
        // 카메라의 위쪽 방향을 Z축 이동에 사용
        FVector moveDir = CameraUp * -DeltaY * 0.05f;
        Component->AddScale(FVector(0.0f, 0.0f, moveDir.Z));
    }
}

UObject* APlayer::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    return NewActor;
}

void APlayer::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}
