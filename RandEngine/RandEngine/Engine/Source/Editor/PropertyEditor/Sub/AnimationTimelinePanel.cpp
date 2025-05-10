#include "AnimationTimelinePanel.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_neo_sequencer.h" // im-neo-sequencer 헤더 포함
#include "Imgui/imgui_neo_internal.h" // 필요한 경우

// 정적 멤버 변수 초기화
#if !defined(FNAME_DEFINED) || !defined(MOCK_GLOBALS_DEFINED) // 이 매크로는 프로젝트 전체에서 한 번만 정의되도록 관리
#define MOCK_GLOBALS_DEFINED
int FMockAnimNotifyEvent::NextEventId = 0;
int FEditorTimelineTrack::NextTrackIdCounter = 1; // ID는 1부터 시작하도록 변경 (0 또는 -1은 특별한 의미로 사용 가능)
#endif

SAnimationTimelinePanel::SAnimationTimelinePanel()
    : MockAnimSequenceInstance(nullptr)
    , TargetSequence(nullptr)
    , bIsPlaying(false)
    , bIsLooping(false)
    , PlaybackSpeed(1.0f)
    , CurrentTimeSeconds(0.0f)
    , SelectedNotifyEventId(-1)
    , bIsDraggingNotify(false)
    , PanelWidth(800.f)  // 기본값
    , PanelHeight(600.f) // 기본값
{

    ImGuiNeoSequencerStyle& neoStyle = ImGui::GetNeoSequencerStyle();
    neoStyle.TopBarHeight = 35.0f;
    neoStyle.CurrentFramePointerSize = 10.f;
    // 어두운 배경 및 요소들
    neoStyle.Colors[ImGuiNeoSequencerCol_Bg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);         // 시퀀서 전체 배경
    neoStyle.Colors[ImGuiNeoSequencerCol_TopBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);      // 상단 바 (눈금자) 배경
    neoStyle.Colors[ImGuiNeoSequencerCol_TimelinesBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f); // 각 타임라인 행 배경 (TopBarBg와 유사하게)
    neoStyle.Colors[ImGuiNeoSequencerCol_TimelineBorder] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f); // 타임라인 간 경계선

    // 선택된 타임라인 (약간 밝지만 여전히 어두운 테마 유지)
    neoStyle.Colors[ImGuiNeoSequencerCol_SelectedTimeline] = ImVec4(0.3f, 0.4f, 0.6f, 0.7f); // 파란색 계열 강조

    // 플레이헤드 (눈에 잘 띄는 색상, 예: 밝은 빨강 또는 주황)
    neoStyle.Colors[ImGuiNeoSequencerCol_FramePointer] = ImVec4(0.9f, 0.3f, 0.2f, 0.8f);
    neoStyle.Colors[ImGuiNeoSequencerCol_FramePointerHovered] = ImVec4(1.0f, 0.4f, 0.3f, 1.0f);
    neoStyle.Colors[ImGuiNeoSequencerCol_FramePointerPressed] = ImVec4(1.0f, 0.2f, 0.1f, 1.0f);
    neoStyle.Colors[ImGuiNeoSequencerCol_FramePointerLine] = ImVec4(0.9f, 0.3f, 0.2f, 0.9f); // 플레이헤드 라인도 유사하게

    // 키프레임 (기본은 약간 어둡게, 선택/호버 시 밝게)
    neoStyle.Colors[ImGuiNeoSequencerCol_Keyframe] = ImVec4(0.7f, 0.7f, 0.7f, 1.f);
    neoStyle.Colors[ImGuiNeoSequencerCol_KeyframeHovered] = ImVec4(0.8f, 0.65f, 0.2f, 1.0f); // 호버 시 밝은 주황/노랑
    neoStyle.Colors[ImGuiNeoSequencerCol_KeyframeSelected] = ImVec4(0.5f, 0.2f, 0.2f, 1.0f); // 선택 시 밝은 파랑

    // 줌 바 (어두운 배경에 밝은 슬라이더)
    neoStyle.Colors[ImGuiNeoSequencerCol_ZoomBarBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.9f);
    neoStyle.Colors[ImGuiNeoSequencerCol_ZoomBarSlider] = ImVec4(0.4f, 0.4f, 0.4f, 0.7f);
    neoStyle.Colors[ImGuiNeoSequencerCol_ZoomBarSliderHovered] = ImVec4(0.5f, 0.5f, 0.5f, 0.9f);

    // 선택 영역 (반투명한 밝은 색)
    neoStyle.Colors[ImGuiNeoSequencerCol_SelectionBorder] = ImVec4(0.2f, 0.5f, 0.8f, 0.8f);
    neoStyle.Colors[ImGuiNeoSequencerCol_Selection] = ImVec4(0.2f, 0.5f, 0.8f, 0.3f);



    MockAnimSequenceInstance = new MockAnimSequence();
    MockAnimSequenceInstance->FrameRate = 30.0f;
    MockAnimSequenceInstance->SequenceLength = 5.0f; // 길이 약간 늘림
    MockAnimSequenceInstance->AddNotify(1.0f, FName("Footstep_L"));
    MockAnimSequenceInstance->AddNotify(1.6f, FName("Footstep_R"));
    MockAnimSequenceInstance->AddNotify(2.5f, FName("Jump"));
    SetTargetSequence(MockAnimSequenceInstance);
}

