#include "AnimationTimelinePanel.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_internal.h"
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
    IsLooping = false;
    PlaybackSpeed = 1.0f;
    SequencerSelectedEntry = -1;
    SequencerFirstVisibleFrame = 0;
    TriggeredNotifyEventIdsThisPlayback.Empty();
    SelectedNotifyEventId = -1;

    if (TargetSequence)
    {
        DisplayableTracks.Add(FEditorTimelineTrack(EEditorTimelineTrackType::AnimNotify_Root, "Notifies", 0));
        SequencerSelectedEntry = 0;
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

// --- ImSequencer::SequenceInterface Implementation ---
int SAnimationTimelinePanel::GetFrameMin() const
{
    return 0;
}

int SAnimationTimelinePanel::GetFrameMax() const
{
    return GetSequenceTotalFrames();
}

int SAnimationTimelinePanel::GetItemCount() const
{
    return DisplayableTracks.Num();
}

const char* SAnimationTimelinePanel::GetItemLabel(int index) const
{
    if (index >= 0 && index < DisplayableTracks.Num())
    {
        return DisplayableTracks[index].DisplayName.c_str();
    }
    return "";
}

void SAnimationTimelinePanel::Get(int index, int** start, int** end, int* type, unsigned int* color)
{
    if (index < 0 || index >= DisplayableTracks.Num())
    {
        return;
    }
    static int trackStart = 0;
    static int trackEnd = 0;
    trackStart = GetFrameMin();
    trackEnd = GetFrameMax(); // GetSequenceTotalFrames() 대신 GetFrameMax() 사용 (일관성)

    if (start) *start = &trackStart;
    if (end)   *end = &trackEnd;
    if (type)  *type = (int)DisplayableTracks[index].TrackType;
    if (color) *color = 0xFF808080;
}

void SAnimationTimelinePanel::CustomDraw(int displayTrackIndex, ImDrawList* drawList, const ImRect& rc,
    const ImRect& legendRect, const ImRect& clippingRect,
    const ImRect& legendClippingRect)
{
    if (!TargetSequence || displayTrackIndex < 0 || displayTrackIndex >= DisplayableTracks.Num())
    {
        return;
    }
    const FEditorTimelineTrack& uiTrack = DisplayableTracks[displayTrackIndex];
    if (uiTrack.TrackType == EEditorTimelineTrackType::AnimNotify_Root)
    {
    }
    else if (uiTrack.TrackType == EEditorTimelineTrackType::AnimNotify_Item)
    {
        TArray<FMockAnimNotifyEvent> filteredNotifies;
        for (const auto& notifyEvent : TargetSequence->Notifies)
        {
            // 함수 호출 대신 직접 조건 비교
            if (notifyEvent.UserInterfaceTrackId == uiTrack.TrackId)
            {
                filteredNotifies.Add(notifyEvent);
            }
        }
        if (filteredNotifies.Num() > 0)
        {
            RenderNotifyTrackItems(filteredNotifies, drawList, rc, clippingRect, false);
        }
    }
}

void SAnimationTimelinePanel::CustomDrawCompact(int displayTrackIndex, ImDrawList* drawList, const ImRect& rc,
    const ImRect& clippingRect)
{
    if (!TargetSequence || displayTrackIndex < 0 || displayTrackIndex >= DisplayableTracks.Num())
    {
        return;
    }
    const FEditorTimelineTrack& uiTrack = DisplayableTracks[displayTrackIndex];
    if (uiTrack.TrackType == EEditorTimelineTrackType::AnimNotify_Root)
    {
    }
    else if (uiTrack.TrackType == EEditorTimelineTrackType::AnimNotify_Item)
    {
        TArray<FMockAnimNotifyEvent> filteredNotifies;
        for (const auto& notifyEvent : TargetSequence->Notifies)
        {
            // 함수 호출 대신 직접 조건 비교
            if (notifyEvent.UserInterfaceTrackId == uiTrack.TrackId)
            {
                filteredNotifies.Add(notifyEvent);
            }
        }
        if (filteredNotifies.Num() > 0)
        {
            RenderNotifyTrackItems(filteredNotifies, drawList, rc, clippingRect, true);
        }
    }
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
void SAnimationTimelinePanel::FitTimelineToView()
{
    if (!TargetSequence || GetSequenceTotalFrames() <= 0)
    {
        return;
    }
    float availableWidth = LastSequencerAreaWidth; // *** 새 코드 ***

    SequencerFirstVisibleFrame = 0;
    FramePixelWidth = availableWidth / (float)GetSequenceTotalFrames();

    // 최소/최대 FramePixelWidth 제한은 그대로 유지하거나 필요에 맞게 조정
    if (FramePixelWidth < 0.01f) FramePixelWidth = 0.01f; // 더 작게 허용 가능

    ConvertFrameToTimeAndSet(ConvertTimeToFrame()); // 현재 프레임 유지 시도
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

    if (ImGui::Button("Fit To View##ViewCtrl"))
    {
        FitTimelineToView();
    }
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
void SAnimationTimelinePanel::RenderViewControls() // 새로운 뷰 컨트롤 함수
{
    if (!TargetSequence) return;

    // --- 뷰 컨트롤 (전체 보기 및 줌 슬라이더) ---
    if (ImGui::Button("Fit To View##ViewCtrl"))
    {
        FitTimelineToView();
    }
    ImGui::SameLine();

    ImGui::PushItemWidth(150);
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

    // --- 프레임 스크롤/이동 슬라이더 ---
    int currentFrameDisplay = ConvertTimeToFrame();
    int totalFramesDisplay = GetSequenceTotalFrames();
    // float frameSliderWidth = ImGui::GetContentRegionAvail().x;
    // if (frameSliderWidth < 100.0f) frameSliderWidth = 100.0f;
    // ImGui::PushItemWidth(frameSliderWidth);
    ImGui::PushItemWidth(150); // 또는 고정 너비
    if (totalFramesDisplay > 0)
    {
        if (ImGui::SliderInt("Frame##Scroll", &currentFrameDisplay, 0, totalFramesDisplay - 1 < 0 ? 0 : totalFramesDisplay - 1))
        {
            ConvertFrameToTimeAndSet(currentFrameDisplay);
            if (IsPlaying)
            {
                IsPlaying = false;
            }
            TriggeredNotifyEventIdsThisPlayback.Empty();
            CenterViewOnFrame(currentFrameDisplay); // 선택 사항
        }
    }
    else
    {
        ImGui::TextUnformatted("(No frames)");
    }
    ImGui::PopItemWidth();
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

        RenderViewControls();     // 뷰 컨트롤 그리기

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

    int currentFrameForSequencer = ConvertTimeToFrame();
    ImSequencer::Sequencer(this, &currentFrameForSequencer,
        &IsSequencerExpanded, &SequencerSelectedEntry, &SequencerFirstVisibleFrame, 0);

    if (currentFrameForSequencer != ConvertTimeToFrame())
    {
        ConvertFrameToTimeAndSet(currentFrameForSequencer);
        if (IsPlaying) IsPlaying = false;
        TriggeredNotifyEventIdsThisPlayback.Empty();
    }
}

void SAnimationTimelinePanel::RenderNotifyEditor()
{
    if (!TargetSequence)
    {
        return;
    }

    ImGui::Separator();
    if (ImGui::Button("Add Mock Notify At Current Time"))
    {
        if (TargetSequence)
        {
            TargetSequence->AddNotify(CurrentTimeSeconds, FName("MockEvent"));
        }
    }
    if (SelectedNotifyEventId != -1 && TargetSequence)
    {
        FMockAnimNotifyEvent* selectedEvent = nullptr;
        for (int i = 0; i < TargetSequence->Notifies.Num(); ++i)
        {
            if (TargetSequence->Notifies[i].EventId == SelectedNotifyEventId)
            {
                selectedEvent = &TargetSequence->Notifies[i];
                break;
            }
        }
        if (selectedEvent)
        {
            ImGui::Text("Editing: %s (ID: %d)", selectedEvent->NotifyName.ToString().ToAnsiString().c_str(), selectedEvent->EventId);
            // 여기에 선택된 노티파이의 상세 편집 UI 추가 가능
        }
    }
}
