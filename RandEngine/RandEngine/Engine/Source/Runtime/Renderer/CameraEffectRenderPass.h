#pragma once
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/DXDBufferManager.h"

class FCameraEffectRenderPass
{
public:
    FCameraEffectRenderPass();
    ~FCameraEffectRenderPass();

    void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager);
    void ReleaseShader();
    void CreateShader();
    void UpdateShader();
    void CreateBlendState();
    void CreateSampler();
    void PrepareRenderState();

    void Render(const std::shared_ptr<FEditorViewportClient>& Viewport);

private:
    FGraphicsDevice* Graphics = nullptr;
    FDXDBufferManager* BufferManager = nullptr;
    FDXDShaderManager* ShaderManager = nullptr;

    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    ID3D11BlendState* BlendState = nullptr;
    ID3D11SamplerState* Sampler = nullptr;
};
