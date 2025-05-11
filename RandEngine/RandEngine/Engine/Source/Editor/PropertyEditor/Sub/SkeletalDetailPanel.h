#pragma once
#include "GameFramework/Actor.h"

class USkeleton;

class SkeletalDetailPanel
{
public:
     void Render(USkeletalMesh* InSkeletalMesh, int32 SelectedBone);
     void OnResize(HWND hWnd);

private:
    float Width = 800, Height = 600;

    FVector PrevLocation;
    FRotator PrevRotation;
    FVector PrevScale;
};
