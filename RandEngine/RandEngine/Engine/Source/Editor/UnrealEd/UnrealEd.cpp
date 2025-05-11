#include "UnrealEd.h"
#include "EditorPanel.h"

#include "PropertyEditor/ControlEditorPanel.h"
#include "PropertyEditor/OutlinerEditorPanel.h"
#include "PropertyEditor/PropertyEditorPanel.h"
#include "PropertyEditor/Sub/SkeletalViewerPanel.h"
#include "PropertyEditor/Sub/AnimationTimelinePanel.h"

void UnrealEd::Initialize()
{
    auto ControlPanel = std::make_shared<ControlEditorPanel>();
    AddEditorPanel("ControlPanel", ControlPanel);

    auto OutlinerPanel = std::make_shared<OutlinerEditorPanel>();
    AddEditorPanel("OutlinerPanel", OutlinerPanel);

    auto PropertyPanel = std::make_shared<PropertyEditorPanel>();
    AddEditorPanel("PropertyPanel", PropertyPanel);

    auto SubSkeletalViewerPanel = std::make_shared<SkeletalViewerPanel>();
    AddEditorPanel("SubSkeletalViewerPanel", SubSkeletalViewerPanel, EWindowType::WT_SkeletalSubWindow);

    auto SubAnimationViewerPanel = std::make_shared<SAnimationTimelinePanel>();
    auto SubSkeletalViewerPanel2 = std::make_shared<SkeletalViewerPanel>();
    AddEditorPanel("SubSkeletalViewerPanel", SubSkeletalViewerPanel2, EWindowType::WT_AnimationSubWindow);
    AddEditorPanel("SubAnimationViewerPanel", SubAnimationViewerPanel, EWindowType::WT_AnimationSubWindow);
 
}

void UnrealEd::Render(EWindowType WindowType) const
{
    TMap<FString, std::shared_ptr<UEditorPanel>> TargetPanels;

    switch (WindowType)
    {
    case EWindowType::WT_Main:
        TargetPanels = Panels;
        break;
    case EWindowType::WT_SkeletalSubWindow:
        TargetPanels = SkeletalSubPanels;
        break;
    case EWindowType::WT_AnimationSubWindow:
        TargetPanels = AnimationSubPanels;
        break;
    }

    for (const auto& Panel : TargetPanels)
    {
        Panel.Value->Render();
    }
}

void UnrealEd::AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel, EWindowType WindowType)
{
    switch (WindowType)
    {
    case EWindowType::WT_Main:
        Panels[PanelId] = EditorPanel;
        break;
    case EWindowType::WT_SkeletalSubWindow:
        SkeletalSubPanels[PanelId] = EditorPanel;
        break;
    case EWindowType::WT_AnimationSubWindow:
        AnimationSubPanels[PanelId] = EditorPanel;
        break;
    default:
        break;
    }

}

void UnrealEd::OnResize(HWND hWnd, EWindowType WindowType) const
{
    
    TMap<FString, std::shared_ptr<UEditorPanel>> TargetPanels;

    switch (WindowType)
    {
    case EWindowType::WT_Main:
        TargetPanels = Panels;
        break;
    case EWindowType::WT_SkeletalSubWindow:
        TargetPanels = SkeletalSubPanels;
        break;
    case EWindowType::WT_AnimationSubWindow:
        TargetPanels = AnimationSubPanels;
        break;
    }

    for (auto& PanelPair : TargetPanels)
    {
        if (PanelPair.Value)
        {
            PanelPair.Value->OnResize(hWnd);
        }
    }
}

std::shared_ptr<UEditorPanel> UnrealEd::GetEditorPanel(const FString& PanelId)
{
    return Panels[PanelId];
}

std::shared_ptr<UEditorPanel> UnrealEd::GetSubEditorPanel(const FString& PanelId)
{
    return SkeletalSubPanels[PanelId];
}
