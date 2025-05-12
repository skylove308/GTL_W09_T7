#include "SkeletalDetailPanel.h"

#include "Animation/Skeleton.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Engine/SkeletalMeshActor.h"
#include "Math/JungleMath.h"
#include "SubWindow/SkeletalSubEngine.h"
#include "UnrealEd/ImGuiWidget.h"

void SkeletalDetailPanel::Render(USkeletalMesh* InSkeletalMesh, int32 SelectedBone)
{
    ImVec2 WinSize = ImVec2(Width, Height);
    
    ImGui::SetNextWindowPos(ImVec2(WinSize.x * 0.8f + 2.0f, 0));

    ImGui::SetNextWindowSize(ImVec2(WinSize.x * 0.25f - 5.0f, 500));
    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;

    if (InSkeletalMesh == nullptr)
        return;

    if (ImGui::Begin("Details", nullptr, PanelFlags))
    {
        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.05f, 0.05f, 0.08f, 0.80f));         // 비활성 탭
        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.35f, 0.35f, 0.40f, 1.00f));  // 호버 탭
        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.20f, 0.20f, 0.25f, 1.00f));   // 활성 탭

        if (ImGui::BeginTabBar("DetailsTabBar", ImGuiTabBarFlags_AutoSelectNewTabs))
        {
            if (ImGui::BeginTabItem("Bone",nullptr))
            {
                if (ImGui::CollapsingHeader("Bone##BoneInfo", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("Bone Name : %s ", *InSkeletalMesh->Skeleton->BoneTree[SelectedBone].Name.ToString());
                }
                FMatrix CurrentLocalMatrix = InSkeletalMesh->GetBoneLocalMatrix(SelectedBone);

                FVector Location = CurrentLocalMatrix.GetTranslationVector();
                FRotator Rotation = CurrentLocalMatrix.GetRotationVector();;
                FVector Scale = CurrentLocalMatrix.GetScaleVector();
                if (ImGui::CollapsingHeader("Transforms", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    // optionally indent for nicer grouping
                    ImGui::Indent(10.0f);

                    if (ImGui::CollapsingHeader("Bone", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PushID("BoneSection");
                        ImGui::Indent(10.0f);
                        FImGuiWidget::DrawVec3Control("Location", Location, 0, 85);
                        ImGui::Spacing();

                        FImGuiWidget::DrawRot3Control("Rotation", Rotation, 0, 85);
                        ImGui::Spacing();

                        FImGuiWidget::DrawVec3Control("Scale", Scale, 0, 85);
                        ImGui::Spacing();
                        ImGui::Unindent(10.0f);
                        ImGui::PopID();
                    }

                    if (ImGui::CollapsingHeader("Reference", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PushID("RefSection");
                        FMatrix RefCurrentRefMatrix = InSkeletalMesh->Skeleton->ReferenceSkeleton.RefBonePose[SelectedBone];
                        FVector RefLocation = RefCurrentRefMatrix.GetTranslationVector();
                        FRotator RefRotation = RefCurrentRefMatrix.GetRotationVector();;
                        FVector RefScale = RefCurrentRefMatrix.GetScaleVector();
                        ImGui::Indent(10.0f);
                        FImGuiWidget::DrawVec3Control("Location", RefLocation, 0, 85);
                        ImGui::Spacing();

                        FImGuiWidget::DrawRot3Control("Rotation", RefRotation, 0, 85);
                        ImGui::Spacing();

                        FImGuiWidget::DrawVec3Control("Scale", RefScale, 0, 85);
                        ImGui::Spacing();
                        ImGui::Unindent(10.0f);
                        ImGui::PopID();
                    }

                    ImGui::Unindent(10.0f);
                }
                if (Location == PrevLocation && Rotation == PrevRotation && Scale == PrevScale)
                {
                    
                    ImGui::EndTabItem();
                    ImGui::EndTabBar();
                    ImGui::PopStyleColor(3);
                    ImGui::End();
                    return;
                }
                else
                {
                    PrevLocation = Location; PrevRotation = Rotation; PrevScale = Scale;
                    USkeletalSubEngine* SkeletalSubEngine = Cast<USkeletalSubEngine>(GEngineLoop.SkeletalViewerSubEngine);
                    SkeletalSubEngine->SkeletalMeshActor->BoneGizmoSceneComponents[SelectedBone]->SetRelativeLocation(Location);
                    SkeletalSubEngine->SkeletalMeshActor->BoneGizmoSceneComponents[SelectedBone]->SetRelativeRotation(Rotation);
                    SkeletalSubEngine->SkeletalMeshActor->BoneGizmoSceneComponents[SelectedBone]->SetRelativeScale3D(Scale);
                    USkeletalSubEngine* AnimationSubEngine = Cast<USkeletalSubEngine>(GEngineLoop.AnimationViewerSubEngine);
                    AnimationSubEngine->SkeletalMeshActor->BoneGizmoSceneComponents[SelectedBone]->SetRelativeLocation(Location);
                    AnimationSubEngine->SkeletalMeshActor->BoneGizmoSceneComponents[SelectedBone]->SetRelativeRotation(Rotation);
                    AnimationSubEngine->SkeletalMeshActor->BoneGizmoSceneComponents[SelectedBone]->SetRelativeScale3D(Scale);
                }
                FMatrix NewLocalMatrix =
                    FMatrix::GetScaleMatrix(Scale) *
                    FMatrix::GetRotationMatrix(Rotation) *
                    FMatrix::GetTranslationMatrix(Location);
                if (InSkeletalMesh->SetBoneLocalMatrix(SelectedBone, NewLocalMatrix))
                {
                    InSkeletalMesh->UpdateWorldTransforms();
                    InSkeletalMesh->UpdateAndApplySkinning();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::PopStyleColor(3);
        ImGui::End();
    }
}

void SkeletalDetailPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}