SAnimationTimelinePanel::~SAnimationTimelinePanel()
{
    if (MockAnimSequenceInstance != nullptr)
    {
        delete MockAnimSequenceInstance;
        MockAnimSequenceInstance = nullptr;
    }
    // TargetSequence는 MockAnimSequenceInstance를 가리키므로 별도 delete 필요 없음
}

void SAnimationTimelinePanel::SetTargetSequence(MockAnimSequence* Sequence)
{
    TargetSequence = Sequence;
    DisplayableTracks.Empty();
    CurrentTimeSeconds = 0.0f;
    bIsPlaying = false;
    bIsLooping = false;
    PlaybackSpeed = 1.0f;
    SelectedNotifyEventId = -1;

    if (TargetSequence != nullptr)
    {
        int rootTrackId = FEditorTimelineTrack::NextTrackIdCounter++;
        // ParentTrackId를 -1로 하여 최상위 루트임을 표시
        DisplayableTracks.Add(FEditorTimelineTrack(EEditorTimelineTrackType::AnimNotify_Root, "Notifies", rootTrackId, -1));

        // 예시: "Footsteps" 사용자 트랙 자동 추가 및 기존 노티파이 할당
        int footstepTrackId = FEditorTimelineTrack::NextTrackIdCounter++;
        DisplayableTracks.Add(FEditorTimelineTrack(EEditorTimelineTrackType::AnimNotify_UserTrack, "Footsteps", footstepTrackId, rootTrackId));
        for (FMockAnimNotifyEvent& notify : TargetSequence->Notifies)
        {
            if (notify.NotifyName == FName("Footstep_L") || notify.NotifyName == FName("Footstep_R"))
            {
                notify.UserInterfaceTrackId = footstepTrackId;
            }
        }
        // 예시: "Actions" 사용자 트랙 자동 추가 및 기존 노티파이 할당
        int actionTrackId = FEditorTimelineTrack::NextTrackIdCounter++;
        DisplayableTracks.Add(FEditorTimelineTrack(EEditorTimelineTrackType::AnimNotify_UserTrack, "Actions", actionTrackId, rootTrackId));
        for (FMockAnimNotifyEvent& notify : TargetSequence->Notifies)
        {
            if (notify.NotifyName == FName("Jump"))
            {
                notify.UserInterfaceTrackId = actionTrackId;
            }
        }
    }
}

float SAnimationTimelinePanel::GetSequenceDurationSeconds() const
{
    if (TargetSequence != nullptr)
    {
        return TargetSequence->SequenceLength;
    }
    return 0.0f;
}

float SAnimationTimelinePanel::GetSequenceFrameRate() const
{
    if (TargetSequence != nullptr)
    {
        return TargetSequence->FrameRate;
    }
    return 30.0f; // 기본값
}

int SAnimationTimelinePanel::GetSequenceTotalFrames() const
{
    if (TargetSequence == nullptr || GetSequenceDurationSeconds() <= 0.0f || GetSequenceFrameRate() <= 0.0f)
    {
        return 0;
    }
    return static_cast<int>(GetSequenceDurationSeconds() * GetSequenceFrameRate());
}

