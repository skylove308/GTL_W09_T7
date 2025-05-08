#include "CameraModifier.h"

#include "PlayerCameraManager.h"

UCameraModifier::UCameraModifier()
{
}

bool UCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    UpdateAlpha(DeltaTime);

    /* 예제
        if (Alpha > 0.0f)
        {
            InOutPOV.FOV -= 20.f * Alpha; // 알파 비율로 줌 효과 주기
	    }
     *
     */

    return false;
}


UWorld* UCameraModifier::GetWorld() const
{
    return CameraOwner ? CameraOwner->GetWorld() : nullptr;
}

void UCameraModifier::EnableModifier()
{
    bDisabled = false;
    bPendingDisable = false;
}

void UCameraModifier::DisableModifier(bool bImmediate)
{
    if (bImmediate)
    {
        bDisabled = true;
        bPendingDisable = false;
    }
    else if (!bDisabled)
    {
        bPendingDisable = true;
    }
}

float UCameraModifier::GetTargetAlpha()
{
    return bPendingDisable ? 0.0f : 1.f;
}


void UCameraModifier::UpdateAlpha(float DeltaTime)
{
    float const TargetAlpha = GetTargetAlpha();
    float const BlendTime = (TargetAlpha == 0.f) ? AlphaOutTime : AlphaInTime;

    // 보간
    if (BlendTime <= 0.f)
    {
        Alpha = TargetAlpha;
    }
    else if (Alpha > TargetAlpha)
    {
        Alpha = FMath::Max<float>(Alpha - DeltaTime / BlendTime, TargetAlpha);
    }
    else
    {
        Alpha = FMath::Min<float>(Alpha + DeltaTime / BlendTime, TargetAlpha);
    }
}
