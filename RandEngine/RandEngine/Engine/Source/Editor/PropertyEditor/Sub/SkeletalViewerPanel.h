#pragma once
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
    USkeleton* Skeleton;

    void CreateSkeletalTreeNode();
    void RenderBoneHierarchy(int32 CurrentBoneIdx, const TArray<FBoneNode>& BoneNodes, const TMap<int32, TArray<int32>>& BoneHierarchy);

private:
    float Width = 800, Height = 600;
};
