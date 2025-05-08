#pragma once
#include "Define.h"
#include "Rotator.h"
#include "Quat.h"

//  Near Clip Plane 값을 정의한 헤더
#ifndef NEAR_PLANE
#define NEAR_PLANE 1.f 
#endif

class JungleMath
{
public:
    static FVector4 ConvertV3ToV4(FVector vec3);
    static FMatrix CreateModelMatrix(FVector translation, FVector rotation, FVector scale);
    static FMatrix CreateModelMatrix(FVector translation, FQuat rotation, FVector scale);
    static FMatrix CreateViewMatrix(FVector eye, FVector target, FVector up);
    static FMatrix CreateProjectionMatrix(float fov, float aspect, float nearPlane, float farPlane);
    static FMatrix CreateOrthoProjectionMatrix(float width, float height, float nearPlane, float farPlane);
    static FMatrix CreateOrthographicOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane);

    static FVector FVectorRotate(FVector& origin, const FVector& InRotation);
    static FVector FVectorRotate(FVector& origin, const FRotator& InRotation);
    static FMatrix CreateRotationMatrix(FVector rotation);
    static FQuat EulerToQuaternion(const FVector& eulerDegrees);
    static FVector QuaternionToEuler(const FQuat& quat);
};
