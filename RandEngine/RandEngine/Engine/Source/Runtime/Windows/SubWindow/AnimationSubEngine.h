#pragma once
#include "SkeletalSubEngine.h"
#include "SubEngine.h"


class ACube;
class ASkeletalMeshActor;
class SAnimationTimelinePanel;

class UAnimationSubEngine : public USkeletalSubEngine
{
    DECLARE_CLASS(UAnimationSubEngine, USkeletalSubEngine)
public:
    UAnimationSubEngine();
    ~UAnimationSubEngine();
public:
    virtual void Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,UnrealEd* InUnrealEd);
    virtual void Tick(float DeltaTime);
    virtual void Input(float DeltaTime);
    virtual void Render();
    virtual void Release();
    
};
