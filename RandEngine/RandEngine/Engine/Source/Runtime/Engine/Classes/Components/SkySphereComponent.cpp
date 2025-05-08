#include "SkySphereComponent.h"
#include "UObject/Casts.h"


USkySphereComponent::USkySphereComponent()
{
    SetType(StaticClass()->GetName());
}

UObject* USkySphereComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->UOffset = UOffset;
    NewComponent->VOffset = VOffset;

    return NewComponent;
}

void USkySphereComponent::TickComponent(float DeltaTime)
{
    UOffset += 0.005f;
    VOffset += 0.005f;
    Super::TickComponent(DeltaTime);
}
