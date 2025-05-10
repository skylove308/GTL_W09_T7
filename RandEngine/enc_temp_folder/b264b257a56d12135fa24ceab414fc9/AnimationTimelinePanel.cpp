#include "AnimationTimelinePanel.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_neo_internal.h"
#include "Container/Map.h"

// ... (Static Member Initializations) ...
#if !defined(FNAME_DEFINED) || !defined(MOCK_GLOBALS_DEFINED)
#define MOCK_GLOBALS_DEFINED
int FMockAnimNotifyEvent::NextEventId = 0;
int FEditorTimelineTrack::NextTrackIdCounter = 0;
#endif

SAnimationTimelinePanel::SAnimationTimelinePanel()
    : TargetSequence(nullptr)
    , IsSequencerExpanded(true)
    , IsPlaying(false)
    , IsLooping(false)
    , PlaybackSpeed(1.0f)
    , CurrentTimeSeconds(0.0f)
    , SequencerSelectedEntry(-1)
    , SequencerFirstVisibleFrame(0)
    , SelectedNotifyEventId(-1)
    , bIsDraggingSelectedNotify(false)
    , DraggingNotifyOriginalTime(0.0f)
    , FramePixelWidth(6.0f) // 적절한 초기값 설정
    , FramePixelWidth_BeforeChange(6.0f) // FramePixelWidth와 동일하게 초기
{

    MocdkAnimSequence = new MockAnimSequence();

  
    MocdkAnimSequence->FrameRate = 30.0f;
    MocdkAnimSequence->SequenceLength = 2.0f;
    MocdkAnimSequence->AddNotify(1.0f, FName("Footstep_L"));
    MocdkAnimSequence->AddNotify(1.6f, FName("Footstep_R"));
    SetTargetSequence(MocdkAnimSequence);
}
void SAnimationTimelinePanel::SetTargetSequence(MockAnimSequence* sequence)
{
    TargetSequence = sequence;
    DisplayableTracks.Empty();
    CurrentTimeSeconds = 0.0f;
    IsPlaying = false;
    // ... (기존 초기화) ...
    SelectedNotifyEventId = -1; // SequencerSelectedEntry는 im-neo-sequencer 방식으로 대체되거나 다른 의미로 사용될 수 있음

    if (TargetSequence)
    {
        // "Notifies" 루트 트랙 추가
        int rootTrackId = FEditorTimelineTrack::NextTrackIdCounter++;
        DisplayableTracks.Add(FEditorTimelineTrack(EEditorTimelineTrackType::AnimNotify_Root, "Notifies", rootTrackId, rootTrackId)); 
        // ParentId를 자기 자신으로 (또는 -1)

        // TODO: 여기서 TargetSequence에 저장된 사용자 트랙 정보를 읽어와 DisplayableTracks에 추가하는 로직 필요
        // 예:
        // for (const auto& savedTrackData : TargetSequence->GetSavedUserTracks())
        // {
        //     AddUserNotifyTrackFromData(rootTrackId, savedTrackData);
        // }

        // 임시로 하위 트랙 추가 예시 (테스트용)
        // AddUserNotifyTrack(rootTrackId, "Footstep Sounds");
        // AddUserNotifyTrack(rootTrackId, "Particle Effects");
    }
}

float SAnimationTimelinePanel::GetSequenceDurationSeconds() const
{
    return TargetSequence ? TargetSequence->SequenceLength : 0.0f;
}

float SAnimationTimelinePanel::GetSequenceFrameRate() const
{
    return TargetSequence ? TargetSequence->FrameRate : 30.0f;
}

int SAnimationTimelinePanel::GetSequenceTotalFrames() const
{
    if (!TargetSequence || GetSequenceDurationSeconds() <= 0.0f || GetSequenceFrameRate() <= 0.0f)
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

void SAnimationTimelinePanel::ConvertFrameToTimeAndSet(int frame)
{
    if (GetSequenceFrameRate() <= 0.0f)
    {
        CurrentTimeSeconds = 0.0f;
        return;
    }
    CurrentTimeSeconds = static_cast<float>(frame) / GetSequenceFrameRate();
    CurrentTimeSeconds = std::max(0.0f, std::min(CurrentTimeSeconds, GetSequenceDurationSeconds()));
}


void SAnimationTimelinePanel::Render()
{
    
    UpdatePlayback(0.01666);
    RenderTimelineEditor();

}

void SAnimationTimelinePanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}


