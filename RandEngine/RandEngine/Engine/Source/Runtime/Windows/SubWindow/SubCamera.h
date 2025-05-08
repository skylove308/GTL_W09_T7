#pragma once
#include "Math/Matrix.h"

class FSubCamera
{
public:
    FSubCamera(float Width, float Height);
    ~FSubCamera() = default;
    
    /** Aspect Ratio */
    void UpdateCamera(float Width = 0, float Height = 0);

    void UpdateViewMatrix();
    
    void CalculateProjection();

    void SetTargetPosition(float X, float Y, float Z);

    void SetTargetZOffset(float ZOffset);

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

    void Reset();
    
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
    float LastWidth;
    float LastHeight;

    bool bDrag = false;
    
private:
    DirectX::XMVECTOR StartVector;
    DirectX::XMVECTOR InitialOrientation;
    DirectX::XMVECTOR Orientation;

    DirectX::XMFLOAT3 Target = { 0, 0, 0 };

    DirectX::XMVECTOR MapToSphere(int X, int Y, HWND hWnd);
};
