#pragma once
#include "Components/ActorComponent.h"
#include "UnrealEd/EditorPanel.h"

class SkeletalViewerPanel : public UEditorPanel
{
public:
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

private:
    void CreateSkeletalTreeNode();

private:
    float Width = 800, Height = 600;
};