void SAnimationTimelinePanel::RenderNotifyTrackItems(const TArray<FMockAnimNotifyEvent>& notifies, ImDrawList* drawList,
                                                     const ImRect& rc, const ImRect& clippingRect, bool isCompact)
{
    if (!TargetSequence)
    {
        return;
    }

    drawList->PushClipRect(clippingRect.Min, clippingRect.Max, true);

    const float trackHeight = rc.Max.y - rc.Min.y;
    const ImGuiStyle& style = ImGui::GetStyle();
    const float textPadding = style.FramePadding.x;

    bool mouseButtonJustReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

    if (bIsDraggingSelectedNotify && mouseButtonJustReleased)
    {
        bIsDraggingSelectedNotify = false;
        if (TargetSequence && SelectedNotifyEventId != -1)
        {
            TargetSequence->Notifies.Sort([](const FMockAnimNotifyEvent& A, const FMockAnimNotifyEvent& B)
                {
                    return A.TriggerTime < B.TriggerTime;
                });
        }
        // SelectedNotifyEventId = -1; // 선택 해제 필요시 주석 해제
    }

    for (const auto& notifyEvent : notifies)
    {
        if (notifyEvent.TriggerTime < 0 || notifyEvent.TriggerTime > GetSequenceDurationSeconds())
        {
            continue;
        }
        float normalizedPos = (GetSequenceDurationSeconds() > 0.0f) ? (notifyEvent.TriggerTime / GetSequenceDurationSeconds()) : 0.0f;
        float xPos = ImLerp(rc.Min.x, rc.Max.x, normalizedPos);

        std::string nameStrStd = notifyEvent.NotifyName.ToString().ToAnsiString();
        const char* nameCStr = nameStrStd.c_str();
        ImVec2 textSize = ImGui::CalcTextSize(nameCStr);

        float tagWidth = textSize.x + textPadding * 2;
        float tagHeight = isCompact ? std::min(textSize.y + style.FramePadding.y, trackHeight - 2.f) : std::min(textSize.y + style.FramePadding.y * 2, trackHeight - 4.f);
        tagHeight = std::max(tagHeight, 10.f);

        float tagStartX = xPos;
        float tagStartY = rc.Min.y + (trackHeight - tagHeight) * 0.5f;
        ImRect tagRect(ImVec2(tagStartX, tagStartY), ImVec2(tagStartX + tagWidth, tagStartY + tagHeight));

        if (tagRect.Max.x < rc.Min.x || tagRect.Min.x > rc.Max.x)
        {
            continue;
        }

        ImU32 bgColor = ImGui::GetColorU32(ImGuiCol_Button);
        if (SelectedNotifyEventId == notifyEvent.EventId)
        {
            bgColor = bIsDraggingSelectedNotify ? ImGui::GetColorU32(ImGuiCol_ButtonHovered) : ImGui::GetColorU32(ImGuiCol_ButtonActive);
        }
        ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);

        drawList->AddRectFilled(tagRect.Min, tagRect.Max, bgColor, 3.0f);
        drawList->PushClipRect(tagRect.Min, tagRect.Max, true);
        drawList->AddText(ImVec2(tagRect.Min.x + textPadding, tagRect.Min.y + (tagHeight - textSize.y) * 0.5f), textColor, nameCStr);
        drawList->PopClipRect();

        bool mouseOverThisTag = ImGui::IsMouseHoveringRect(tagRect.Min, tagRect.Max);

        if (mouseOverThisTag && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            SelectedNotifyEventId = notifyEvent.EventId;
            bIsDraggingSelectedNotify = true;
            DraggingNotifyOriginalTime = notifyEvent.TriggerTime;
            DraggingStartMousePosInTrack = ImVec2(ImGui::GetMousePos().x - rc.Min.x, ImGui::GetMousePos().y - rc.Min.y);
        }

        if (bIsDraggingSelectedNotify && SelectedNotifyEventId == notifyEvent.EventId && ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
        {
            ImVec2 currentMousePosInTrack = ImVec2(ImGui::GetMousePos().x - rc.Min.x, ImGui::GetMousePos().y - rc.Min.y);
            float mouseDeltaXInTrack = currentMousePosInTrack.x - DraggingStartMousePosInTrack.x;

            float trackPixelWidth = rc.Max.x - rc.Min.x;
            if (fabs(trackPixelWidth) >= FLT_EPSILON)
            {
                float timePerPixel = GetSequenceDurationSeconds() / trackPixelWidth;
                float timeDelta = mouseDeltaXInTrack * timePerPixel;
                float newTime = DraggingNotifyOriginalTime + timeDelta;
                newTime = std::max(0.0f, std::min(newTime, GetSequenceDurationSeconds()));

                for (int i = 0; i < TargetSequence->Notifies.Num(); ++i)
                {
                    if (TargetSequence->Notifies[i].EventId == SelectedNotifyEventId)
                    {
                        TargetSequence->Notifies[i].TriggerTime = newTime;
                        break;
                    }
                }
            }
        }

        if (mouseOverThisTag)
        {
            ImGui::SetTooltip("Notify: %s\nTime: %.2fs (Frame: %d)", nameCStr, notifyEvent.TriggerTime,
                static_cast<int>(notifyEvent.TriggerTime * GetSequenceFrameRate()));
        }

    }
    if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && !ImGui::IsAnyItemHovered() && !bIsDraggingSelectedNotify || ImGui::IsMouseReleased(0))
    {
        SelectedNotifyEventId = -1;
    }
    drawList->PopClipRect();
}
void SAnimationTimelinePanel::UpdatePlayback(float DeltaSeconds)
{
    if (!IsPlaying || !TargetSequence || GetSequenceDurationSeconds() <= 0.0f) return;

    float previousTimeSeconds = CurrentTimeSeconds;
    CurrentTimeSeconds += DeltaSeconds * PlaybackSpeed;

    if (CurrentTimeSeconds >= GetSequenceDurationSeconds())
    {
        if (IsLooping)
        {
            CurrentTimeSeconds = fmodf(CurrentTimeSeconds, GetSequenceDurationSeconds());
            TriggeredNotifyEventIdsThisPlayback.Empty();
        }
        else
        {
            CurrentTimeSeconds = GetSequenceDurationSeconds();
            IsPlaying = false;
        }
    }
    else if (CurrentTimeSeconds < 0.0f)
    {
        if (IsLooping)
        {
            CurrentTimeSeconds = GetSequenceDurationSeconds() - fmodf(-CurrentTimeSeconds, GetSequenceDurationSeconds());
            TriggeredNotifyEventIdsThisPlayback.Empty();
        }
        else
        {
            CurrentTimeSeconds = 0.0f;
            IsPlaying = false;
        }
    }
}

