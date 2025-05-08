#pragma once
#include "Components/ActorComponent.h"
#include "UnrealEd/EditorPanel.h"

struct FSkeletalHierarchyData;

class SkeletalViewerPanel : public UEditorPanel
{
public:
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

private:
    void CreateSkeletalTreeNode();

    void RenderSkeletalTreeNode(const FSkeletalHierarchyData& Node);
    
private:
    float Width = 800, Height = 600;
};
