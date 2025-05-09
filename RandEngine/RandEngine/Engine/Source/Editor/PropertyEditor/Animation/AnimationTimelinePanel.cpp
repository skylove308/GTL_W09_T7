#include "AnimationTimelinePanel.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_internal.h"

// ... (Static Member Initializations) ...
#if !defined(FNAME_DEFINED) || !defined(MOCK_GLOBALS_DEFINED)
#define MOCK_GLOBALS_DEFINED
int FMockAnimNotifyEvent::NextEventId = 0;
int FEditorTimelineTrack::NextTrackIdCounter = 0;
#endif

SAnimationTimelinePanel::SAnimationTimelinePanel()
    : TargetSequence(nullptr), IsSequencerExpanded(true)
{
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
        DisplayableTracks.Add(FEditorTimelineTrack(EEditorTimelineTrackType::AnimNotify, "Notifies"));
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
    if (!TargetSequence || GetSequenceDurationSeconds() <= 0.0f || GetSequenceFrameRate() <= 0.0f) {
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
    if (type)  *type = (int)DisplayableTracks[index].Type;
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
    if (uiTrack.Type == EEditorTimelineTrackType::AnimNotify)
    {
        RenderNotifyTrackItems(TargetSequence->Notifies, drawList, rc, clippingRect, false);
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
    if (uiTrack.Type == EEditorTimelineTrackType::AnimNotify)
    {
        RenderNotifyTrackItems(TargetSequence->Notifies, drawList, rc, clippingRect, true);
    }
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


// --- 분리된 UI 렌더링 함수들 ---
void SAnimationTimelinePanel::RenderPlaybackControls()
{
    if (!TargetSequence) return;

    ImGui::BeginChild("TimelineTopControls", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 2.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (IsPlaying) {
        if (ImGui::Button("Pause##Playback")) IsPlaying = false;
    }
    else {
        if (ImGui::Button("Play##Playback")) {
            IsPlaying = true;
            if (CurrentTimeSeconds >= GetSequenceDurationSeconds() - FLT_EPSILON && GetSequenceDurationSeconds() > 0.f) {
                CurrentTimeSeconds = 0.0f;
                TriggeredNotifyEventIdsThisPlayback.Empty();
            }
            else if (CurrentTimeSeconds < FLT_EPSILON) {
                TriggeredNotifyEventIdsThisPlayback.Empty();
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop##Playback")) {
        IsPlaying = false;
        CurrentTimeSeconds = 0.0f;
        TriggeredNotifyEventIdsThisPlayback.Empty();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Loop##Playback", &IsLooping);
    ImGui::SameLine();
    ImGui::PushItemWidth(80);
    ImGui::DragFloat("Speed##Playback", &PlaybackSpeed, 0.01f, 0.01f, 10.0f, "%.2fx");
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Text("Time: %.2f / %.2f s", CurrentTimeSeconds, GetSequenceDurationSeconds());
    ImGui::SameLine();

    int currentFrameDisplay = ConvertTimeToFrame();
    int totalFramesDisplay = GetSequenceTotalFrames();
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
    if (totalFramesDisplay > 0) {
        if (ImGui::SliderInt("Frame##Playback", &currentFrameDisplay, 0, totalFramesDisplay)) {
            ConvertFrameToTimeAndSet(currentFrameDisplay);
            if (IsPlaying) IsPlaying = false;
            TriggeredNotifyEventIdsThisPlayback.Empty();
        }
    }
    else {
        ImGui::TextUnformatted("(No frames)");
    }
    ImGui::PopItemWidth();
    ImGui::EndChild(); // TimelineTopControls
}

void SAnimationTimelinePanel::RenderSequencerWidget()
{
    if (!TargetSequence) return;

    int currentFrameForSequencer = ConvertTimeToFrame();
    ImSequencer::Sequencer(this, &currentFrameForSequencer, &IsSequencerExpanded, &SequencerSelectedEntry, &SequencerFirstVisibleFrame, 0);
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
    if (ImGui::Button("Add Mock Notify At Current Time")) {
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

void SAnimationTimelinePanel::RenderTimelineEditor()
{
    if (!TargetSequence)
    {
        ImGui::Text("No MockAnimSequence set. Please call SetTargetSequence().");
        return;
    }

    RenderPlaybackControls(); // 분리된 함수 호출

    ImGui::BeginChild("TimelineMainArea"); // 하단 메인 영역 시작
    RenderSequencerWidget(); // 분리된 함수 호출
    RenderNotifyEditor();    // 분리된 함수 호출
    ImGui::EndChild();       // 하단 메인 영역 끝
}
