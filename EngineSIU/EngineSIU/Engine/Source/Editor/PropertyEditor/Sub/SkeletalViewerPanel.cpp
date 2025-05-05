#include "SkeletalViewerPanel.h"

#include "Components/Mesh/SkeletalMeshRenderData.h"

void SkeletalViewerPanel::Render()
{
    ImVec2 WinSize = ImVec2(Width, Height);
    
    ImGui::SetNextWindowPos(ImVec2(WinSize.x * 0.75f + 2.0f, 2));
    ImGui::SetNextWindowSize(ImVec2(WinSize.x * 0.25f - 5.0f, WinSize.y - 5.0f));
    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    
    ImGui::Begin("Skeletal Tree", nullptr, PanelFlags);

    CreateSkeletalTreeNode();
    
    ImGui::End();
}

void SkeletalViewerPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}

void SkeletalViewerPanel::CreateSkeletalTreeNode()
{
    USkeletalMesh* Selected = GEngineLoop.GetSelectedSkeletalMesh();

    if (Selected == nullptr) return;
    
    ImGui::Selectable(GetData(Selected->GetOjbectName()));
}
