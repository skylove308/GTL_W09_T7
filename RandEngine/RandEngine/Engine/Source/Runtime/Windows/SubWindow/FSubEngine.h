#pragma once
#include "WindowsCursor.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Engine/EditorEngine.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/UnrealEd.h"

class FSubEngine
{
public:
    FSubEngine();
    ~FSubEngine();

    void Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,UnrealEd* InUnrealEd);
    void Tick(float DeltaTime);
    void Input(float DeltaTime);
    void Render();
    void Release();
    void RequestShowWindow(bool bShow);
    void SeltSkeletalMesh(USkeletalMesh* InSkeletalMesh);
public:
    HWND* Wnd;
    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    FSubRenderer* SubRenderer;
    UnrealEd* UnrealEditor;
    FImGuiSubWindow* SubUI;
    USkeletalMesh* SelectedSkeletalMesh;
    
    POINT LastMousePos;

    FEditorViewportClient* ViewportClient;
    bool bIsShowSubWindow;

private:
    bool bAClicked =false;
    bool bWClicked =false;
    bool bDClicked =false;
    bool bSClicked =false;
    bool bRBClicked =false;

};

