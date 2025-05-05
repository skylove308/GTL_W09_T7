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
    ID3D11RenderTargetView* RTV = nullptr;
    ID3D11Texture2D* RenderTargetTexture = nullptr;

    int Width = 640;
    int Height = 480;

    FImGuiSubWindow(HWND InWnd, ID3D11Device* InDevice, ID3D11DeviceContext* InContext)
        : WindowHandle(InWnd), Device(InDevice), DeviceContext(InContext)
    {
        // 1. Context 생성
        Context = ImGui::CreateContext();

        // 2. 백엔드 초기화
        ImGui::SetCurrentContext(Context);
        ImGui_ImplWin32_Init(WindowHandle);
        ImGui_ImplDX11_Init(Device, DeviceContext);
        //
        // // 3. 기본 RenderTarget 생성
        CreateRenderTarget();
    }

    void CreateRenderTarget()
    {
        if (RenderTargetTexture)
        {
            RenderTargetTexture->Release();
            RTV->Release();
        }

        D3D11_TEXTURE2D_DESC Desc = {};
        Desc.Width = Width;
        Desc.Height = Height;
        Desc.MipLevels = 1;
        Desc.ArraySize = 1;
        Desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        Desc.SampleDesc.Count = 1;
        Desc.Usage = D3D11_USAGE_DEFAULT;
        Desc.BindFlags = D3D11_BIND_RENDER_TARGET;

        HRESULT hr = Device->CreateTexture2D(&Desc, nullptr, &RenderTargetTexture);
        if (FAILED(hr))
        {
            std::cout << "Failed to create render target texture" << std::endl;
            std::cout << "HRESULT: " << hr << std::endl;
        }
        Device->CreateRenderTargetView(RenderTargetTexture, nullptr, &RTV);
    }

    void BeginFrame()
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

        constexpr float ClearColor[4] = { 1.0f, 0.1f, 0.1f, 1.0f };
        DeviceContext->OMSetRenderTargets(1, &RTV, nullptr);
        DeviceContext->ClearRenderTargetView(RTV, ClearColor);
    }

    void Shutdown()
    {
        ImGui::SetCurrentContext(Context);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(Context);
        if (RTV) RTV->Release();
        if (RenderTargetTexture) RenderTargetTexture->Release();
    }
};
