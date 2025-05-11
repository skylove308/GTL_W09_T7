#pragma once
#include <d3d11.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"


#include "Font/RawFonts.h"
#include "Font/IconDefs.h"

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

        ImGui_ImplWin32_Init(WindowHandle);
        ImGui_ImplDX11_Init(Device, DeviceContext);

        // 2. 폰트 설정

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\malgun.ttf)", 18.0f, nullptr, io.Fonts->GetGlyphRangesKorean());
        unsigned char* pixels;
        int w, h;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h); // 여기서 atlas 생성됨

        ImFontConfig FeatherFontConfig;
        FeatherFontConfig.PixelSnapH = true;
        FeatherFontConfig.FontDataOwnedByAtlas = false;
        FeatherFontConfig.GlyphOffset = ImVec2(0, 0);
        static constexpr ImWchar IconRanges[] = {
            ICON_MOVE,      ICON_MOVE + 1,
            ICON_ROTATE,    ICON_ROTATE + 1,
            ICON_SCALE,     ICON_SCALE + 1,
            ICON_MONITOR,   ICON_MONITOR + 1,
            ICON_BAR_GRAPH, ICON_BAR_GRAPH + 1,
            ICON_NEW,       ICON_NEW + 1,
            ICON_SAVE,      ICON_SAVE + 1,
            ICON_LOAD,      ICON_LOAD + 1,
            ICON_MENU,      ICON_MENU + 1,
            ICON_SLIDER,    ICON_SLIDER + 1,
            ICON_PLUS,      ICON_PLUS + 1,
            ICON_PLAY,      ICON_PLAY + 1,
            ICON_STOP,      ICON_STOP + 1,
            ICON_SQUARE,    ICON_SQUARE + 1,
             ICON_PAUSE,     ICON_PAUSE + 1,
            0 };

        io.Fonts->AddFontFromMemoryTTF(FeatherRawData, FontSizeOfFeather, 22.0f, &FeatherFontConfig, IconRanges);

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
