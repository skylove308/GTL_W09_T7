#pragma once
#include "Container/Map.h"
#include "Container/String.h"
#include "EditorPanel.h"

enum EWindowType : uint8;
class UEditorPanel;

class UnrealEd
{
public:
    UnrealEd() = default;
    ~UnrealEd() = default;
    void Initialize();
    
    void Render(EWindowType WindowType = EWindowType::WT_Main) const;
    void OnResize(HWND hWnd, EWindowType WindowType = EWindowType::WT_Main) const;
    
    void AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel, EWindowType WindowType= EWindowType::WT_Main);
    std::shared_ptr<UEditorPanel> GetEditorPanel(const FString& PanelId);
    std::shared_ptr<UEditorPanel> GetSubEditorPanel(const FString& PanelId);

private:
    TMap<FString, std::shared_ptr<UEditorPanel>> Panels;
    TMap<FString, std::shared_ptr<UEditorPanel>> SkeletalSubPanels;
    TMap<FString, std::shared_ptr<UEditorPanel>> AnimationSubPanels;
};
