#pragma once

#include "ImGui/imgui_neo_sequencer.h" // im-neo-sequencer 헤더
#include "Container/Array.h"
#include "Container/Set.h"
#include "UObject/NameTypes.h"
#include "UnrealEd/EditorPanel.h" // UEditorPanel 상속 가정
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/AnimDataModel.h" 


#include "Imgui/imgui.h"
#include "Imgui/imgui_internal.h" // 필요한 경우 (하지만 직접 사용은 최소화)

// 전방 선언
class UAnimSequence; // 실제 UAnimSequence를 사용한다면

// --- 모의(Mock) 애니메이션 데이터 구조체 ---
struct FMockAnimNotifyEvent
{
    int EventId;
    float TriggerTime = 0.0f;
    FName NotifyName;
    // FString NotifyDisplayName; // NotifyName.ToString()으로 동적 생성 가능
    int UserInterfaceTrackId; // 이 노티파이가 속한 FEditorTimelineTrack의 TrackId

    FMockAnimNotifyEvent(float InTime, FName InName, int InUITrackId = -1) // 기본값 -1 (할당 안됨)
        : TriggerTime(InTime), NotifyName(InName), UserInterfaceTrackId(InUITrackId)
    {
        EventId = NextEventId++;
    }

    static int NextEventId;
};

class MockAnimSequence
{
public:
    TArray<FMockAnimNotifyEvent> Notifies;
    float SequenceLength = 10.0f;
    float FrameRate = 30.0f;

    MockAnimSequence() {}

    void AddNotify(float TriggerTime, const FName& Name, int AssignedTrackId = -1) // 트랙 ID 인자 추가
    {
        Notifies.Add(FMockAnimNotifyEvent(TriggerTime, Name, AssignedTrackId));
        // 정렬은 필요시 외부에서 수행하거나, 추가 직후가 아닌 다른 시점에 수행
    }

    void RemoveNotifyByEventId(int EventId)
    {
        Notifies.RemoveAll([EventId](const FMockAnimNotifyEvent& Event)
            {
                return Event.EventId == EventId;
            });
    }

    void SortNotifies()
    {
        Notifies.Sort([](const FMockAnimNotifyEvent& A, const FMockAnimNotifyEvent& B)
            {
                return A.TriggerTime < B.TriggerTime;
            });
    }
};

// --- Editor Specific Data Structures (ImGui 연동용) ---
enum class EEditorTimelineTrackType
{
    AnimNotify_Root,     // "Notifies" 같은 최상위 그룹
    AnimNotify_UserTrack // 사용자가 추가한 하위 트랙
};

struct FEditorTimelineTrack
{
    EEditorTimelineTrackType TrackType;
    std::string DisplayName; // UTF-8 문자열 사용 권장 (ImGui는 UTF-8 기본)
    int TrackId;             // 이 트랙의 고유 ID
    int ParentTrackId;       // 부모 트랙의 ID (루트면 -1 또는 자기 자신 ID)
    bool bIsExpanded;        // 자신이 그룹일 경우 확장 상태
    // int IndentLevel;      // im-neo-sequencer가 Begin/EndTimelineEx 중첩으로 자동 처리

    FEditorTimelineTrack(EEditorTimelineTrackType InTrackType, const std::string& InDisplayName, int InTrackId, int InParentTrackId = -1)
        : TrackType(InTrackType), DisplayName(InDisplayName), TrackId(InTrackId),
        ParentTrackId(InParentTrackId), bIsExpanded(true) // 기본적으로 확장
    {
        if (NextTrackIdCounter <= InTrackId)
        {
            NextTrackIdCounter = InTrackId + 1;
        }
    }

    static int NextTrackIdCounter;
};


class SAnimationTimelinePanel : public UEditorPanel
{
public:
    SAnimationTimelinePanel();
    virtual ~SAnimationTimelinePanel(); // 가상 소멸자 권장

    // --- Public Interface Methods ---
    void SetTargetSequence(MockAnimSequence* Sequence);

    // 데이터 접근 및 변환 함수
    float GetSequenceDurationSeconds() const;
    float GetSequenceFrameRate() const;
    int GetSequenceTotalFrames() const;
    int ConvertTimeToFrame() const;
    void ConvertFrameToTimeAndSet(int Frame);

    // 메인 UI 렌더링 함수 (UEditorPanel 오버라이드)
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override; // Windows 핸들 의존성 제거 고려

private:
    // --- Private Helper Methods for UI Rendering ---
    void RenderTimelineEditor();      // 전체 타임라인 UI를 그리는 메인 함수
    void RenderPlaybackControls();    // 재생 관련 컨트롤 UI
    void RenderSequencerWidget();     // im-neo-sequencer를 사용하여 실제 시퀀서 부분을 그리는 함수
    void RenderTrackManagementUI();   // 트랙 추가/삭제, 노티파이 할당 등의 UI
    void RenderNotifyPropertiesUI(); // 선택된 노티파이의 속성 UI

    // 트랙 및 노티파이 관리 함수
    void UpdatePlayback(float DeltaSeconds);
    void AddUserNotifyTrack(int ParentRootTrackId, const std::string& NewTrackName);
    void RemoveUserNotifyTrack(int TrackIdToRemove); // TODO: 구현 필요
    void AssignNotifyToTrack(int NotifyEventId, int TargetTrackId); // TODO: 구현 필요

    // 계층적 트랙 렌더링 헬퍼
    void RenderTracksRecursive(int ParentId);


    // --- Member Variables ---
    MockAnimSequence* MockAnimSequenceInstance; // 생성자에서 할당, 소멸자에서 해제
    MockAnimSequence* TargetSequence;           // 현재 편집 대상 시퀀스

    TArray<FEditorTimelineTrack> DisplayableTracks; // UI에 표시될 트랙 정보

    // Playback state
    bool bIsPlaying;
    bool bIsLooping;
    float PlaybackSpeed;
    float CurrentTimeSeconds;

    // UI Interaction State
    int SelectedNotifyEventId; // 현재 선택된 노티파이의 EventId
    // int SequencerSelectedTimelineId; // im-neo-sequencer의 선택된 타임라인 ID (필요시)
    bool bIsDraggingNotify; // im-neo-sequencer의 드래그 기능을 사용하므로, 별도 플래그 필요성 재검토

    int LastSelectedUserTrackId = -1;
    // Windows 핸들 의존성 제거를 위해 Width, Height는 다른 방식으로 관리 고려
    // (예: ImGui::GetContentRegionAvail())
    float Width;
    float Height;
};
