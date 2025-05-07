#include "SubCamera.h"

#include "Math/JungleMath.h"
#include <DirectXMath.h>

FSubCamera::FSubCamera(float Width, float Height)
{
    Pitch = 0.0f;
    Yaw = 0.0f;
    FOV = 90.0f;
    
    ViewMatrix = FMatrix::Identity;
    ProjectionMatrix = FMatrix::Identity;

    CameraLocation = FVector(0, 0, -8);
    CameraNearClip = 0.1f;
    CameraFarClip = 1000.0f;
    
    Rotate(30.0f, 0.0f);
    UpdateCamera(Width, Height);
}

void FSubCamera::Rotate(float InPitch, float InYaw)
{
    Pitch += InPitch;
    Yaw += InYaw;

    Pitch = std::clamp(Pitch, -89.0f, 89.0f);
    Pitch = FMath::RadiansToDegrees(Pitch);
    Yaw = FMath::RadiansToDegrees(Yaw);

    const float Radius = -8.0f;
    DirectX::XMMATRIX PitchMatrix = DirectX::XMMatrixRotationY(Pitch);
    DirectX::XMMATRIX YawMatrix   = DirectX::XMMatrixRotationZ(Yaw);
    
    DirectX::XMMATRIX RotationMatrix = PitchMatrix * YawMatrix;
    DirectX::XMVECTOR Offset = DirectX::XMVectorSet(0.0f, 0.0f, -Radius, 0.0f);
    DirectX::XMVECTOR Position = DirectX::XMVector3TransformCoord(Offset, RotationMatrix);

    DirectX::XMVECTOR at  = DirectX::XMVectorZero();
    DirectX::XMVECTOR up  = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // Z-up
    DirectX::XMVECTOR eye = Position;

    DirectX::XMMATRIX TempView = DirectX::XMMatrixLookAtLH(eye, at, up);

    ViewMatrix = FMatrix::FromXMMatrix(TempView);
}

void FSubCamera::UpdateCamera(float Width, float Height)
{
    AspectRatio = Width / Height;
    CalculateProjection();
}

void FSubCamera::CalculateProjection()
{
    float FOVRadian = FMath::DegreesToRadians(FOV);
    ProjectionMatrix = JungleMath::CreateProjectionMatrix(FOVRadian, AspectRatio, CameraNearClip, CameraFarClip);
}

void FSubCamera::UpdateFOV(float InFOV)
{
    FOV += InFOV;
    FOV = std::clamp(FOV, 0.1f, 179.9f);
    CalculateProjection();
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
