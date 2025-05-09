#pragma once
#include "WindowsCursor.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Engine/EditorEngine.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/UnrealEd.h"

//자신의 꿈을 펼쳐볼 수 있는 서브 엔진입니다.
class FSubEngine
{
public:
    FSubEngine();
    ~FSubEngine();

    virtual void Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,UnrealEd* InUnrealEd);
    virtual void Tick(float DeltaTime);
    virtual void Input(float DeltaTime);
    virtual void Render();
    virtual void Release();
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
    bool bRBClicked =false;

    FEditorViewportClient* ViewportClient;
    bool bIsShowSubWindow;

private:
};

