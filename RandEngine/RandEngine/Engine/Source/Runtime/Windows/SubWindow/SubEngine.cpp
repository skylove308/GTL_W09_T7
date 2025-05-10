#include "SubEngine.h"

#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "SubRenderer.h"
#include "UnrealClient.h"
#include "UnrealEd/EditorViewportClient.h"

USubEngine::USubEngine() :
                         Graphics(nullptr),
                         BufferManager(nullptr),
                         Wnd(nullptr),
                         SubRenderer(nullptr),
                         UnrealEditor(nullptr),
                         SubUI(nullptr),
                         bIsShowSubWindow(false)
{
}

USubEngine::~USubEngine()
{
}

void USubEngine::Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,UnrealEd* InUnrealEd)
{
}

void USubEngine::Input(float DeltaTime)
{

}

void USubEngine::Tick(float DeltaTime)
{
}

void USubEngine::Render()
{

}

void USubEngine::Release()
{
}

void USubEngine::RequestShowWindow(bool bShow)
{
    bIsShowSubWindow = bShow;
}