int SAnimationTimelinePanel::ConvertTimeToFrame() const
{
    if (GetSequenceFrameRate() <= 0.0f)
    {
        return 0;
    }
    return static_cast<int>(CurrentTimeSeconds * GetSequenceFrameRate());
}

void SAnimationTimelinePanel::ConvertFrameToTimeAndSet(int Frame)
{
    if (GetSequenceFrameRate() <= 0.0f)
    {
        CurrentTimeSeconds = 0.0f;
        return;
    }
    CurrentTimeSeconds = static_cast<float>(Frame) / GetSequenceFrameRate();
    CurrentTimeSeconds = std::max(0.0f, std::min(CurrentTimeSeconds, GetSequenceDurationSeconds()));
}

void SAnimationTimelinePanel::UpdatePlayback(float DeltaSeconds)
{
    if (!bIsPlaying || TargetSequence == nullptr || GetSequenceDurationSeconds() <= 0.0f)
    {
        return;
    }

    CurrentTimeSeconds += DeltaSeconds * PlaybackSpeed;

    if (CurrentTimeSeconds >= GetSequenceDurationSeconds())
    {
        if (bIsLooping)
        {
            CurrentTimeSeconds = fmodf(CurrentTimeSeconds, GetSequenceDurationSeconds());
            // TriggeredNotifyEventIdsThisPlayback.Empty(); // 루핑 시 트리거된 이벤트 초기화 (필요시)
        }
        else
        {
            CurrentTimeSeconds = GetSequenceDurationSeconds();
            bIsPlaying = false;
        }
    }
    else if (CurrentTimeSeconds < 0.0f)
    {
        if (bIsLooping)
        {
            CurrentTimeSeconds = GetSequenceDurationSeconds() - fmodf(-CurrentTimeSeconds, GetSequenceDurationSeconds());
            // TriggeredNotifyEventIdsThisPlayback.Empty(); // 루핑 시 트리거된 이벤트 초기화 (필요시)
        }
        else
        {
            CurrentTimeSeconds = 0.0f;
            bIsPlaying = false;
        }
    }
}

void SAnimationTimelinePanel::Render() // UEditorPanel 오버라이드
{
    UpdatePlayback(ImGui::GetIO().DeltaTime); // ImGui 델타 타임 사용
    RenderTimelineEditor();
}

void SAnimationTimelinePanel::OnResize(HWND hWnd) // HWND 의존성은 실제 환경에 맞게 수정
{
    // 이 부분은 실제 윈도우 시스템과 통합 방식에 따라 달라집니다.
    // ImGui 자체는 창 크기를 내부적으로 관리하므로,
    // 패널 크기를 ImGui 레이아웃 시스템에 맡기는 것이 일반적입니다.
    // RECT ClientRect;
    // if (GetClientRect(hWnd, &ClientRect))
    // {
    //     PanelWidth = static_cast<float>(ClientRect.right - ClientRect.left);
    //     PanelHeight = static_cast<float>(ClientRect.bottom - ClientRect.top);
    // }
}

void SAnimationTimelinePanel::RenderTimelineEditor()
{
    if (TargetSequence == nullptr)
    {
        ImGui::Text("No TargetSequence set.");
        return;
    }

    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    if (!mainViewport)
    {
        return;
    }

    const float timelinePanelHeight = 400.0f;

    ImVec2 windowPos = ImVec2(mainViewport->WorkPos.x, mainViewport->WorkPos.y + mainViewport->WorkSize.y - timelinePanelHeight);
    ImVec2 windowSize = ImVec2(mainViewport->WorkSize.x, timelinePanelHeight);
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("AnimationTimelineHost", nullptr, windowFlags))
    {

        RenderPlaybackControls();
        ImGui::Separator();
        RenderTrackManagementUI(); // 트랙 관리 UI 추가
        RenderNotifyPropertiesUI();
        ImGui::Separator();
        RenderSequencerWidget();   // 시퀀서 위젯


        ImGui::End();
    }
}

