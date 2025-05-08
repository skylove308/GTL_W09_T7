#include "CameraEffectRenderPass.h"
#include "Engine/Engine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "Engine/World/World.h"
#include "Classes/Camera/PlayerCameraManager.h"
#include "Renderer/ShaderConstants.h"
#include "RendererHelpers.h"
#include "UnrealClient.h"

FCameraEffectRenderPass::FCameraEffectRenderPass() {}
FCameraEffectRenderPass::~FCameraEffectRenderPass() { ReleaseShader(); }

void FCameraEffectRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    ShaderManager = InShaderManager;
    CreateShader();
    CreateBlendState();
    CreateSampler();
}

void FCameraEffectRenderPass::CreateShader()
{
    ShaderManager->AddVertexShader(L"CameraEffectVertexShader", L"Shaders/CameraEffectShader.hlsl", "mainVS");
    ShaderManager->AddPixelShader(L"CameraEffectPixelShader", L"Shaders/CameraEffectShader.hlsl", "mainPS");
    VertexShader = ShaderManager->GetVertexShaderByKey(L"CameraEffectVertexShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"CameraEffectPixelShader");
    BufferManager->CreateBufferGeneric<FConstantBufferCameraFade>("CameraFadeConstantBuffer", nullptr, sizeof(FConstantBufferCameraFade), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    BufferManager->CreateBufferGeneric<FConstantBufferCameraVignette>("CameraVignetteConstantBuffer", nullptr, sizeof(FConstantBufferCameraVignette), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    BufferManager->CreateBufferGeneric<FConstantBufferLetterBox>("LetterBoxConstantBuffer", nullptr, sizeof(FConstantBufferLetterBox), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void FCameraEffectRenderPass::UpdateShader()
{
    VertexShader = ShaderManager->GetVertexShaderByKey(L"CameraEffectVertexShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"CameraEffectPixelShader");
}

void FCameraEffectRenderPass::ReleaseShader() {}

void FCameraEffectRenderPass::CreateBlendState()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Graphics->Device->CreateBlendState(&blendDesc, &BlendState);
}

void FCameraEffectRenderPass::CreateSampler()
{
    D3D11_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SamplerDesc.MinLOD = 0;
    SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    Graphics->Device->CreateSamplerState(&SamplerDesc, &Sampler);
}

void FCameraEffectRenderPass::PrepareRenderState()
{
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    Graphics->DeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    Graphics->DeviceContext->IASetInputLayout(nullptr);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);

    Graphics->DeviceContext->PSSetSamplers(0, 1, &Sampler);
}

void FCameraEffectRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const EResourceType ResourceType = EResourceType::ERT_PP_CameraEffect;
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);

    FConstantBufferCameraFade FadeParams;
    FConstantBufferCameraVignette VignetteParams;
    FConstantBufferLetterBox LetterBoxParams;

    BufferManager->BindConstantBuffer("CameraFadeConstantBuffer", 0, EShaderStage::Pixel);
    BufferManager->BindConstantBuffer("CameraVignetteConstantBuffer", 1, EShaderStage::Pixel);
    BufferManager->BindConstantBuffer("LetterBoxConstantBuffer", 2, EShaderStage::Pixel);

    if (GEngine->ActiveWorld->GetPlayerController() && GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager) {
        FadeParams.FadeColor = GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->FadeColor;
        FadeParams.FadeAmount = GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->FadeAmount;
        VignetteParams.VignetteColor = GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->VignetteColor;
        VignetteParams.VignetteCenter = GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->VignetteCenter;
        VignetteParams.VignetteRadius = GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->VignetteRadius;
        VignetteParams.VignetteSmoothness = GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->VignetteSmoothness;
        VignetteParams.VignetteIntensity = GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->VignetteIntensity;
        LetterBoxParams.LetterBoxColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
        LetterBoxParams.ScreenAspectRatio = Viewport->AspectRatio;
        LetterBoxParams.LetterBoxAspectRatio = GEngine->ActiveWorld->GetPlayerController()->PlayerCameraManager->GetLetterBoxRatio();
    }
    else
    {
        FadeParams.FadeColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
        FadeParams.FadeAmount = 0.0f;
        VignetteParams.VignetteColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
        VignetteParams.VignetteCenter = FVector2D(0.5, 0.5);
        VignetteParams.VignetteRadius = 0.5;
        VignetteParams.VignetteSmoothness = 0.1;
        VignetteParams.VignetteIntensity = 0.0f;
        LetterBoxParams.LetterBoxColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
        LetterBoxParams.ScreenAspectRatio = Viewport->AspectRatio;
        LetterBoxParams.LetterBoxAspectRatio = Viewport->AspectRatio;
    }

    BufferManager->UpdateConstantBuffer<FConstantBufferCameraFade>("CameraFadeConstantBuffer", FadeParams);
    BufferManager->UpdateConstantBuffer<FConstantBufferCameraVignette>("CameraVignetteConstantBuffer", VignetteParams);
    BufferManager->UpdateConstantBuffer<FConstantBufferLetterBox>("LetterBoxConstantBuffer", LetterBoxParams);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, nullptr);
    Graphics->DeviceContext->OMSetBlendState(BlendState, nullptr, 0xffffffff);

    UpdateShader();
    PrepareRenderState();

    Graphics->DeviceContext->Draw(6, 0);

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}


