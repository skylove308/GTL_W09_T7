#include "FSubWindow.h"

#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "SubCamera.h"
#include "SubRenderer.h"

void FSubWindow::Initialize(HWND hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* UIManger)
{
    SubRenderer = new FSubRenderer;
    
    SubUI = new FImGuiSubWindow(hWnd, InGraphics->Device, InGraphics->DeviceContext);
    UImGuiManager::ApplySharedStyle(UIManger->GetContext(), SubUI->Context);
    SubRenderer->Initialize(InGraphics, InBufferManager);
    SubCamera = new FSubCamera(800,600);
}

void FSubWindow::Release()
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
