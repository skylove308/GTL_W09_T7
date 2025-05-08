#include "Drawer.h"

// #include "Engine/FbxLoader.h"
#include "Components/Mesh/SkeletalMeshRenderData.h"
#include "ImGui/imgui_internal.h"
#include "UObject/UObjectIterator.h"

FDrawer& FDrawer::GetInstance()
{
    static FDrawer Instance;
    return Instance;
}

void FDrawer::Toggle()
{
    bIsOpen = !bIsOpen;
    if (!bFirstOpenFrame)
    {
        bFirstOpenFrame = true;
    }
}

void FDrawer::Render(float DeltaTime)
{
    if (!bIsOpen)
        return;

    ImVec2 WinSize = ImVec2(Width, Height);

    // 목표 위치
    float TargetY = WinSize.y * 0.75f;
    float StartY  = WinSize.y; // 아래에서 올라오게
    float CurrentY = TargetY;

    if (bFirstOpenFrame)
    {
        AnimationTime = 0.0f;
        bFirstOpenFrame = false;
    }

    // 애니메이션 진행
    if (AnimationTime < AnimationDuration)
    {
        AnimationTime += DeltaTime;
        float T = AnimationTime / AnimationDuration;
        T = ImClamp(T, 0.0f, 1.0f);
        T = ImGui::GetStyle().Alpha * T; // 곡선 보간을 원한다면 여기서 ease 적용
        CurrentY = ImLerp(StartY, TargetY, T);
    }

    ImGui::SetNextWindowPos(ImVec2(5, CurrentY));
    ImGui::SetNextWindowSize(ImVec2(WinSize.x - 10.0f, WinSize.y * 0.25f));
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Content Drawer", nullptr, PanelFlags);
    RenderContentDrawer();
    ImGui::End();
}

void FDrawer::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}

void FDrawer::RenderContentDrawer()
{
    for (auto Obj : TObjectRange<USkeletalMesh>())
    {
        ImGui::Selectable(GetData(Obj->GetObjectName()));
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            UE_LOG(ELogLevel::Display, TEXT("Double Clicked"));
            GEngineLoop.SelectSkeletalMesh(Obj);
            GEngineLoop.RequestShowWindow(true);
            break;
        }
    }
}
