#pragma once
#include "SubEngine.h"

class USkeletalSubEngine : public USubEngine
{
    DECLARE_CLASS(USkeletalSubEngine, USubEngine)
public:
    USkeletalSubEngine();
    ~USkeletalSubEngine();
public:
    virtual void Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,UnrealEd* InUnrealEd);
    virtual void Tick(float DeltaTime);
    virtual void Input(float DeltaTime);
    virtual void Render();
    virtual void Release();

    void SetSkeletalMesh(USkeletalMesh* InSkeletalMesh);    
    USkeletalMesh* SelectedSkeletalMesh;
};