// 줌 레벨 변경 시 현재 보이는 중앙 프레임을 유지하도록 FirstVisibleFrame 조정
void SAnimationTimelinePanel::AdjustFirstFrameToKeepCenter()
{
    if (!TargetSequence || FramePixelWidth <= 0.0f) return;

    // 현재 보이는 화면의 중앙에 어떤 프레임이 있는지 계산
    // (RenderTimelineEditor에서 계산된 timelineWidgetRect.GetWidth() 사용 필요)
    float viewWidth = ImGui::GetContentRegionAvail().x; // 또는 마지막으로 그린 시퀀서 폭
    if (viewWidth < 100) viewWidth = 800; // 임시

    float viewCenterX = viewWidth * 0.5f;
    float centerFrameCurrentlyVisible = (float)SequencerFirstVisibleFrame + (viewCenterX / FramePixelWidth_BeforeChange);

    // 새로운 FramePixelWidth로 새로운 FirstVisibleFrame 계산
    SequencerFirstVisibleFrame = static_cast<int>(roundf(centerFrameCurrentlyVisible - (viewCenterX / FramePixelWidth)));

    ConvertFrameToTimeAndSet(ConvertTimeToFrame());
    // FitTimelineToView 호출 후에는 FramePixelWidth_BeforeChange도 현재 값으로 동기화하는 것이 좋을 수 있습니다.
    FramePixelWidth_BeforeChange = FramePixelWidth;
    ClampFirstVisibleFrame(); // SequencerFirstVisibleFrame이 0으로 설정되었으므로 호출
}
void SAnimationTimelinePanel::ClampFirstVisibleFrame()
{
    if (!TargetSequence) return;
    int totalFrames = GetSequenceTotalFrames();
    if (totalFrames <= 0)
    {
        SequencerFirstVisibleFrame = 0;
        return;
    }

    float viewWidth = ImGui::GetContentRegionAvail().x; // 또는 마지막으로 그린 시퀀서 폭
    if (viewWidth < 100) viewWidth = 800; // 임시

    int visibleFramesOnScreen = static_cast<int>(floorf(viewWidth / FramePixelWidth));
    if (visibleFramesOnScreen < 1) visibleFramesOnScreen = 1;

    if (SequencerFirstVisibleFrame + visibleFramesOnScreen > totalFrames)
    {
        SequencerFirstVisibleFrame = totalFrames - visibleFramesOnScreen;
    }
    if (SequencerFirstVisibleFrame < 0)
    {
        SequencerFirstVisibleFrame = 0;
    }
    if (totalFrames <= visibleFramesOnScreen) // 전체가 다 보이면 항상 0부터 시작
    {
        SequencerFirstVisibleFrame = 0;
    }
}

