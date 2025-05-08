#include "SphereActor.h"

#include "Components/SphereComponent.h"


ASphereActor::ASphereActor()
{
    SphereComponent = AddComponent<USphereComponent>();
    RootComponent = SphereComponent;
}

USphereComponent* ASphereActor::GetShapeComponent() const
{
    return SphereComponent;
}
