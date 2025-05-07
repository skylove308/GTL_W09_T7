#include "SubRenderer.h"

#include "RendererHelpers.h"
#include "StaticMeshRenderPass.h"
#include "Components/Mesh/SkeletalMeshRenderData.h"
#include "Windows/SubWindow/SubCamera.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/Asset/SkeletalMeshAsset.h"

FSubRenderer::~FSubRenderer()
{
    Release();
}

void FSubRenderer::Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;

    ShaderManager = new FDXDShaderManager(Graphics->Device);

    StaticMeshRenderPass = new FStaticMeshRenderPass();
    StaticMeshRenderPass->Initialize(BufferManager, Graphics, ShaderManager);

    UINT MaterialBufferSize = sizeof(FMaterialConstants);
    BufferManager->CreateBufferGeneric<FMaterialConstants>("FMaterialConstants", nullptr, MaterialBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT LitUnlitBufferSize = sizeof(FLitUnlitConstants);
    BufferManager->CreateBufferGeneric<FLitUnlitConstants>("FLitUnlitConstants", nullptr, LitUnlitBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    
    UINT LightInfoBufferSize = sizeof(FLightInfoBuffer);
    BufferManager->CreateBufferGeneric<FLightInfoBuffer>("FLightInfoBuffer", nullptr, LightInfoBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    
    UINT ObjectBufferSize = sizeof(FObjectConstantBuffer);
    BufferManager->CreateBufferGeneric<FObjectConstantBuffer>("FObjectConstantBuffer", nullptr, ObjectBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT CameraConstantBufferSize = sizeof(FCameraConstantBuffer);
    BufferManager->CreateBufferGeneric<FCameraConstantBuffer>("FCameraConstantBuffer", nullptr, CameraConstantBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    ID3D11Buffer* ObjectBuffer = BufferManager->GetConstantBuffer(TEXT("FObjectConstantBuffer"));
    ID3D11Buffer* CameraConstantBuffer = BufferManager->GetConstantBuffer(TEXT("FCameraConstantBuffer"));
    Graphics->DeviceContext->VSSetConstantBuffers(12, 1, &ObjectBuffer);
    Graphics->DeviceContext->VSSetConstantBuffers(13, 1, &CameraConstantBuffer);
    Graphics->DeviceContext->PSSetConstantBuffers(12, 1, &ObjectBuffer);
    Graphics->DeviceContext->PSSetConstantBuffers(13, 1, &CameraConstantBuffer);
    
    D3D11_INPUT_ELEMENT_DESC StaticMeshLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"MATERIAL_INDEX", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"StaticMeshVertexShader", L"Shaders/StaticMeshVertexShader.hlsl", "mainVS", StaticMeshLayoutDesc, ARRAYSIZE(StaticMeshLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }

    hr = ShaderManager->AddPixelShader(L"StaticMeshPixelShader", L"Shaders/StaticMeshPixelShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        // TODO: 적절한 오류 처리
        return;
    }
}

void FSubRenderer::Release()
{
    if (ShaderManager)
    {
        delete ShaderManager;
        ShaderManager = nullptr;
    }

    if (StaticMeshRenderPass)
    {
        delete StaticMeshRenderPass;
        StaticMeshRenderPass = nullptr;
    }
}

void FSubRenderer::PrepareRender(const FSubCamera& Camera) const
{
    UpdateViewCamera(Camera);

    // Set RTV + DSV
    Graphics->DeviceContext->OMSetRenderTargets(1, &Graphics->BackBufferRTV, Graphics->DeviceDSV);
    
    // Set Viewport
    Graphics->DeviceContext->RSSetViewports(1, &Graphics->RenderViewport);

    // Set Rasterizer + DSS
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState, 0);
    
    // Clear RenderTarget
    Graphics->DeviceContext->ClearRenderTargetView(Graphics->BackBufferRTV, Graphics->ClearColor);
}

void FSubRenderer::Render() const
{
    if (PreviewSkeletalMesh == nullptr)
    {
        return;
    }

    // 셰이더 설정
    ID3D11VertexShader* vertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
    ID3D11InputLayout* inputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader"); // VS와 함께 생성했으므로 같은 키 사용
    ID3D11PixelShader* pixelShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShader");

    if (!vertexShader || !inputLayout || !pixelShader)
    {
        return;
    }

    Graphics->DeviceContext->VSSetShader(vertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(pixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(inputLayout);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    UpdateObjectConstant(FMatrix::Identity, FVector4(), false);

    UpdateConstants();

    RenderMesh();
}

void FSubRenderer::RenderMesh() const
{
    FSkeletalMeshRenderData* RenderData = PreviewSkeletalMesh->GetRenderData();

    TArray<FStaticMaterial*> RenderMaterial = PreviewSkeletalMesh->GetMaterials();
    
    UINT Stride = sizeof(FSkeletalMeshVertex);
    UINT Offset = 0;

    FVertexInfo VertexInfo;
    BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);

    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }

    if (RenderData->MaterialSubsets.Num() == 0)
    {
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
        return;
    }

    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        uint32 MaterialIndex = RenderData->MaterialSubsets[SubMeshIndex].MaterialIndex;
        
        MaterialUtils::UpdateMaterial(BufferManager, Graphics, RenderMaterial[MaterialIndex]->Material->GetMaterialInfo());
        
        uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);
    }
}

void FSubRenderer::ClearRender() const
{
    ShaderManager->ReloadAllShaders();
}

void FSubRenderer::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;
    
    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}

