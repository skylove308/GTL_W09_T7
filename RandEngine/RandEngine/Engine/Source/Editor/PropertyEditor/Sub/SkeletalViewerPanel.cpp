#include "SkeletalViewerPanel.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "SubWindow/SkeletalSubEngine.h"
#include "SubWindow/SubEngine.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Engine/SkeletalMeshActor.h"

void SkeletalViewerPanel::Render()
{
    ImVec2 WinSize = ImVec2(Width, Height);
    
    // ImGui::SetNextWindowPos(ImVec2(WinSize.x * 0.75f + 2.0f, 2));
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::SetNextWindowSize(ImVec2(WinSize.x * 0.2f - 5.0f, WinSize.y));
    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    
    ImGui::Begin("Skeletal Tree", nullptr, PanelFlags);

    CreateSkeletalTreeNode();
    
    ImGui::End();
    DetailPanel.Render(SkeletalMesh,SelectedBoneIdx);
}

void SkeletalViewerPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
    DetailPanel.OnResize(hWnd);
}

void SkeletalViewerPanel::CreateSkeletalTreeNode()
{
    USkeletalSubEngine* SubEngine = nullptr;
    if (WindowType == WT_SkeletalSubWindow)
        SubEngine = static_cast<USkeletalSubEngine*>(GEngineLoop.SkeletalViewerSubEngine);
    else if (WindowType == WT_AnimationSubWindow)
        SubEngine = static_cast<USkeletalSubEngine*>(GEngineLoop.AnimationViewerSubEngine);

    SkeletalMesh = SubEngine->SelectedSkeletalMesh;

    // if (Skeleton == Selected->Skeleton)
    //     return;
    // else

    const FReferenceSkeleton& RefSkeleton = SkeletalMesh->Skeleton->ReferenceSkeleton;
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
    const FString BoneName = CurrentBone.Name.ToString().IsEmpty() ? TEXT("Unnamed Bone") : CurrentBone.Name.ToString();

    // selectable 트리 노드 플래그 설정
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow 
        | ImGuiTreeNodeFlags_SpanAvailWidth 
        | ImGuiTreeNodeFlags_Leaf * !BoneHierarchy.Contains(CurrentBoneIdx)
        | ImGuiTreeNodeFlags_Selected * (CurrentBoneIdx == SelectedBoneIdx)
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_DefaultOpen; // 원하는 경우 기본 열기

    // TreeNodeEx 호출 → 반환값이 열림 여부
    bool nodeOpen = ImGui::TreeNodeEx(
        (void*)(intptr_t)CurrentBoneIdx, // 고유 ID
        nodeFlags,
        "%s",
        *BoneName
    );

    // 클릭 시 선택 처리
    if (ImGui::IsItemClicked())
    {
        SelectedBoneIdx = CurrentBoneIdx;
        // 필요하다면, 이름도 저장
        if (WindowType == WT_SkeletalSubWindow)
        {
            USkeletalSubEngine* SubEngine = static_cast<USkeletalSubEngine*>(GEngineLoop.SkeletalViewerSubEngine);
            SubEngine->SelectedComponent = SubEngine->SkeletalMeshActor->BoneGizmoSceneComponents[SelectedBoneIdx];
        }
        else if (WindowType == WT_AnimationSubWindow)
        {
            USkeletalSubEngine* SubEngine = static_cast<USkeletalSubEngine*>(GEngineLoop.AnimationViewerSubEngine);
            SubEngine->SelectedComponent = SubEngine->SkeletalMeshActor->BoneGizmoSceneComponents[SelectedBoneIdx];
        }
    }

    if (nodeOpen)
    {
        if (BoneHierarchy.Contains(CurrentBoneIdx))
        {
            for (int32 ChildIdx : BoneHierarchy[CurrentBoneIdx])
            {
                RenderBoneHierarchy(ChildIdx, BoneNodes, BoneHierarchy);
            }
        }
        ImGui::TreePop();
    }
}
