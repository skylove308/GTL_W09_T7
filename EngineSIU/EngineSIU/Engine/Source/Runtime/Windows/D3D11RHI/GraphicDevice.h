#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define _TCHAR_DEFINED
#include <d3d11.h>

#include "EngineBaseTypes.h"

#include "Core/HAL/PlatformType.h"
#include "Core/Math/Vector4.h"

class FEditorViewportClient;

class FGraphicsDevice
{
public:
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    
    IDXGISwapChain* SwapChain = nullptr;
    
    ID3D11Texture2D* BackBufferTexture = nullptr;
    ID3D11RenderTargetView* BackBufferRTV = nullptr;

    ID3D11Texture2D* DeviceDSVTexture = nullptr;
    ID3D11DepthStencilView* DeviceDSV = nullptr;
    
    ID3D11RasterizerState* RasterizerSolidBack = nullptr;
    ID3D11RasterizerState* RasterizerSolidFront = nullptr;
    ID3D11RasterizerState* RasterizerWireframeBack = nullptr;
    ID3D11RasterizerState* RasterizerShadow = nullptr;

    ID3D11DepthStencilState* DepthStencilState = nullptr;
    
    ID3D11BlendState* AlphaBlendState = nullptr;
    
    DXGI_SWAP_CHAIN_DESC SwapchainDesc;
    
    UINT ScreenWidth = 0;
    UINT ScreenHeight = 0;

    D3D11_VIEWPORT Viewport;
    D3D11_VIEWPORT RenderViewport;
    
    FLOAT ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 화면을 초기화(clear) 할 때 사용할 색상(RGBA)

    void Initialize(HWND hWindow);
    void Initialize(HWND hWindow, ID3D11Device* InDevice);
    
    void ChangeRasterizer(EViewModeIndex ViewModeIndex);
    void CreateRTV(ID3D11Texture2D*& OutTexture, ID3D11RenderTargetView*& OutRTV);
    ID3D11Texture2D* CreateTexture2D(const D3D11_TEXTURE2D_DESC& Description, const void* InitialData);
    
    void Release();
    
    void Prepare();

    void SwapBuffer() const;
    
    void Resize(HWND hWindow);

    void Resize(HWND hWnd, float Width, float Height);
    
    ID3D11RasterizerState* GetCurrentRasterizer() const { return CurrentRasterizer; }

    /*
    uint32 GetPixelUUID(POINT pt) const;
    uint32 DecodeUUIDColor(FVector4 UUIDColor) const;
    */
    
private:
    void CreateDeviceAndSwapChain(HWND hWindow);
    void CreateSwapChain(HWND hWnd);
    void CreateBackBuffer();
    void CreateDepthStencilState();
    void CreateRasterizerState();
    void CreateAlphaBlendState();

    void CreateDepthStencilViewAndTexture();
    
    void ReleaseDeviceAndSwapChain();
    void ReleaseFrameBuffer();
    void ReleaseRasterizerState();
    void ReleaseDepthStencilResources();
    
    ID3D11RasterizerState* CurrentRasterizer = nullptr;

    const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT BackBufferRTVFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
};

