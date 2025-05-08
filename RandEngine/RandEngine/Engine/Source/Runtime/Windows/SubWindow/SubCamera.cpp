#include "SubCamera.h"

#include "Math/JungleMath.h"
#include <DirectXMath.h>

FSubCamera::FSubCamera(float Width, float Height)
{
    Reset();
    UpdateCamera(Width, Height);
}

void FSubCamera::UpdateCamera(float Width, float Height)
{
    if (Width == 0 || Height == 0)
    {
        CalculateProjection();
    }
    else
    {
        LastWidth = Width;
        LastHeight = Height;
        AspectRatio = Width / Height;
        CalculateProjection();
    }
}

void FSubCamera::UpdateViewMatrix()
{
    // 회전 행렬
    DirectX::XMMATRIX RotMat = DirectX::XMMatrixRotationQuaternion(Orientation);
    // 카메라 위치 = Target + RotMat * (0,0,-Radius)
    DirectX::XMVECTOR Offset = XMVector3TransformCoord(DirectX::XMVectorSet(0,0, -CameraLocation.Z,0), RotMat);
    DirectX::XMVECTOR Eye    = DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&Target), Offset);

    // Up 벡터도 회전 적용 (원하면 고정 Z-up 사용 가능)
    DirectX::XMVECTOR Up = XMVector3TransformCoord(DirectX::XMVectorSet(0,1,0,0), RotMat);

    DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, DirectX::XMLoadFloat3(&Target), Up);
    ViewMatrix = FMatrix::FromXMMatrix(View);
}

void FSubCamera::CalculateProjection()
{
    float FOVRadian = FMath::DegreesToRadians(FOV);
    ProjectionMatrix = JungleMath::CreateProjectionMatrix(FOVRadian, AspectRatio, CameraNearClip, CameraFarClip);
}

void FSubCamera::SetTargetPosition(float X, float Y, float Z)
{
    Target.x = X;
    Target.y = Y;
    Target.z = Z;
}

void FSubCamera::SetTargetZOffset(float ZOffset)
{
    CameraLocation.Z = ZOffset;
}

void FSubCamera::OnMouseDownRight(int MouseX, int MouseY, HWND hWnd)
{
    bDrag = true;
    StartVector = MapToSphere(MouseX, MouseY, hWnd);
    InitialOrientation = Orientation;
}

void FSubCamera::OnMouseUpRight()
{
    bDrag = false;
}

void FSubCamera::OnMouseMove(int MouseX, int MouseY, HWND hWnd)
{
    if (!bDrag)
    {
        return;
    }

    DirectX::XMVECTOR EndVector = MapToSphere(MouseX, MouseY, hWnd);

    DirectX::XMVECTOR RotationAxis = DirectX::XMVector3Cross(StartVector, EndVector);

    DirectX::XMVECTOR DeltaQuat;

    float AxisLenSq = DirectX::XMVectorGetX(DirectX::XMVector3Dot(RotationAxis, RotationAxis));
    if (AxisLenSq < 1e-6f)
    {
        // 회전축이 거의 0벡터면 회전 없음
        DeltaQuat = DirectX::XMQuaternionIdentity();
    }
    else
    {
        // 축 정규화
        DirectX::XMVECTOR NormAxis = DirectX::XMVector3Normalize(RotationAxis);
        // 두 벡터 사이 각도
        float Dot   = std::clamp(DirectX::XMVectorGetX(DirectX::XMVector3Dot(StartVector, EndVector)), -1.0f, 1.0f);
        float Angle = acosf(Dot);
        // Δ쿼터니언 생성
        DeltaQuat = DirectX::XMQuaternionRotationAxis(NormAxis, Angle);
    }
    
    Orientation = DirectX::XMQuaternionNormalize(DirectX::XMQuaternionMultiply(DeltaQuat, InitialOrientation));

    UpdateViewMatrix();
}

void FSubCamera::OnMouseWheel(short WheelDelta)
{
    float FovSensitivity = 0.1f;
    // UpdateFOV(-WheelDelta * FovSensitivity);
    CameraLocation.Z += -WheelDelta * FovSensitivity;
    UpdateViewMatrix();
}

FMatrix FSubCamera::GetViewMatrix() const
{
    return ViewMatrix;
}

FMatrix FSubCamera::GetProjectionMatrix() const
{
    return ProjectionMatrix;
}

FVector FSubCamera::GetCameraLocation() const
{
    return CameraLocation;
}

float FSubCamera::GetCameraNearClip() const
{
    return CameraNearClip;
}

float FSubCamera::GetCameraFarClip() const
{
    return CameraFarClip;
}

void FSubCamera::Reset()
{
    Pitch = 0.0f;
    Yaw = 0.0f;
    FOV = 90.0f;
    
    ViewMatrix = FMatrix::Identity;
    ProjectionMatrix = FMatrix::Identity;

    StartVector = DirectX::XMVectorZero();
    InitialOrientation = DirectX::XMQuaternionIdentity();
    Orientation = DirectX::XMQuaternionIdentity();

    CameraLocation = FVector(0, 0, 10);
    CameraNearClip = 0.1f;
    CameraFarClip = 1000.0f;

    UpdateViewMatrix();
    UpdateCamera(LastWidth, LastHeight);
}

DirectX::XMVECTOR FSubCamera::MapToSphere(int X, int Y, HWND hWnd)
{
    RECT Rect; GetClientRect(hWnd, &Rect);
    float Width  = float(Rect.right - Rect.left);
    float Height = float(Rect.bottom - Rect.top);

    // 정규화된 스크린 좌표 [-1,1]
    float NX = (2.0f * X - Width)  / Width;
    float NY = (Height - 2.0f * Y) / Height;

    float Length2 = NX*NX + NY*NY;
    if (Length2 > 1.0f)
    {
        float Norm = 1.0f / sqrtf(Length2);
        return DirectX::XMVectorSet(NX * Norm, NY * Norm, 0.0f, 0.0f);
    }
    else
    {
        return DirectX::XMVectorSet(NX, NY, sqrtf(1.0f - Length2), 0.0f);
    }
}
