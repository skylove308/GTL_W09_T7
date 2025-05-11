#pragma once
#include "SkeletalDetailPanel.h"
#include "Components/ActorComponent.h"
#include "UnrealEd/EditorPanel.h"

struct FBoneNode;

class USkeleton;

class SkeletalViewerPanel : public UEditorPanel
{
public:
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

private:
    USkeletalMesh* SkeletalMesh =nullptr;

    void CreateSkeletalTreeNode();
    void RenderBoneHierarchy(int32 CurrentBoneIdx, const TArray<FBoneNode>& BoneNodes, const TMap<int32, TArray<int32>>& BoneHierarchy);

    SkeletalDetailPanel DetailPanel;
    int32 SelectedBoneIdx = 0;
private:
    float Width = 800, Height = 600;
};