// (선택 사항) 특정 프레임이 뷰의 중앙에 오도록 SequencerFirstVisibleFrame 조정
void SAnimationTimelinePanel::CenterViewOnFrame(int targetFrame)
{
    if (!TargetSequence || FramePixelWidth <= 0.0f) return;

    float viewWidth = ImGui::GetContentRegionAvail().x; // 또는 마지막으로 그린 시퀀서 폭
    if (viewWidth < 100) viewWidth = 800; // 임시

    float viewCenterX = viewWidth * 0.5f;
    SequencerFirstVisibleFrame = static_cast<int>(roundf((float)targetFrame - (viewCenterX / FramePixelWidth)));
    ClampFirstVisibleFrame();
}


void SAnimationTimelinePanel::RenderPlaybackControls()
{
    if (!TargetSequence) return;

    // BeginChild의 높이를 충분히 확보하거나, 요소들이 넘치지 않도록 크기 조절
    // 예: ImGui::GetFrameHeightWithSpacing() * 2.0f (한 줄 기준) 또는 * 3.0f (두 줄 기준)
    ImGui::BeginChild("TimelineTopControlsArea", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 3.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // --- 첫 번째 줄: 재생 컨트롤 ---
    // 각 버튼에 고유 ID를 명시적으로 부여 (##ID 사용)
    if (IsPlaying)
    {
        if (ImGui::Button("Pause##PlaybackBtn")) { IsPlaying = false; }
    }
    else
    {
        if (ImGui::Button("Play##PlaybackBtn")) // 버튼 ID 확인
        {
            IsPlaying = true;
            if (CurrentTimeSeconds >= GetSequenceDurationSeconds() - FLT_EPSILON && GetSequenceDurationSeconds() > 0.f)
            {
                CurrentTimeSeconds = 0.0f;
                TriggeredNotifyEventIdsThisPlayback.Empty();
            }
            else if (CurrentTimeSeconds < FLT_EPSILON)
            {
                TriggeredNotifyEventIdsThisPlayback.Empty();
            }
        }
    }
    ImGui::SameLine(); // 같은 줄에 다음 요소 배치
    if (ImGui::Button("Stop##PlaybackBtn"))
    {
        IsPlaying = false;
        CurrentTimeSeconds = 0.0f;
        TriggeredNotifyEventIdsThisPlayback.Empty();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Loop##Playback", &IsLooping);
    ImGui::SameLine();

    ImGui::PushItemWidth(80); // 고정 너비
    ImGui::DragFloat("Speed##Playback", &PlaybackSpeed, 0.01f, 0.01f, 10.0f, "%.2fx");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Time 텍스트가 다른 요소에 밀리지 않도록 충분한 공간 확보
    // 또는 ImGui::SetNextItemWidth(-100) 등으로 남은 공간의 일부를 사용하게 할 수도 있음
    ImGui::Text("Time: %.2f / %.2f s", CurrentTimeSeconds, GetSequenceDurationSeconds());

    // --- 두 번째 줄: 뷰 컨트롤 및 프레임 슬라이더 ---
    // 명시적으로 줄바꿈을 원하면 ImGui::NewLine() 또는 ImGui::Separator() 사용
    // 여기서는 요소들이 자동으로 다음 줄로 넘어가도록 배치
    // ImGui::Separator(); // 구분선 추가

  
    ImGui::SameLine();

    ImGui::PushItemWidth(150); // 줌 슬라이더 너비
    float oldFramePixelWidth = FramePixelWidth;
    if (ImGui::SliderFloat("Zoom##ViewCtrl", &FramePixelWidth, 0.1f, 200.0f, "%.1f px/fr", ImGuiSliderFlags_Logarithmic))
    {
        if (fabs(FramePixelWidth - oldFramePixelWidth) > FLT_EPSILON)
        {
            FramePixelWidth_BeforeChange = oldFramePixelWidth;
            AdjustFirstFrameToKeepCenter();
        }
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();

    int currentFrameDisplay = ConvertTimeToFrame();
    int totalFramesDisplay = GetSequenceTotalFrames();
    // 프레임 슬라이더 너비 조절
    // float frameSliderWidth = ImGui::GetContentRegionAvail().x; // 남은 공간 모두 사용
    // if (frameSliderWidth < 50.0f) frameSliderWidth = 50.0f; // 최소 너비 보장
    ImGui::PushItemWidth(150); // 또는 고정 너비
    if (totalFramesDisplay > 0)
    {
        if (ImGui::SliderInt("Frame##Scroll", &currentFrameDisplay, 0, totalFramesDisplay - 1 < 0 ? 0 : totalFramesDisplay - 1))
        {
            ConvertFrameToTimeAndSet(currentFrameDisplay);
            if (IsPlaying) IsPlaying = false;
            TriggeredNotifyEventIdsThisPlayback.Empty();
        }
    }
    else
    {
        ImGui::TextUnformatted("(No frames)");
    }
    ImGui::PopItemWidth();

    ImGui::EndChild(); // TimelineTopControlsArea
}

void SAnimationTimelinePanel::RenderTimelineEditor()
{
    if (!TargetSequence)
    {
        ImGui::Text("No MockAnimSequence set. Please call SetTargetSequence().");
        return;
    }




    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    if (!mainViewport) // 뷰포트 정보가 없으면 (매우 예외적인 상황) 아무것도 하지 않음
    {
        return;
    }

    const float timelinePanelHeight = 300.0f;

    ImVec2 windowPos = ImVec2(mainViewport->WorkPos.x, mainViewport->WorkPos.y + mainViewport->WorkSize.y - timelinePanelHeight);
    ImVec2 windowSize = ImVec2(mainViewport->WorkSize.x, timelinePanelHeight);
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    if (mainViewport->WorkSize.y <= timelinePanelHeight)
    {
        // 뷰포트 세로 크기가 타임라인 패널 높이보다 작거나 같으면,
        // 타임라인 패널을 그리지 않거나 매우 작게 그리는 등의 예외 처리 가능
        // 여기서는 그냥 진행하지만, 실제로는 UI가 이상하게 보일 수 있음
    }

    if (ImGui::Begin("AnimationTimelineHost", nullptr, windowFlags))
    {

        // 상단 컨트롤들을 그리기 위한 자식 창
        ImGui::BeginChild("TimelineTopControlsArea", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2.5f),
            false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);


        float currentMainAreaWidth = ImGui::GetContentRegionAvail().x;
        if (currentMainAreaWidth >= 50.0f) // 매우 작은 값 방지 (최소 50px 이라고 가정)
        {
            LastSequencerAreaWidth = currentMainAreaWidth;
        }


        RenderPlaybackControls(); // 재생 컨트롤 그리기

        ImGui::EndChild();        // TimelineTopControlsArea

        ImGui::BeginChild("TimelineMainArea");
        RenderSequencerWidget();
        RenderNotifyEditor();
        ImGui::EndChild();
        ImGui::End();
    }
}
void SAnimationTimelinePanel::RenderSequencerWidget()
{
    if (!TargetSequence) return;

    // im-neo-sequencer 프레임 타입은 uint32_t (FrameIndexType)
    ImGui::FrameIndexType currentFrame = static_cast<ImGui::FrameIndexType>(ConvertTimeToFrame());
    ImGui::FrameIndexType startFrame = 0; // GetFrameMin() 대신
    ImGui::FrameIndexType endFrame = static_cast<ImGui::FrameIndexType>(GetSequenceTotalFrames()); // GetFrameMax() 대신 (총 프레임 수)

    // endFrame이 0이면 시퀀서가 제대로 동작하지 않을 수 있으므로 최소 길이 보장
    if (endFrame <= startFrame) {
        endFrame = startFrame + 1; // 또는 적절한 최소 길이
    }

    // 시퀀서 플래그 설정
    ImGuiNeoSequencerFlags sequencerFlags = ImGuiNeoSequencerFlags_EnableSelection |
        ImGuiNeoSequencerFlags_Selection_EnableDragging |
        ImGuiNeoSequencerFlags_Selection_EnableDeletion;
    if (/* 길이 변경 허용 조건 */ true) { // 필요에 따라 조건 추가
        sequencerFlags |= ImGuiNeoSequencerFlags_AllowLengthChanging;
    }
    // if (/* 줌 숨기기 조건 */ false) {
    //     sequencerFlags |= ImGuiNeoSequencerFlags_HideZoom;
    // }


    // BeginNeoSequencer 호출
    // 세 번째 파라미터 startFrame, 네 번째 파라미터 endFrame은 포인터로 전달되어 내부에서 변경될 수 있음 (AllowLengthChanging 플래그 시)
    if (ImGui::BeginNeoSequencer("AnimationNotifySequencer", &currentFrame, &startFrame, &endFrame, ImVec2(0, 0) /* 자동 크기 */, sequencerFlags))
    {
        // 현재 프레임이 UI를 통해 변경되었다면 게임/애니메이션 상태에 반영
        if (currentFrame != static_cast<ImGui::FrameIndexType>(ConvertTimeToFrame()))
        {
            ConvertFrameToTimeAndSet(static_cast<int>(currentFrame));
            if (IsPlaying) IsPlaying = false;
            TriggeredNotifyEventIdsThisPlayback.Empty();
        }

        // (선택적) 시퀀스 길이 변경 시 처리
        if (sequencerFlags & ImGuiNeoSequencerFlags_AllowLengthChanging)
        {
            if (startFrame != 0 || endFrame != static_cast<ImGui::FrameIndexType>(GetSequenceTotalFrames()))
            {
                // TODO: TargetSequence의 전체 길이를 startFrame, endFrame에 맞게 업데이트하는 로직
                // 예: TargetSequence->SequenceLength = static_cast<float>(endFrame) / GetSequenceFrameRate();
                // GetFrameMin()이 항상 0이 아니라면 startFrame도 고려해야 함.
            }
        }

        // DisplayableTracks를 순회하며 타임라인 그리기
        for (int i = 0; i < DisplayableTracks.Num(); ++i)
        {
            FEditorTimelineTrack& uiTrack = DisplayableTracks[i]; // bIsExpanded 상태 변경을 위해 non-const 참조

            if (uiTrack.TrackType == EEditorTimelineTrackType::AnimNotify_Root)
            {
                // 루트 트랙 (그룹으로 표시)
                // BeginNeoTimelineEx의 두 번째 파라미터는 bool* open 상태입니다.
                if (ImGui::BeginNeoTimelineEx(uiTrack.DisplayName.c_str(), &uiTrack.bIsExpanded, ImGuiNeoTimelineFlags_Group))
                {
                    // 그룹이 열렸을 때만 내부(하위) 타임라인들을 그립니다.
                    // DisplayableTracks는 이미 순서대로 정렬되어 있다고 가정 (루트 다음 자식들)
                    // 또는 여기서 ParentTrackId를 확인하여 해당 루트의 자식만 그리는 로직 추가
                    ImGui::EndNeoTimeLine(); // 그룹 타임라인 닫기
                }
            }
            else if (uiTrack.TrackType == EEditorTimelineTrackType::AnimNotify_UserTrack)
            {
                // 이 사용자 트랙이 표시되어야 하는지 확인 (부모 루트가 열려있는지)
                bool bShouldDisplay = false;
                for (const auto& parentTrack : DisplayableTracks) {
                    if (parentTrack.TrackId == uiTrack.ParentTrackId && parentTrack.bIsExpanded) {
                        bShouldDisplay = true;
                        break;
                    }
                }

                if (bShouldDisplay)
                {
                    // 사용자 하위 트랙
                    // BeginNeoTimelineEx의 두 번째 파라미터는 일반 타임라인이므로 nullptr (또는 확장 기능이 없다면)
                    if (ImGui::BeginNeoTimelineEx(uiTrack.DisplayName.c_str(), nullptr, ImGuiNeoTimelineFlags_None))
                    {
                        // 이 트랙에 속한 노티파이들을 키프레임으로 그립니다.
                        for (int notifyIdx = 0; notifyIdx < TargetSequence->Notifies.Num(); ++notifyIdx)
                        {
                            FMockAnimNotifyEvent& notifyEvent = TargetSequence->Notifies[notifyIdx]; // 값 변경 위해 non-const
                            if (notifyEvent.UserInterfaceTrackId == uiTrack.TrackId)
                            {
                                // FMockAnimNotifyEvent의 TriggerTime을 프레임으로 변환
                                int32_t keyframeTime = static_cast<int32_t>(notifyEvent.TriggerTime * GetSequenceFrameRate());

                                // NeoKeyframe은 int32_t*를 받으므로, 임시 변수 또는 직접 수정 가능한 포인터 필요
                                // 중요: 만약 notifyEvent.TriggerTime (또는 keyframeTime)이 NeoKeyframe 내부에서
                                // 드래그 등으로 변경될 수 있다면, 그 변경 사항을 다시 원래 데이터에 반영해야 합니다.
                                // NeoKeyframe이 값 자체를 변경하는지, 아니면 콜백을 제공하는지 API 확인 필요.
                                // 현재 im-neo-sequencer API를 보면 NeoKeyframe(int32_t* value)는 포인터를 받으므로,
                                // value가 가리키는 메모리의 값이 직접 변경될 수 있습니다.

                                ImGui::PushID(notifyEvent.EventId); // 각 키프레임에 고유 ID 부여
                                ImGui::NeoKeyframe(&keyframeTime); // 주소 전달

                                // 키프레임 값 변경 감지 및 원래 데이터 업데이트
                                float newTimeBasedOnFrame = static_cast<float>(keyframeTime) / GetSequenceFrameRate();
                                if (fabs(newTimeBasedOnFrame - notifyEvent.TriggerTime) > FLT_EPSILON) {
                                    notifyEvent.TriggerTime = std::max(0.0f, std::min(newTimeBasedOnFrame, GetSequenceDurationSeconds()));
                                    // TODO: 변경 사항에 대한 추가 처리 (예: 정렬, Undo/Redo)
                                    bIsDraggingSelectedNotify = true; // 드래그 중임을 표시 (정렬 등에 사용)
                                }


                                // 키프레임 선택 및 우클릭 등 상호작용 처리
                                if (ImGui::IsNeoKeyframeSelected()) {
                                    SelectedNotifyEventId = notifyEvent.EventId;
                                    // 선택 시 추가 작업
                                }
                                if (ImGui::IsNeoKeyframeHovered()) {
                                    // 호버 시 툴팁 등 표시
                                    ImGui::SetTooltip("Notify: %s\nTime: %.2fs (Frame: %d)",
                                        notifyEvent.NotifyName.ToString().ToAnsiString().c_str(),
                                        notifyEvent.TriggerTime,
                                        static_cast<int>(notifyEvent.TriggerTime * GetSequenceFrameRate()));
                                }
                                if (ImGui::IsNeoKeyframeRightClicked()) {
                                    // 우클릭 메뉴 띄우기 등
                                    SelectedNotifyEventId = notifyEvent.EventId; // 컨텍스트 메뉴를 위해 선택
                                    // ImGui::OpenPopup(...)
                                }
                                ImGui::PopID();
                            }
                        }

                        // 선택된 타임라인에 대한 처리 (예: 컨텍스트 메뉴 - 트랙 삭제 등)
                        if (ImGui::IsNeoTimelineSelected()) {
                            // 이 트랙이 선택됨
                            SequencerSelectedEntry = uiTrack.TrackId; // 예시: 선택된 트랙 ID 저장
                            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsWindowHovered(ImGuiHoveredFlags_RectOnly)) {
                                // ImGui::OpenPopupOnItemClick("TrackContextPopup", ImGuiPopupFlags_MouseButtonRight);
                            }
                        }


                        ImGui::EndNeoTimeLine(); // 하위 타임라인 닫기
                    }
                }
            }
        }

        // 시퀀서 전체 영역에서의 상호작용 처리 (예: 빈 공간 클릭 시 선택 해제)
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemActive() && !ImGui::IsAnyItemHovered()) {
            ImGui::NeoClearSelection(); // im-neo-sequencer의 선택 해제 API
            SelectedNotifyEventId = -1;
            SequencerSelectedEntry = -1;
        }

        // 드래그 완료 후 정렬
        if (bIsDraggingSelectedNotify && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            TargetSequence->Notifies.Sort([](const FMockAnimNotifyEvent& A, const FMockAnimNotifyEvent& B) {
                return A.TriggerTime < B.TriggerTime;
                });
            bIsDraggingSelectedNotify = false;
        }


        ImGui::EndNeoSequencer();
    }
}
void SAnimationTimelinePanel::RenderNotifyEditor()
{
    if (!TargetSequence)
    {
        return;
    }

    ImGui::Separator();
    int rootTrackId = -1;
    for (const auto& track : DisplayableTracks) 
    {
        if (track.TrackType == EEditorTimelineTrackType::AnimNotify_Root) 
        {
            rootTrackId = track.TrackId;
            break;
        }
    }

    if (rootTrackId != -1) {
        if (ImGui::Button("Add Notify Track")) 
        {
            // 이름 입력 UI (예: 팝업)
            static char newTrackNameInput[128] = "New User Track";
            ImGui::InputText("Track Name", newTrackNameInput, IM_ARRAYSIZE(newTrackNameInput));
            if (ImGui::Button("Create")) 
            {
                // 실제로는 모달 팝업 등을 사용하는 것이 좋음
                AddUserNotifyTrack(rootTrackId, newTrackNameInput);
            }
        }
    }


    // 선택된 노티파이 편집 (이전과 유사하게 유지)
    if (SelectedNotifyEventId != -1)
    {
        FMockAnimNotifyEvent* currentSelectedEvent = nullptr;
        for (auto& notify : TargetSequence->Notifies) {
            if (notify.EventId == SelectedNotifyEventId) {
                currentSelectedEvent = &notify;
                break;
            }
        }

        if (currentSelectedEvent) {
            ImGui::Text("Editing Notify: %s (ID: %d)", currentSelectedEvent->NotifyName.ToString().ToAnsiString().c_str(), currentSelectedEvent->EventId);
            // 여기에 트랙 할당 UI 추가
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

            if (ImGui::BeginCombo("##TrackAssignCombo", currentAssignedTrackName.c_str())) 
            {
                // 기본 할당 (특정 트랙에 속하지 않음)
                if (ImGui::Selectable("None (Default)", currentAssignedUiTrackId <= 0)) 
                { // ID가 0 또는 음수면 특정 트랙 아님
                    currentSelectedEvent->UserInterfaceTrackId = 0; // 또는 -1
                }
                for (const auto& track : DisplayableTracks) {
                    if (track.TrackType == EEditorTimelineTrackType::AnimNotify_UserTrack) {
                        if (ImGui::Selectable(track.DisplayName.c_str(), currentAssignedUiTrackId == track.TrackId)) {
                            currentSelectedEvent->UserInterfaceTrackId = track.TrackId;
                        }
                    }
                }
                ImGui::EndCombo();
            }
        }
    }
}

