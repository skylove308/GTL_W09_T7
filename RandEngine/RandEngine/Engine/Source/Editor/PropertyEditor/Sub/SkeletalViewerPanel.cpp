#include "SkeletalViewerPanel.h"

#include "Components/Mesh/SkeletalMeshRenderData.h"
#include "Engine/Asset/SkeletalMeshAsset.h"

void SkeletalViewerPanel::Render()
{
    ImVec2 WinSize = ImVec2(Width, Height);
    
    ImGui::SetNextWindowPos(ImVec2(WinSize.x * 0.75f + 2.0f, 2));
    ImGui::SetNextWindowSize(ImVec2(WinSize.x * 0.25f - 5.0f, WinSize.y - 5.0f));
    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    
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

    FSkeletalHierarchyData Data = Selected->GetRenderData()->RootSkeletal;
    
    if (!Data.NodeName.IsEmpty() || !Data.Children.IsEmpty())
    {
        RenderSkeletalTreeNode(Data);
    }
}

void SkeletalViewerPanel::RenderSkeletalTreeNode(const FSkeletalHierarchyData& Node)
{
    if (ImGui::TreeNode(Node.NodeName.IsEmpty() ? "undefined" : GetData(Node.NodeName)))
    {
        for (const FSkeletalHierarchyData& ChildNode : Node.Children)
        {
            RenderSkeletalTreeNode(ChildNode);
        }
        ImGui::TreePop();
    }

}
