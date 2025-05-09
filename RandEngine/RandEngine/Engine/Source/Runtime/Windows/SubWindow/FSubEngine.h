#pragma once
#include "GameFramework/Actor.h"
#include "UnrealEd/UnrealEd.h"

class FSubEngine
{
public:
    FSubEngine();
    ~FSubEngine();

    void Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,UnrealEd* InUnrealEd);
    void Tick(float DeltaTime);
    void Render(float DeltaTime);
    void Release();
    void RequestShowWindow(bool bShow);
public:
    HWND* Wnd;
    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    FSubRenderer* SubRenderer;
    FSubCamera* SubCamera;
    UnrealEd* UnrealEditor;
    FImGuiSubWindow* SubUI;
    USkeletalMesh* SelectedSkeletalMesh;
    bool bIsShowSubWindow;
};
