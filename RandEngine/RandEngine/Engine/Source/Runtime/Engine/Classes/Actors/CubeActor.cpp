#include "CubeActor.h"
#include "Components/BoxComponent.h"

ACubeActor::ACubeActor()
{
    BoxComponent = AddComponent<UBoxComponent>();
    RootComponent = BoxComponent;
}

UBoxComponent* ACubeActor::GetShapeComponent() const
{
    return BoxComponent;
}
