#include "SkeletalViewerPanel.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "SubWindow/SkeletalSubEngine.h"
#include "SubWindow/SubEngine.h"
#include "Components/Mesh/SkeletalMesh.h"

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
    USkeletalMesh* Selected = static_cast<FSkeletalSubEngine*>(GEngineLoop.SkeletalViewerSubEngine)->SelectedSkeletalMesh;

    const FReferenceSkeleton& RefSkeleton = Selected->Skeleton->ReferenceSkeleton;
    const TArray<FBoneNode>& BoneNodes = RefSkeleton.BoneInfo;

    // 1. 뼈 계층 구조 분석
    TMap<int32, TArray<int32>> BoneHierarchy;
    for (int32 BoneIdx = 0; BoneIdx < BoneNodes.Num(); ++BoneIdx)
    {
        const int32 ParentIdx = BoneNodes[BoneIdx].ParentIndex;
        BoneHierarchy.FindOrAdd(ParentIdx).Add(BoneIdx);
    }

    // 2. 루트 본 찾기
    const TArray<int32>& RootBones = BoneHierarchy.FindOrAdd(INDEX_NONE);

    // 3. 계층 구조 렌더링
    for (int32 RootBoneIdx : RootBones)
    {
        RenderBoneHierarchy(RootBoneIdx, BoneNodes, BoneHierarchy);
    }
}

// 재귀적 뼈 계층 구조 렌더링 함수
void SkeletalViewerPanel::RenderBoneHierarchy(
    int32 CurrentBoneIdx,
    const TArray<FBoneNode>& BoneNodes,
    const TMap<int32, TArray<int32>>& BoneHierarchy)
{
    const FBoneNode& CurrentBone = BoneNodes[CurrentBoneIdx];
    const FString BoneName = CurrentBone.Name.ToString();

    // 4. 트리 노드 생성 및 자식 처리
    if (ImGui::TreeNode(BoneName.IsEmpty() ? "Unnamed Bone" : (*BoneName)))
    {
        if (BoneHierarchy.Contains(CurrentBoneIdx))
        {
            for (int32 ChildBoneIdx : BoneHierarchy[CurrentBoneIdx])
            {
                RenderBoneHierarchy(ChildBoneIdx, BoneNodes, BoneHierarchy);
            }
        }
        ImGui::TreePop();
    }
}