void SAnimationTimelinePanel::RenderPlaybackControls()
{
    if (TargetSequence == nullptr)
    {
        return;
    }

    const ImGuiIO& IO = ImGui::GetIO();
    ImFont* IconFont = IO.Fonts->Fonts.size() == 1 ? IO.FontDefault : IO.Fonts->Fonts[FEATHER_FONT];
    ImGui::PushFont(IconFont);
    if (ImGui::Button("\ue9a8", ImVec2(32,32))) // Play
    {
        bIsPlaying = !bIsPlaying;
        if (bIsPlaying && CurrentTimeSeconds >= GetSequenceDurationSeconds() - FLT_EPSILON && GetSequenceDurationSeconds() > 0.f)
        {
            CurrentTimeSeconds = 0.0f;
        }
    }
    ImGui::PopFont();
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        bIsPlaying = false;
        CurrentTimeSeconds = 0.0f;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Loop", &bIsLooping);
    ImGui::SameLine();
    ImGui::PushItemWidth(80);
    ImGui::DragFloat("Speed", &PlaybackSpeed, 0.01f, 0.01f, 10.0f, "%.2fx");
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Text("Time: %.2f / %.2f s (Frame: %d)", CurrentTimeSeconds, GetSequenceDurationSeconds(), ConvertTimeToFrame());

    // im-neo-sequencer는 자체 줌/패닝 기능이 있으므로, 이전 줌/프레임 슬라이더는 제거하거나
    // im-neo-sequencer의 내부 상태를 제어하는 방식으로 변경해야 합니다.
    // 여기서는 일단 제거하고, im-neo-sequencer의 기본 컨트롤에 의존합니다.
}