void SAnimationTimelinePanel::AddUserNotifyTrack(int InParentRootTrackId, const std::string& NewTrackName)
{
    if (!TargetSequence) return;

    // 부모 루트 트랙 찾기 및 인덱스 확인
    FEditorTimelineTrack* parentRootTrack = nullptr;
    int parentRootTrackIndex = -1;
    for (int i = 0; i < DisplayableTracks.Num(); ++i)
    {
        if (DisplayableTracks[i].TrackId == InParentRootTrackId &&
            DisplayableTracks[i].TrackType == EEditorTimelineTrackType::AnimNotify_Root)
        {
            parentRootTrack = &DisplayableTracks[i];
            parentRootTrackIndex = i;
            break;
        }
    }

    if (parentRootTrack)
    {
        int newTrackId = FEditorTimelineTrack::NextTrackIdCounter++;

        // 새 트랙을 부모 루트 트랙의 자식들 중 가장 마지막에 삽입
        // 또는 특정 정렬 기준이 있다면 그에 맞게 삽입 위치를 찾습니다.
        int insertAtIndex = parentRootTrackIndex + 1; // 기본적으로 부모 바로 다음
        for (int i = parentRootTrackIndex + 1; i < DisplayableTracks.Num(); ++i)
        {
            // 부모가 같은 다른 자식 트랙들을 지나, 다른 부모의 트랙이 시작되거나 배열 끝에 도달하면 그 위치
            if (DisplayableTracks[i].ParentTrackId == InParentRootTrackId)
            {
                insertAtIndex = i + 1;
            }
            else // 다른 부모를 만나거나, 루트가 아닌 다른 타입의 트랙을 만나면 그 앞에 삽입
            {
                break;
            }
        }

        // IndentLevel은 부모보다 하나 더 깊게
        int indentLevel = parentRootTrack->IndentLevel + 1;
        FEditorTimelineTrack newUserTrack(EEditorTimelineTrackType::AnimNotify_UserTrack, NewTrackName, newTrackId, InParentRootTrackId, indentLevel);

        if (insertAtIndex >= DisplayableTracks.Num())
        {
            DisplayableTracks.Add(newUserTrack);
        }
        else
        {
            DisplayableTracks[insertAtIndex] = newUserTrack;
        }

         parentRootTrack->bIsExpanded = true;

        // (선택사항) 새로 추가된 트랙을 선택 상태로 만들 수 있습니다.
        // SetSelectedTimeline(NewTrackName.c_str()); // im-neo-sequencer API가 있다면
        // 또는 SequencerSelectedEntry = newTrackId; (자체 관리 방식)
    }
    else
    {
        // 부모 루트 트랙을 찾지 못한 경우에 대한 처리 (예: 로그 출력)
        // UE_LOG(LogTemp, Warning, TEXT("AddUserNotifyTrack: Parent root track with ID %d not found."), InParentRootTrackId);
    }
}

