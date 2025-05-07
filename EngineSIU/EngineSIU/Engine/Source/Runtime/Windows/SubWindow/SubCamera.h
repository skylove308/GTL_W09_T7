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

    void CalculateProjection();

    void UpdateFOV(float InFOV);
    
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
};
