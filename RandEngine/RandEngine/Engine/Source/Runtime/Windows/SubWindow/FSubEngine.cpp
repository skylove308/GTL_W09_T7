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

void FSubEngine::Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,UnrealEd* InUnrealEd)
{

}

void FSubEngine::Input(float DeltaTime)
{

}

void FSubEngine::Tick(float DeltaTime)
{

}

void FSubEngine::Render()
{

}

void FSubEngine::Release()
{

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
