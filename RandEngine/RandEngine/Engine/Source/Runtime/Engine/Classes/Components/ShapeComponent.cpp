
#include "ShapeComponent.h"

UShapeComponent::UShapeComponent()
{
}

void UShapeComponent::TickComponent(float DeltaTime)
{
    UPrimitiveComponent::TickComponent(DeltaTime);

    UpdateOverlaps();
}
