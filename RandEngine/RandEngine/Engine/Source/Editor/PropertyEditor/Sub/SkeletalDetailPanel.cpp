#include "SkeletalDetailPanel.h"

#include "Animation/Skeleton.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Math/JungleMath.h"
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

        ImGui::Begin("Details", nullptr, PanelFlags);

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
            ImGui::End();
            return;
        }
        else
            PrevLocation = Location; PrevRotation = Rotation; PrevScale = Scale;
        
        FMatrix NewLocalMatrix =
                    FMatrix::GetScaleMatrix(Scale) *
                    FMatrix::GetRotationMatrix(Rotation) *
                    FMatrix::GetTranslationMatrix(Location);
        if (InSkeletalMesh->SetBoneLocalMatrix(SelectedBone, NewLocalMatrix))
        {
            InSkeletalMesh->UpdateWorldTransforms();
            InSkeletalMesh->UpdateAndApplySkinning();
        } 
        ImGui::End();
}

void SkeletalDetailPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}
