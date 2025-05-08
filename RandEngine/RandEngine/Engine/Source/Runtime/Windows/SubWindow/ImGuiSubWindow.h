#pragma once
#include <d3d11.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

class FImGuiSubWindow
{
public:
    HWND WindowHandle = nullptr;
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;

    ImGuiContext* Context = nullptr;

    FImGuiSubWindow(HWND InWnd, ID3D11Device* InDevice, ID3D11DeviceContext* InContext)
        : WindowHandle(InWnd), Device(InDevice), DeviceContext(InContext)
    {
        // 1. Context 생성
        Context = ImGui::CreateContext();
        ImGui::SetCurrentContext(Context);

        // 2. 폰트 설정
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\malgun.ttf)", 18.0f, nullptr, io.Fonts->GetGlyphRangesKorean());

        // 3. 백엔드 초기화
        ImGui_ImplWin32_Init(WindowHandle);
        ImGui_ImplDX11_Init(Device, DeviceContext);
    }

    void BeginFrame() const
    {
        ImGui::SetCurrentContext(Context);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void EndFrame()
    {
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void Shutdown() const
    {
        ImGui::SetCurrentContext(Context);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(Context);
    }
};
