#pragma once

#include "imgui.h"

// 베지어 관련 함수 선언
namespace ImGui
{
    // 베지어 곡선의 포인트 테이블 생성
    template<int steps>
    void bezier_table(ImVec2 P[4], ImVec2 results[steps + 1]);

    // 베지어 곡선의 y값 반환
    float BezierValue(float dt01, float P[4]);

    // 베지어 에디터 위젯
    int Bezier(const char* label, float P[4]);
}