void FSubRenderer::UpdateLightConstant() const
{
    BufferManager->BindConstantBuffer(TEXT("FLightInfoBuffer"), 0, EShaderStage::Vertex);
    
    FLightInfoBuffer LightBufferData = {};
    LightBufferData.DirectionalLightsCount = 0;
    LightBufferData.PointLightsCount = 0;
    LightBufferData.SpotLightsCount = 0;

    FAmbientLightInfo AmbientLightInfo;
    AmbientLightInfo.AmbientColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    
    LightBufferData.AmbientLightsCount = 1;
    LightBufferData.Ambient[0] = AmbientLightInfo;

    BufferManager->UpdateConstantBuffer(TEXT("FLightInfoBuffer"), LightBufferData);
}

void FSubRenderer::UpdateConstants() const
{
    TArray<FString> PSBufferKeys = {
        TEXT("FLightInfoBuffer"),
        TEXT("FMaterialConstants"),
        TEXT("FLitUnlitConstants")
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 0, EShaderStage::Pixel);
    
    /** Lit Flag */
    FLitUnlitConstants Data;
    Data.bIsLit = true;
    BufferManager->UpdateConstantBuffer(TEXT("FLitUnlitConstants"), Data);

    /** Light */
    UpdateLightConstant();
    
}

void FSubRenderer::UpdateViewCamera(const FSubCamera& Camera) const
{
    FCameraConstantBuffer CameraConstantBuffer;
    CameraConstantBuffer.ViewMatrix = Camera.GetViewMatrix();
    CameraConstantBuffer.InvViewMatrix = FMatrix::Inverse(CameraConstantBuffer.ViewMatrix);
    CameraConstantBuffer.ProjectionMatrix = Camera.GetProjectionMatrix();
    CameraConstantBuffer.InvProjectionMatrix = FMatrix::Inverse(CameraConstantBuffer.ProjectionMatrix);
    CameraConstantBuffer.ViewLocation = Camera.GetCameraLocation();
    CameraConstantBuffer.NearClip = Camera.GetCameraNearClip();
    CameraConstantBuffer.FarClip = Camera.GetCameraFarClip();
    BufferManager->UpdateConstantBuffer("FCameraConstantBuffer", CameraConstantBuffer);
}

void FSubRenderer::SetPreviewSkeletalMesh(USkeletalMesh* InPreviewSkeletalMesh)
{
    PreviewSkeletalMesh = InPreviewSkeletalMesh;
}
