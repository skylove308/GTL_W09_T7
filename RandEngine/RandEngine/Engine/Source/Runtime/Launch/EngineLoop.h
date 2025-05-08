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
    void RenderSubWindow() const;
    void Tick();
    void Exit();
    void CleanupSubWindow();
    void RequestShowWindow(bool bShow);

    void GetClientSize(uint32& OutWidth, uint32& OutHeight) const;
    static void ToggleContentDrawer();

private:
    void WindowInit(HINSTANCE hInstance);
    void SubWindowInit(HINSTANCE hInstance);
    static LRESULT CALLBACK AppWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    void UpdateUI();

public:
    static FGraphicsDevice GraphicDevice;
    static FGraphicsDevice SubGraphicDevice;
    static FRenderer Renderer;
    static UPrimitiveDrawBatch PrimitiveDrawBatch;
    static FResourceMgr ResourceManager;
    static uint32 TotalAllocationBytes;
    static uint32 TotalAllocationCount;

    HWND AppWnd;

    /** Sub Window Handle - Skeletal Mesh Viewer */
    HWND SubAppWnd;

    FGPUTimingManager GPUTimingManager;
    FEngineProfiler EngineProfiler;

private:
    UImGuiManager* FUIManager;
    FImGuiSubWindow* SubUI;
    ImGuiContext* CurrentImGuiContext;
    USkeletalMesh* SelectedSkeletalMesh;
    //TODO: Editor들 EditorEngine으로 넣기

    std::unique_ptr<FSlateAppMessageHandler> AppMessageHandler;
    SLevelEditor* LevelEditor;
    UnrealEd* UnrealEditor;
    FDXDBufferManager* BufferManager; //TODO: UEngine으로 옮겨야함.

    FSubRenderer* SubRenderer;
    FSubCamera* SubCamera;
    
    bool bIsExit = false;
    bool bIsShowSubWindow = false;
    // @todo Option으로 선택 가능하도록
    int32 TargetFPS = 999;

public:
    SLevelEditor* GetLevelEditor() const { return LevelEditor; }
    UnrealEd* GetUnrealEditor() const { return UnrealEditor; }
    void SelectSkeletalMesh(USkeletalMesh* SkeletalMesh);
    USkeletalMesh* GetSelectedSkeletalMesh() const { return SelectedSkeletalMesh; }
    FSlateAppMessageHandler* GetAppMessageHandler() const { return AppMessageHandler.get(); }
};

