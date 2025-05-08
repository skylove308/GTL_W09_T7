#pragma once
#include "LightActor.h"
class AAmbientLight : public ALight
{
    DECLARE_CLASS(AAmbientLight, ALight)
public:
    AAmbientLight();
    virtual ~AAmbientLight();

protected:
    UPROPERTY
    (UAmbientLightComponent*, AmbientLightComponent, = nullptr);

    UPROPERTY
    (UBillboardComponent*, BillboardComponent, = nullptr);
};
