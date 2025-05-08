
#include "HitResult.h"

#include "Components/PrimitiveComponent.h"

FHitResult::FHitResult(class AActor* InActor, class UPrimitiveComponent* InComponent, FVector const& HitLoc, FVector const& HitNorm)
{
    ZeroMemory(this, sizeof(FHitResult));
    Location = HitLoc;
    ImpactPoint = HitLoc;
    Normal = HitNorm;
    ImpactNormal = HitNorm;
    HitActor = InActor;
    // HitObjectHandle = FActorInstanceHandle(InActor);
    Component = InComponent;
}

FString FHitResult::ToString() const
{
    return FString::Printf(TEXT("bBlockingHit:%s bStartPenetrating:%s Time:%f Location:%s ImpactPoint:%s Normal:%s ImpactNormal:%s TraceStart:%s TraceEnd:%s PenetrationDepth:%f Item:%d PhysMaterial:%s Actor:%s Component:%s BoneName:%s FaceIndex:%d"),
        bBlockingHit == true ? TEXT("True") : TEXT("False"),
        bStartPenetrating == true ? TEXT("True") : TEXT("False"),
        Time,
        *Location.ToString(),
        *ImpactPoint.ToString(),
        *Normal.ToString(),
        *ImpactNormal.ToString(),
        *TraceStart.ToString(),
        *TraceEnd.ToString(),
        PenetrationDepth,
        Item,
        TEXT("None"), // PhysMaterial.IsValid() ? *PhysMaterial->GetName() : TEXT("None"),
        TEXT("None"), // HitObjectHandle.IsValid() ? *HitObjectHandle.GetName() : TEXT("None"),
        Component ? *Component->GetName() : TEXT("None"), // Component.IsValid() ? *Component->GetName() : TEXT("None"),
        *BoneName.ToString(), // BoneName.IsValid() ? *BoneName.ToString() : TEXT("None"),
        FaceIndex);
}
