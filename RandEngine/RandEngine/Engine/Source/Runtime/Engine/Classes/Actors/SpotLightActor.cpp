#include "SpotLightActor.h"
#include "Components/Light/SpotLightComponent.h"
#include "Components/BillboardComponent.h"
ASpotLight::ASpotLight()
{
    SpotLightComponent = AddComponent<USpotLightComponent>("USpotLightComponent_0");
    BillboardComponent = AddComponent<UBillboardComponent>("UBillboardComponent_0");

    RootComponent = BillboardComponent;

    BillboardComponent->SetTexture(L"Assets/Editor/Icon/S_LightSpot.PNG");
    BillboardComponent->bIsEditorBillboard = true;

    SpotLightComponent->AttachToComponent(RootComponent);
}

ASpotLight::~ASpotLight()
{
}
