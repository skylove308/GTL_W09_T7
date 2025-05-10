#include "AnimationSubEngine.h"
#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "SubRenderer.h"
#include "UnrealClient.h"

UAnimationSubEngine::UAnimationSubEngine()
{
}

UAnimationSubEngine::~UAnimationSubEngine()
{
}

void UAnimationSubEngine::Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,
    UnrealEd* InUnrealEd)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    Wnd = &hWnd;
    SubRenderer = new FSubRenderer;
    UnrealEditor = InUnrealEd;
    SubUI = new FImGuiSubWindow(hWnd, InGraphics->Device, InGraphics->DeviceContext);
    UImGuiManager::ApplySharedStyle(InSubWindow->GetContext(), SubUI->Context);
    SubRenderer->Initialize(InGraphics, InBufferManager);

    ViewportClient = new FEditorViewportClient();
    ViewportClient->Initialize(EViewScreenLocation::EVL_MAX, FRect(0,0,800,600));
    ViewportClient->CameraReset();
    // EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>(this);
}

void UAnimationSubEngine::Tick(float DeltaTime)
{
    Input(DeltaTime);
    ViewportClient->Tick(DeltaTime);
    // EditorPlayer->Tick(DeltaTime);
    Render();    
}

void UAnimationSubEngine::Input(float DeltaTime)
{
    if (::GetForegroundWindow() != *Wnd)
        return;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        if (!bRBClicked)
        {
            bRBClicked = true;
            GetCursorPos(&LastMousePos);
        }
        POINT CursorPos;
        GetCursorPos(&CursorPos);

        float DeltaX = CursorPos.x - LastMousePos.x;
        float DeltaY = CursorPos.y - LastMousePos.y;
        ViewportClient->CameraRotateYaw(DeltaX * 0.1f);
        ViewportClient->CameraRotatePitch(DeltaY*0.1f);
        LastMousePos = CursorPos;
    }
    else
    {
        if (bRBClicked)
        {
            bRBClicked = false;
        }
    }
    if (bRBClicked)
    {
        if (GetAsyncKeyState('A') & 0x8000)
        {
            ViewportClient->CameraMoveRight(-100.f * DeltaTime);
        }
        if (GetAsyncKeyState('D') & 0x8000)
        {
            ViewportClient->CameraMoveRight(100.f * DeltaTime);
        }
        if (GetAsyncKeyState('W') & 0x8000)
        {
            ViewportClient->CameraMoveForward(100.f * DeltaTime);
        }
        if (GetAsyncKeyState('S') & 0x8000)
        {
            ViewportClient->CameraMoveForward(-100.f * DeltaTime);
        }
        if (GetAsyncKeyState('E') & 0x8000)
        {
            ViewportClient->CameraMoveUp(100.f * DeltaTime);
        }
        if (GetAsyncKeyState('Q') & 0x8000)
        {
            ViewportClient->CameraMoveUp(-100.f * DeltaTime);
        }
    }
}

void UAnimationSubEngine::Render()
{
    if (Wnd && IsWindowVisible(*Wnd) && Graphics->Device)
    {
        Graphics->Prepare();
        
        SubRenderer->PrepareRender(ViewportClient);
        SubRenderer->Render();
        SubRenderer->ClearRender();
        
        // Sub window rendering
        SubUI->BeginFrame();

        UnrealEditor->Render(EWindowType::WT_AnimationSubWindow);
       
        SubUI->EndFrame();
         
        // Sub swap
        Graphics->SwapBuffer();
    }
}

void UAnimationSubEngine::Release()
{
    USubEngine::Release();
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

void UAnimationSubEngine::SetSkeletalMesh(USkeletalMesh* InSkeletalMesh)
{
    SelectedSkeletalMesh = InSkeletalMesh;
    if (SubRenderer)
    {
        SubRenderer->SetPreviewSkeletalMesh(SelectedSkeletalMesh);
    }
}