// (선택적) 트랙 제거 함수 구현 예시
void SAnimationTimelinePanel::RemoveUserNotifyTrack(int TrackIdToRemove)
{
    if (!TargetSequence) return;

    int trackIndexToRemove = -1;
    for (int i = 0; i < DisplayableTracks.Num(); ++i)
    {
        if (DisplayableTracks[i].TrackId == TrackIdToRemove &&
            DisplayableTracks[i].TrackType == EEditorTimelineTrackType::AnimNotify_UserTrack)
        {
            trackIndexToRemove = i;
            break;
        }
    }

    if (trackIndexToRemove != -1)
    {
        DisplayableTracks.RemoveAt(trackIndexToRemove);

        // 이 트랙에 할당되었던 노티파이들의 UserInterfaceTrackId를 기본값(예: 0 또는 -1)으로 변경
        for (auto& notifyEvent : TargetSequence->Notifies)
        {
            if (notifyEvent.UserInterfaceTrackId == TrackIdToRemove)
            {
                notifyEvent.UserInterfaceTrackId = 0; // 또는 다른 기본 트랙 ID
            }
        }
        // 선택 상태 등 관련 데이터 초기화
        // if (SequencerSelectedEntry == TrackIdToRemove) SequencerSelectedEntry = -1;
    }
}

// (선택적) 노티파이 할당 함수 구현 예시
void SAnimationTimelinePanel::AssignNotifyToTrack(int NotifyEventId, int TargetTrackId)
{
    if (!TargetSequence)
    {
        return;
    }

    for (auto& notifyEvent : TargetSequence->Notifies)
    {
        if (notifyEvent.EventId == NotifyEventId)
        {
            notifyEvent.UserInterfaceTrackId = TargetTrackId; // TargetTrackId가 0이면 특정 트랙 아님
            break;
        }
    }
}
