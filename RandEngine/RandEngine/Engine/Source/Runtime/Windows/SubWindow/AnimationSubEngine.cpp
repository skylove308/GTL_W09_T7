#include "AnimationSubEngine.h"
#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "SubRenderer.h"
#include "UnrealClient.h"
#include "Actors/Cube.h"
#include "Animation/Skeleton.h"

UAnimationSubEngine::UAnimationSubEngine()
{
}

UAnimationSubEngine::~UAnimationSubEngine()
{
}

void UAnimationSubEngine::Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,
    UnrealEd* InUnrealEd)
{
    USkeletalSubEngine::Initialize(hWnd,InGraphics, InBufferManager, InSubWindow,InUnrealEd);
}

void UAnimationSubEngine::Tick(float DeltaTime)
{
    USkeletalSubEngine::Tick(DeltaTime);  
}

void UAnimationSubEngine::Input(float DeltaTime)
{
    USkeletalSubEngine::Input(DeltaTime);
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
    USkeletalSubEngine::Release();
}
