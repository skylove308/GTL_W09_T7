#pragma once
#include "Core/HAL/PlatformType.h"
#include "Engine/ResourceMgr.h"
#include "LevelEditor/SlateAppMessageHandler.h"
#include "Renderer/Renderer.h"
#include "UnrealEd/PrimitiveDrawBatch.h"
#include "Stats/ProfilerStatsManager.h"
#include "Stats/GPUTimingManager.h"


class FSubCamera;
class FSubRenderer;
class USkeletalMesh;
class FImGuiSubWindow;
class FSlateAppMessageHandler;
class UnrealEd;
class UImGuiManager;
class UWorld;
class FEditorViewportClient;
class SSplitterV;
class SSplitterH;
class FGraphicDevice;
class SLevelEditor;
class FDXDBufferManager;

class FEngineLoop
{
public:
    FEngineLoop();

    int32 PreInit();
    int32 Init(HINSTANCE hInstance);
    void Render(float DeltaTime);
    void Tick();
    void Exit();

    void GetClientSize(uint32& OutWidth, uint32& OutHeight) const;
    static void ToggleContentDrawer();

private:
    void WindowInit(HINSTANCE hInstance);
    static LRESULT CALLBACK AppWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    void UpdateUI();
public:
    static FGraphicsDevice GraphicDevice;
    static FRenderer Renderer;
    static UPrimitiveDrawBatch PrimitiveDrawBatch;
    static FResourceMgr ResourceManager;

    HWND AppWnd;
    
    FGPUTimingManager GPUTimingManager;
    FEngineProfiler EngineProfiler;

    static FGraphicsDevice SkeletalViewerGD;
    HWND SkeletalViewerWnd;
    static FGraphicsDevice AnimationViewerGD;
    HWND AnimationViewerWnd;
    void RenderSubWindow();
    void CleanupSubWindow();
    void RequestShowWindow(bool bShow);
    void SubWindowInit(HINSTANCE hInstance);
    void SelectSkeletalMesh(USkeletalMesh* SkeletalMesh);
    USkeletalMesh* GetSelectedSkeletalMesh() const { return SelectedSkeletalMesh; }
    /** Sub Window Handle - Skeletal Mesh Viewer */

    FSubRenderer* SubRenderer;
    FSubCamera* SubCamera;
    FImGuiSubWindow* SubUI;
    USkeletalMesh* SelectedSkeletalMesh;
    bool bIsShowSubWindow = false;
private:
    UImGuiManager* FUIManager;
    ImGuiContext* CurrentImGuiContext;
    
    //TODO: Editor들 EditorEngine으로 넣기

    std::unique_ptr<FSlateAppMessageHandler> AppMessageHandler;
    SLevelEditor* LevelEditor;
    UnrealEd* UnrealEditor;
    FDXDBufferManager* BufferManager; //TODO: UEngine으로 옮겨야함.


    

    
    bool bIsExit = false;

    int32 TargetFPS = 999;
public:
    SLevelEditor* GetLevelEditor() const { return LevelEditor; }
    UnrealEd* GetUnrealEditor() const { return UnrealEditor; }
    
    FSlateAppMessageHandler* GetAppMessageHandler() const { return AppMessageHandler.get(); }
};

