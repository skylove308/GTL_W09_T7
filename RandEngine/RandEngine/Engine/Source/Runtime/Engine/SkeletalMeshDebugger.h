#pragma once

#include "Define.h"

class USkeletalMeshComponent;

class FSkeletalMeshDebugger
{
public:
    static void DrawSkeleton(const USkeletalMeshComponent* SkelMeshComp);
    static void DrawSkeletonAABBs(const USkeletalMeshComponent* SkelMeshComp);
};
