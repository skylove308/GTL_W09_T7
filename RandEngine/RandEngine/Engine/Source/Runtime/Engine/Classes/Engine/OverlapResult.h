#pragma once

#include "HAL/PlatformType.h"

class AActor;
class UObject;
class UPrimitiveComponent;

struct FOverlapResult
{
    // FActorInstanceHandle OverlapObjectHandle;
    AActor* Actor;

    /** PrimitiveComponent that the check hit. */
    UPrimitiveComponent* Component;

    /** This is the index of the overlapping item.
        For DestructibleComponents, this is the ChunkInfo index.
        For SkeletalMeshComponents this is the Body index or INDEX_NONE for single body */
    int32 ItemIndex;

    /** Utility to return the Actor that owns the Component that was hit */
    AActor* GetActor() const;

    /** Utility to return the Component that was hit */
    UPrimitiveComponent* GetComponent() const;

    /** The object that owns the PhysicsObject. This is used to determine if the PhysicsObject is still valid when not owned by an Actor */
    // UObject* PhysicsObjectOwner;

    /** PhysicsObjects hit by the query. Not exposed to blueprints for the time being */
    // Chaos::FPhysicsObjectHandle PhysicsObject;

    /** Indicates if this hit was requesting a block - if false, was requesting a touch instead */
    uint32 bBlockingHit : 1;

    FOverlapResult()
    {
        ZeroMemory(this, sizeof(FOverlapResult));
    }
};

inline AActor* FOverlapResult::GetActor() const
{
    return Actor;
}

inline UPrimitiveComponent* FOverlapResult::GetComponent() const
{
    return Component;
}
