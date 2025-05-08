#pragma once

#include "imgui.h"

namespace tween {
    enum TYPE {
        LINEAR,
        QUADIN, QUADOUT, QUADINOUT,
        CUBICIN, CUBICOUT, CUBICINOUT,
        QUARTIN, QUARTOUT, QUARTINOUT,
        QUINTIN, QUINTOUT, QUINTINOUT,
        SINEIN, SINEOUT, SINEINOUT,
        EXPOIN, EXPOOUT, EXPOINOUT,
        CIRCIN, CIRCOUT, CIRCINOUT,
        ELASTICIN, ELASTICOUT, ELASTICINOUT,
        BACKIN, BACKOUT, BACKINOUT,
        BOUNCEIN, BOUNCEOUT, BOUNCEINOUT,
        SINESQUARE, EXPONENTIAL,
        SCHUBRING1, SCHUBRING2, SCHUBRING3,
        SINPI2, SWING
    };

    double ease(int easetype, double t);
}

namespace ImGui
{
    // 미니 스플라인 보간
    void spline(const float* key, int num, int dim, float t, float* v);

    // 곡선값 계산 (선형 보간)
    float CurveValue(float p, int maxpoints, const ImVec2* points);

    // 곡선값 계산 (스무스 보간)
    float CurveValueSmooth(float p, int maxpoints, const ImVec2* points);

    // 곡선 에디터 위젯
    int Curve(const char* label, const ImVec2& size, int maxpoints, ImVec2* points);
}
