#include "FSubEngine.h"

#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "SubCamera.h"
#include "SubRenderer.h"
#include "UnrealClient.h"
#include "UnrealEd/EditorViewportClient.h"

FSubEngine::FSubEngine() :
                         Graphics(nullptr),
                         BufferManager(nullptr),
                         Wnd(nullptr),
                         SubRenderer(nullptr),
                         UnrealEditor(nullptr),
                         SubUI(nullptr),
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

    ViewportClient = new FEditorViewportClient();
    ViewportClient->Initialize(EViewScreenLocation::EVL_MAX, FRect(0,0,800,600));

    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();
    
    // Handler->OnMouseDownDelegate.AddLambda([this](const FPointerEvent& InMouseEvent)
    //     {
    //         if (ImGui::GetIO().WantCaptureMouse) return;
    //
    //         switch (InMouseEvent.GetEffectingButton())  // NOLINT(clang-diagnostic-switch-enum)
    //         {
    //         case EKeys::LeftMouseButton:
    //         {
    //             if (const UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine))
    //             {
    //                 if (const AActor* SelectedActor = EdEngine->GetSelectedActor())
    //                 {
    //                     USceneComponent* TargetComponent = nullptr;
    //                     if (USceneComponent* SelectedComponent = EdEngine->GetSelectedComponent())
    //                     {
    //                         TargetComponent = SelectedComponent;
    //                     }
    //                     else if (AActor* SelectedActor = EdEngine->GetSelectedActor())
    //                     {
    //                         TargetComponent = SelectedActor->GetRootComponent();
    //                     }
    //                     else
    //                     {
    //                         return;
    //                     }
    //
    //                     // 초기 Actor와 Cursor의 거리차를 저장
    //                     const FViewportCamera* ViewTransform = ViewportClient->GetViewportType() == LVT_Perspective
    //                                                         ? &ViewportClient->PerspectiveCamera
    //                                                         : &ViewportClient->OrthogonalCamera;
    //
    //                     FVector RayOrigin, RayDir;
    //                     ViewportClient->DeprojectFVector2D(FWindowsCursor::GetClientPosition(), RayOrigin, RayDir);
    //
    //                     const FVector TargetLocation = TargetComponent->GetWorldLocation();
    //                     const float TargetDist = FVector::Distance(ViewTransform->GetLocation(), TargetLocation);
    //                     const FVector TargetRayEnd = RayOrigin + RayDir * TargetDist;
    //                     TargetDiff = TargetLocation - TargetRayEnd;
    //                 }
    //             }
    //             break;
    //         }
    //         case EKeys::RightMouseButton:
    //         {
    //             if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
    //             {
    //                 FWindowsCursor::SetShowMouseCursor(false);
    //                 MousePinPosition = InMouseEvent.GetScreenSpacePosition();
    //             }
    //             break;
    //         }
    //         default:
    //             break;
    //         }
    //     });
    // Handler->OnKeyDownDelegate.AddLambda([this](const FKeyEvent& InKeyEvent)
    //         {
    //             ViewportClient->InputKey(InKeyEvent);
    //         });
}

void FSubEngine::Input(float DeltaTime)
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

void FSubEngine::Tick(float DeltaTime)
{
    Input(DeltaTime);
    ViewportClient->Tick(DeltaTime);
    Render();    
}

void FSubEngine::Render()
{
    if (Wnd && IsWindowVisible(*Wnd) && Graphics->Device)
    {
        Graphics->Prepare();
        
        SubRenderer->PrepareRender(ViewportClient);
        SubRenderer->Render();
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

void FSubEngine::SeltSkeletalMesh(USkeletalMesh* InSkeletalMesh)
{
    SelectedSkeletalMesh = InSkeletalMesh;
    if (SubRenderer)
    {
        SubRenderer->SetPreviewSkeletalMesh(SelectedSkeletalMesh);
    }
}
