#pragma once
#include "GameFramework/Actor.h"

class FSubWindow
{
public:
    FSubWindow();
    ~FSubWindow();

    void Initialize(HWND hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow);
    void Release();
public:
    // HWND SubAppWnd;
    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    FSubRenderer* SubRenderer;
    FSubCamera* SubCamera;
    FImGuiSubWindow* SubUI;
    USkeletalMesh* SelectedSkeletalMesh;
    bool bIsShowSubWindow = false;
};
