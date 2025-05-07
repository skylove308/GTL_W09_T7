#pragma once
#include "Math/Matrix.h"
#include "Math/Vector4.h"

class USkeletalMesh;
class FSubCamera;
class FDXDShaderManager;
class FStaticMeshRenderPass;
class FDXDBufferManager;
class FGraphicsDevice;

class FSubRenderer
{
public:
    /** Initialize */
    FSubRenderer() = default;
    ~FSubRenderer();
    
    void Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager);
    void Release();

    /** Render */
    void PrepareRender(const FSubCamera& Camera) const;
    void Render() const;
    void ClearRender() const;

    void UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const;

    void UpdateLightConstant() const;

    /** Update Buffer */
    void UpdateViewCamera(const FSubCamera& Camera) const;

    /** Set */
    void SetPreviewSkeletalMesh(USkeletalMesh* InPreviewSkeletalMesh);

private:
    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    FDXDShaderManager* ShaderManager = nullptr;
    
    FStaticMeshRenderPass* StaticMeshRenderPass = nullptr;

    USkeletalMesh* PreviewSkeletalMesh = nullptr;
};
