#pragma once
#include "Math/Matrix.h"

class FSubCamera
{
public:
    FSubCamera(float Width, float Height);
    ~FSubCamera() = default;

    /** Camera Rotation (Params. Radians) */
    void Rotate(float InPitch, float InYaw);

    /** Aspect Ratio */
    void UpdateCamera(float Width, float Height);

    void UpdateViewMatrix();
    
    void CalculateProjection();

    void UpdateFOV(float InFOV);

public:
    void OnMouseDownRight(int MouseX, int MouseY, HWND hWnd);
    void OnMouseUpRight();
    void OnMouseMove(int MouseX, int MouseY, HWND hWnd);
    void OnMouseWheel(short WheelDelta);
    
public:
    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix() const;
    FVector GetCameraLocation() const;

    float GetCameraNearClip() const;
    float GetCameraFarClip() const;

private:
    FMatrix ViewMatrix;
    FMatrix ProjectionMatrix;
    FVector CameraLocation;

    float CameraNearClip;
    float CameraFarClip;

    /** Value is degree */
    float FOV;
    
    float Pitch, Yaw;
    float AspectRatio;

    bool bDrag = false;
    
private:
    DirectX::XMVECTOR StartVector;
    DirectX::XMVECTOR InitialOrientation;
    DirectX::XMVECTOR Orientation;

    DirectX::XMFLOAT3 Target = { 0, 0,0 };

    DirectX::XMVECTOR MapToSphere(int X, int Y, HWND hWnd);
};
