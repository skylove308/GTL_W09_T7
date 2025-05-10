#pragma once

#include "ImGui/ImSequencer.h"
#include "Container/Array.h"
#include "Container/Set.h"
#include "UObject/NameTypes.h"
#include "UnrealEd/EditorPanel.h"

#include "Imgui/imgui.h"
#include "Imgui/imgui_internal.h"

class FEditorTimelineTrack;
class FAnimNotifyEvent;
class UAnimSequence;



// --- 모의(Mock) 애니메이션 데이터 구조체 --- ----------------
// 
// ToDo: 실제 애니메이션 애니메이션 구조체로 변경해야함
//
// ---------------------------------------------------------
struct FMockAnimNotifyEvent
{
    int EventId;
    float TriggerTime = 0.0f;
    FName NotifyName;
    FString NotifyDisplayName;
    int UserInterfaceTrackId;

    FMockAnimNotifyEvent(float InTime, FName InName, int InUITrackId)
        : TriggerTime(InTime), NotifyName(InName), UserInterfaceTrackId(InUITrackId)
    {
        EventId = NextEventId++;
        NotifyDisplayName = NotifyName.ToString().ToAnsiString(); // 기본 표시 이름
    }
 
    FMockAnimNotifyEvent(float InTime, FName InName) // 이 생성자는 특정 UI 트랙 ID가 없을 경우 사용
        : TriggerTime(InTime), NotifyName(InName), UserInterfaceTrackId(-1) // -1은 특정 UI 트랙에 할당되지 않음을 의미 (예: 기본 Notifies 트랙)
    {
        EventId = NextEventId++;
        NotifyDisplayName = NotifyName.ToString().ToAnsiString();
    }
    static int NextEventId;
};


class MockAnimSequence
{
public:
    TArray<FMockAnimNotifyEvent> Notifies;
    float SequenceLength = 10.0f; // 기본 길이 10초
    float FrameRate = 30.0f;      // 기본 프레임률 30 FPS

    MockAnimSequence() {}

    void AddNotify(float triggerTime, const FName& name) {
        Notifies.Add(FMockAnimNotifyEvent(triggerTime, name));
        Notifies.Sort([](const FMockAnimNotifyEvent& A, const FMockAnimNotifyEvent& B) {
            return A.TriggerTime < B.TriggerTime;
            });
    }
    void RemoveNotifyByEventId(int eventId) {
        Notifies.RemoveAll([eventId](const FMockAnimNotifyEvent& Event) {
            return Event.EventId == eventId;
            });
    }
};

// --- Editor Specific Data Structures (ImGui 연동용) ---
enum class EEditorTimelineTrackType
{
    AnimNotify_Root,
    AnimNotify_Item
};

struct FEditorTimelineTrack
{
    EEditorTimelineTrackType TrackType;
    std::string DisplayName;
    int TrackId;
    int Depth;
    bool bIsExpanded;

    FEditorTimelineTrack(EEditorTimelineTrackType InType, const std::string& InBaseName, int InDepth = 0)
        : TrackType(InType), Depth(InDepth), bIsExpanded(true)
    {
        TrackId = NextTrackIdCounter++;
        std::string prefix = "";
        for (int i = 0; i < Depth; ++i)
        {
            prefix += "  ";
        }
        DisplayName = prefix + InBaseName;
    }
    static int NextTrackIdCounter;
};


class SAnimationTimelinePanel : public ImSequencer::SequenceInterface, public UEditorPanel
{
    MockAnimSequence* MocdkAnimSequence; //for Text
public:
    SAnimationTimelinePanel();
    ~SAnimationTimelinePanel()
    {
        if (MocdkAnimSequence)
        {
            delete MocdkAnimSequence;
            MocdkAnimSequence = nullptr;
        }
    }
    // --- Public Interface Methods ---
    void SetTargetSequence(MockAnimSequence* sequence); 

    // 데이터 접근 및 변환 함수
    float GetSequenceDurationSeconds() const;
    float GetSequenceFrameRate() const;
    int GetSequenceTotalFrames() const;
    int ConvertTimeToFrame() const;
    void ConvertFrameToTimeAndSet(int frame);

    // 재생 로직 업데이트
    void UpdatePlayback(float DeltaSeconds);

    // 메인 UI 렌더링 함수
    void RenderTimelineEditor();


    // --- ImSequencer::SequenceInterface Implementation (가상 함수들) ---
    virtual int GetFrameMin() const override;
    virtual int GetFrameMax() const override;
    virtual int GetItemCount() const override;
    virtual const char* GetItemLabel(int index) const override;
    virtual void Get(int index, int** start, int** end, int* type, unsigned int* color) override;

    // 아래 함수들은 현재 구현 범위에서는 간소화
    virtual void Add(int typeIndex) override {}
    virtual void Del(int index) override {}
    virtual void Duplicate(int index) override {}
    virtual size_t GetCustomHeight(int index) override { return 0; }
    virtual void DoubleClick(int index) override {}

    // 노티파이 UI 그리기 (ImSequencer 인터페이스)
    virtual void CustomDraw(int displayTrackIndex, 
                            ImDrawList* drawList, const ImRect& rc, 
                            const ImRect& legendRect, const ImRect& clippingRect,
                            const ImRect& legendClippingRect) override;

    virtual void CustomDrawCompact(int displayTrackIndex, ImDrawList* drawList, const ImRect& rc, const ImRect& clippingRect) override;
    
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;
private:
    // --- Private Helper Methods for UI Rendering ---
    void FitTimelineToView();
    void RenderPlaybackControls();
    void RenderViewControls();
    void CenterViewOnFrame(int targetFrame);
    void AdjustFirstFrameToKeepCenter();
    void ClampFirstVisibleFrame();
    void RenderSequencerWidget();
    void RenderNotifyEditor();

    // 노티파이 그리기 헬퍼 함수
    void RenderNotifyTrackItems(const TArray<FMockAnimNotifyEvent>& notifies, ImDrawList* drawList, const ImRect& rc, const ImRect& clippingRect, bool isCompact); 

public:
    MockAnimSequence* TargetSequence; // 편집 대상이 되는 모의 시퀀스 객체

    TArray<FEditorTimelineTrack> DisplayableTracks;
    float FramePixelWidth = 10.0f;
    float FramePixelWidth_BeforeChange;
    // Playback state
    bool IsPlaying = false;
    bool IsLooping = false;
    float PlaybackSpeed = 1.0f;
    float CurrentTimeSeconds = 0.0f;

    TSet<int> TriggeredNotifyEventIdsThisPlayback;

    // ImSequencer state
    int SequencerSelectedEntry = -1; // 현재는 항상 0 (Notify 트랙만 있음)
    int SequencerFirstVisibleFrame = 0;
    int SelectedNotifyEventId = -1; // UI에서 선택된 노티파이의 EventId
    bool IsSequencerExpanded = false;

    // 드래그 상태
    bool bIsDraggingSelectedNotify = false;
    float DraggingNotifyOriginalTime = 0.0f;
    ImVec2 DraggingStartMousePosInTrack;

    float LastSequencerAreaWidth = 400.f;

    float Width = 800, Height = 600;
};