void SAnimationTimelinePanel::RenderTrackManagementUI()
{
    if (TargetSequence == nullptr)
    {
        return;
    }

    int rootTrackId = -1;
    for (const auto& track : DisplayableTracks)
    {
        if (track.TrackType == EEditorTimelineTrackType::AnimNotify_Root)
        {
            rootTrackId = track.TrackId;
            break;
        }
    }

    if (rootTrackId != -1)
    {
        static char newTrackNameInput[128] = "New User Track";
        ImGui::InputText("New Track Name", newTrackNameInput, IM_ARRAYSIZE(newTrackNameInput));
        ImGui::SameLine();
        if (ImGui::Button("Add User Track"))
        {
            if (strlen(newTrackNameInput) > 0)
            {
                AddUserNotifyTrack(rootTrackId, newTrackNameInput);
                newTrackNameInput[0] = '\0'; // 입력 필드 초기화
            }
        }
    }
}
void SAnimationTimelinePanel::RenderNotifyPropertiesUI()
{
    ImGui::Text("Notify Properties");

    if (TargetSequence == nullptr) // TargetSequence는 항상 있어야 함
    {
        ImGui::TextDisabled("No sequence loaded.");
        return;
    }

    // "Add Notify At Current Time" 버튼 위치 이동 (예시)
    if (ImGui::Button("Add Notify At Current Time"))
    {
        int targetTrackIdForNewNotify = -1;
        if (LastSelectedUserTrackId != -1) // 멤버 변수로 관리
        {
            // 유효성 검사
            bool bIsValidTrack = false;
            for (const auto& track : DisplayableTracks)
            {
                if (track.TrackId == LastSelectedUserTrackId && track.TrackType == EEditorTimelineTrackType::AnimNotify_UserTrack)
                {
                    bIsValidTrack = true;
                    break;
                }
            }
            if (bIsValidTrack) targetTrackIdForNewNotify = LastSelectedUserTrackId;
        }
        TargetSequence->AddNotify(CurrentTimeSeconds, FName("NewEvent"), targetTrackIdForNewNotify);
        TargetSequence->SortNotifies();
    }
    ImGui::Spacing();


    if (SelectedNotifyEventId != -1)
    {
        FMockAnimNotifyEvent* currentSelectedEvent = nullptr;
        for (auto& notify : TargetSequence->Notifies)
        {
            if (notify.EventId == SelectedNotifyEventId)
            {
                currentSelectedEvent = &notify;
                break;
            }
        }

        if (currentSelectedEvent != nullptr)
        {
            ImGui::Text("Name: %s ID: &d", currentSelectedEvent->NotifyName.ToString().ToAnsiString().c_str(), currentSelectedEvent->EventId);

            float triggerTime = currentSelectedEvent->TriggerTime;
            ImGui::PushItemWidth(100); // 시간 입력 필드 너비
            if (ImGui::DragFloat("Time (s)", &triggerTime, 0.01f, 0.0f, GetSequenceDurationSeconds(), "%.2f"))
            {
                if (TargetSequence->FrameRate > 0)
                {
                    int frame = static_cast<int>(round(triggerTime * TargetSequence->FrameRate));
                    currentSelectedEvent->TriggerTime = static_cast<float>(frame) / TargetSequence->FrameRate;
                }
                else
                {
                    currentSelectedEvent->TriggerTime = triggerTime;
                }
                bIsDraggingNotify = true; // 정렬 트리거 (드래그 완료 시 SortNotifies 호출됨)
            }
            ImGui::PopItemWidth();

            ImGui::Text("Assign to Track:");
            std::string currentAssignedTrackName = "None (Default)";
            int currentAssignedUiTrackId = currentSelectedEvent->UserInterfaceTrackId;

            for (const auto& track : DisplayableTracks)
            {
                if (track.TrackType == EEditorTimelineTrackType::AnimNotify_UserTrack && track.TrackId == currentAssignedUiTrackId)
                {
                    currentAssignedTrackName = track.DisplayName;
                    break;
                }
            }

            ImGui::PushItemWidth(200);
            if (ImGui::BeginCombo("##TrackAssignCombo", currentAssignedTrackName.c_str()))
            {
                if (ImGui::Selectable("None (Default)", currentAssignedUiTrackId <= 0))
                {
                    currentSelectedEvent->UserInterfaceTrackId = -1;
                }
                for (const auto& track : DisplayableTracks)
                {
                    if (track.TrackType == EEditorTimelineTrackType::AnimNotify_UserTrack)
                    {
                        if (ImGui::Selectable(track.DisplayName.c_str(), currentAssignedUiTrackId == track.TrackId))
                        {
                            currentSelectedEvent->UserInterfaceTrackId = track.TrackId;
                        }
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Selected notify (ID: %d) not found in sequence.", SelectedNotifyEventId);
        }
    }
    else
    {
        ImGui::TextDisabled("No notify selected.");
        ImGui::Dummy(ImVec2(0, 53)); // 공간
        ImGui::TextWrapped("Click on a notify in the timeline or use 'Add Notify At Current Time' to create one.");
    }
}void SAnimationTimelinePanel::AddUserNotifyTrack(int ParentRootTrackId, const std::string& NewTrackName)
{
    if (TargetSequence == nullptr)
    {
        return;
    }

    FEditorTimelineTrack* parentRootTrack = nullptr;
    int parentRootTrackIndex = -1;
    for (int i = 0; i < DisplayableTracks.Num(); ++i)
    {
        if (DisplayableTracks[i].TrackId == ParentRootTrackId &&
            DisplayableTracks[i].TrackType == EEditorTimelineTrackType::AnimNotify_Root)
        {
            parentRootTrack = &DisplayableTracks[i];
            parentRootTrackIndex = i;
            break;
        }
    }

    if (parentRootTrack != nullptr)
    {
        int newTrackId = FEditorTimelineTrack::NextTrackIdCounter++;
        int insertAtIndex = parentRootTrackIndex + 1;
        for (int i = parentRootTrackIndex + 1; i < DisplayableTracks.Num(); ++i)
        {
            if (DisplayableTracks[i].ParentTrackId == ParentRootTrackId)
            {
                insertAtIndex = i + 1;
            }
            else
            {
                break;
            }
        }

        FEditorTimelineTrack newUserTrack(EEditorTimelineTrackType::AnimNotify_UserTrack, NewTrackName, newTrackId, ParentRootTrackId);

        if (insertAtIndex >= DisplayableTracks.Num())
        {
            DisplayableTracks.Add(newUserTrack);
        }
        else
        {
            DisplayableTracks.Insert(newUserTrack, insertAtIndex);
        }

        parentRootTrack->bIsExpanded = true;
    }
}


void SAnimationTimelinePanel::RenderTracksRecursive(int ParentId)
{
    for (int i = 0; i < DisplayableTracks.Num(); ++i)
    {
        FEditorTimelineTrack& uiTrack = DisplayableTracks[i]; // bIsExpanded 수정을 위해 non-const

        if (uiTrack.ParentTrackId == ParentId)
        {
            bool isGroup = (uiTrack.TrackType == EEditorTimelineTrackType::AnimNotify_Root);
            ImGuiNeoTimelineFlags flags = isGroup ? ImGuiNeoTimelineFlags_Group : ImGuiNeoTimelineFlags_None;
            bool* openStatePtr = isGroup ? &uiTrack.bIsExpanded : nullptr;

            if (ImGui::BeginNeoTimelineEx(uiTrack.DisplayName.c_str(), openStatePtr, flags))
            {
                if (!isGroup && ImGui::IsNeoTimelineSelected()) // 사용자 트랙이고, 현재 선택된 타임라인이라면
                {
                    LastSelectedUserTrackId = uiTrack.TrackId; // 선택된 트랙 ID 저장
                }

                if (isGroup) // 그룹이면 (그리고 열려있다면 BeginNeoTimelineEx가 true 반환)
                {
                    // 현재 uiTrack.bIsExpanded는 BeginNeoTimelineEx에 의해 UI 상태가 반영됨
                    if (uiTrack.bIsExpanded)
                    {
                        RenderTracksRecursive(uiTrack.TrackId); // 자식들 렌더링
                    }
                }
                else // 일반 사용자 트랙이면 키프레임 렌더링
                {
                    for (int notifyIdx = 0; notifyIdx < TargetSequence->Notifies.Num(); ++notifyIdx)
                    {
                        FMockAnimNotifyEvent& notifyEvent = TargetSequence->Notifies[notifyIdx];
                        if (notifyEvent.UserInterfaceTrackId == uiTrack.TrackId)
                        {
                            // int32_t는 FrameIndexType의 기본 타입일 가능성이 높음
                            int32_t keyframeTime = static_cast<int32_t>(notifyEvent.TriggerTime * GetSequenceFrameRate());

                            ImGui::PushID(notifyEvent.EventId);
                            ImGui::NeoKeyframe(&keyframeTime);

                            float newTimeBasedOnFrame = static_cast<float>(keyframeTime) / GetSequenceFrameRate();
                            if (fabs(newTimeBasedOnFrame - notifyEvent.TriggerTime) > 1e-5f) // 부동소수점 비교 수정
                            {
                                notifyEvent.TriggerTime = std::max(0.0f, std::min(newTimeBasedOnFrame, GetSequenceDurationSeconds()));
                                bIsDraggingNotify = true; // 드래그 완료 후 정렬을 위해 플래그 설정
                            }

                            if (ImGui::IsNeoKeyframeSelected())
                            {
                                SelectedNotifyEventId = notifyEvent.EventId;
                                LastSelectedUserTrackId = uiTrack.TrackId;
                            }
                            if (ImGui::IsNeoKeyframeHovered())
                            {
                                ImGui::SetTooltip("Notify: %s\nTime: %.2fs (Frame: %d)",
                                    notifyEvent.NotifyName.ToString().ToAnsiString().c_str(),
                                    notifyEvent.TriggerTime,
                                    static_cast<int>(notifyEvent.TriggerTime * GetSequenceFrameRate()));
                            }
                            if (ImGui::IsNeoKeyframeRightClicked())
                            {
                                SelectedNotifyEventId = notifyEvent.EventId;
                                // TODO: ImGui::OpenPopup("NotifyContextMenu");
                            }
                            ImGui::PopID();
                        }
                    }
                    // 현재 타임라인(사용자 트랙)이 선택되었는지 확인
                    if (ImGui::IsNeoTimelineSelected()) {
                        // TODO: 선택된 트랙에 대한 처리 (예: 컨텍스트 메뉴)
                        // SequencerSelectedEntry = uiTrack.TrackId; // 예시
                    }
                }
                ImGui::EndNeoTimeLine(); // BeginNeoTimelineEx가 true를 반환했으므로 호출
            }
        }
    }
}


void SAnimationTimelinePanel::RenderSequencerWidget()
{
    if (TargetSequence == nullptr)
    {
        return;
    }

    ImGui::FrameIndexType currentFrame = static_cast<ImGui::FrameIndexType>(ConvertTimeToFrame());
    ImGui::FrameIndexType startFrame = 0;
    ImGui::FrameIndexType endFrame = static_cast<ImGui::FrameIndexType>(GetSequenceTotalFrames());

    if (endFrame <= startFrame)
    {
        endFrame = startFrame + 100; // 시퀀스가 비었을 때 기본 길이 (예: 100 프레임)
        if (TargetSequence != nullptr && TargetSequence->FrameRate > 0)
        {
            // TargetSequence->SequenceLength = static_cast<float>(endFrame) / TargetSequence->FrameRate; // 길이도 업데이트
        }
    }


    ImGuiNeoSequencerFlags sequencerFlags = ImGuiNeoSequencerFlags_EnableSelection |
        ImGuiNeoSequencerFlags_Selection_EnableDragging |
        ImGuiNeoSequencerFlags_Selection_EnableDeletion |
        ImGuiNeoSequencerFlags_AllowLengthChanging; // 길이 변경 허용

    // ImVec2(0,0)은 사용 가능한 전체 공간을 사용하려고 시도합니다.
    // 높이를 명시적으로 지정하려면 ImVec2(0, 원하는_높이)
    if (ImGui::BeginNeoSequencer("MainSequencer", &currentFrame, &startFrame, &endFrame, ImVec2(0, 200.f), sequencerFlags))
    {
        if (currentFrame != static_cast<ImGui::FrameIndexType>(ConvertTimeToFrame()))
        {
            ConvertFrameToTimeAndSet(static_cast<int>(currentFrame));
            if (bIsPlaying)
            {
                bIsPlaying = false;
            }
            // TriggeredNotifyEventIdsThisPlayback.Empty(); // 필요시
        }

        // 시퀀스 길이 변경 처리 (AllowLengthChanging 플래그 사용 시)
        // uint32_t currentTotalFrames = static_cast<uint32_t>(GetSequenceTotalFrames());
        // if (startFrame != 0 || endFrame != currentTotalFrames) // 시작 프레임은 0으로 고정 가정
        if (endFrame != static_cast<ImGui::FrameIndexType>(GetSequenceTotalFrames()) && TargetSequence->FrameRate > 0.0f)
        {
            TargetSequence->SequenceLength = static_cast<float>(endFrame) / TargetSequence->FrameRate;
        }


        // 최상위 루트 트랙들 (ParentTrackId가 -1인 트랙들)부터 재귀적으로 렌더링
        RenderTracksRecursive(-1);


        // 삭제 로직 (Begin/End NeoSequencer 내부)
        // Delete 키는 ImGui::IsKeyPressed를 사용 (포커스된 창에서만 감지)
        // 또는 GetIO().KeysDown[]을 직접 확인 (전역적이지만, 다른 입력과 충돌 가능성)
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        {
            if (ImGui::NeoCanDeleteSelection()) // 선택된 "키프레임"이 있는지 확인
            {
                if (SelectedNotifyEventId != -1) // 우리가 관리하는 선택된 "노티파이" ID
                {
                    TargetSequence->RemoveNotifyByEventId(SelectedNotifyEventId);
                    SelectedNotifyEventId = -1;
                    ImGui::NeoClearSelection(); // 시퀀서 라이브러리의 선택 상태도 클리어
                }
            }
        }

        // 빈 공간 클릭 시 선택 해제
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && !ImGui::IsAnyItemHovered() && !ImGui::IsItemActive())
        {
            if (ImGui::NeoHasSelection()) // 라이브러리에 선택된게 있을때만
            {
                ImGui::NeoClearSelection();
            }
            SelectedNotifyEventId = -1;
        }

        // 드래그 완료 후 노티파이 정렬
        if (bIsDraggingNotify && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            TargetSequence->SortNotifies();
            bIsDraggingNotify = false;
        }

        ImGui::EndNeoSequencer();
    }
}

// --- SAnimationTimelinePanel의 나머지 함수들 (AdjustFirstFrameToKeepCenter, ClampFirstVisibleFrame, CenterViewOnFrame 등) ---
// --- 이 함수들은 im-neo-sequencer가 자체 줌/패닝을 처리하므로 필요 없거나, ---
// --- im-neo-sequencer의 API를 통해 줌/오프셋을 제어하는 방식으로 변경되어야 합니다. ---
// --- 지금은 일단 주석 처리하거나 삭제합니다. ---
/*
void SAnimationTimelinePanel::FitTimelineToView() { ... }
void SAnimationTimelinePanel::AdjustFirstFrameToKeepCenter() { ... }
void SAnimationTimelinePanel::ClampFirstVisibleFrame() { ... }
void SAnimationTimelinePanel::CenterViewOnFrame(int targetFrame) { ... }
*/
