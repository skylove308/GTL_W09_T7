#pragma once
#include "CameraModifier.h"

enum class ECameraShakeType : uint8;
class UCameraShakeBase;

class UCameraModifier_CameraShake : public UCameraModifier
{
    DECLARE_CLASS(UCameraModifier_CameraShake, UCameraModifier)

public:
    UCameraModifier_CameraShake() = default;
    virtual ~UCameraModifier_CameraShake() override;

    virtual UCameraShakeBase* AddCameraShake(UClass* ShakeClass);

    virtual void RemoveCameraShake(UCameraShakeBase* ShakeInst, bool bImmediately = true);

    virtual void RemoveAllCameraShakesOfClass(UClass* ShakeClass, bool bImmediately = true);
    
    virtual void RemoveAllCameraShakes(bool bImmediately = true);

    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

protected:
    TArray<UCameraShakeBase*> ActiveShakes;

    // TODO: 현재 Pool 작동 안됨.
    TMap<UClass*, TArray<UCameraShakeBase*>> ExpiredPooledShakesMap;

    void SaveShakeInExpiredPool(UCameraShakeBase* ShakeInst);
    UCameraShakeBase* ReclaimShakeFromExpiredPool(UClass* ShakeClass);
};
