#pragma once
#include "FSubEngine.h"

class FSkeletalSubEngine : public FSubEngine
{
public:
    FSkeletalSubEngine();
    ~FSkeletalSubEngine();
public:
    virtual void Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,UnrealEd* InUnrealEd);
    virtual void Tick(float DeltaTime);
    virtual void Input(float DeltaTime);
    virtual void Render();
    virtual void Release();
};
