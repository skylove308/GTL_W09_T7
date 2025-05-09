#include "FSubEngine.h"

#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "SubCamera.h"
#include "SubRenderer.h"

FSubEngine::FSubEngine() :
Graphics(nullptr),
BufferManager(nullptr),
Wnd(nullptr),
SubRenderer(nullptr),
UnrealEditor(nullptr),
SubUI(nullptr),
SubCamera(nullptr),
bIsShowSubWindow(false),
SelectedSkeletalMesh(nullptr)
{
}

FSubEngine::~FSubEngine()
{
}

void FSubEngine::Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* UIManger,UnrealEd* InUnrealEd)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    Wnd = &hWnd;
    SubRenderer = new FSubRenderer;
    UnrealEditor = InUnrealEd;
    SubUI = new FImGuiSubWindow(hWnd, InGraphics->Device, InGraphics->DeviceContext);
    UImGuiManager::ApplySharedStyle(UIManger->GetContext(), SubUI->Context);
    SubRenderer->Initialize(InGraphics, InBufferManager);
    SubCamera = new FSubCamera(800,600);
}

void FSubEngine::Tick(float DeltaTime)
{
    Render(DeltaTime);    
}

void FSubEngine::Render(float DeltaTime)
{
    if (Wnd && IsWindowVisible(*Wnd) && Graphics->Device)
    {
        Graphics->Prepare();
        
        SubRenderer->PrepareRender(*SubCamera);
        SubRenderer->Render(*SubCamera);
        SubRenderer->ClearRender();
        
        // Sub window rendering
        SubUI->BeginFrame();

        UnrealEditor->RenderSubWindowPanel();
        
        SubUI->EndFrame();
        
        // Sub swap
        Graphics->SwapBuffer();
    }
}

void FSubEngine::Release()
{
    if (SubUI)
    {
        SubUI->Shutdown();
        delete SubUI;
        SubUI = nullptr;
    }
    if (SubRenderer)
    {
        SubRenderer->Release();
        delete SubRenderer;
        SubRenderer = nullptr;
    }
}

void FSubEngine::RequestShowWindow(bool bShow)
{
    bIsShowSubWindow = bShow;
}
