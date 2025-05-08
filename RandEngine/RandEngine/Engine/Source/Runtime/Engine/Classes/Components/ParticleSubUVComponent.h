#pragma once
#define _TCHAR_DEFINED
#include <wrl.h>
#include "BillboardComponent.h"

// ParticleSubUVComponent: 서브UV 파티클 컴포넌트 (Billboard 컴포넌트를 상속)
class UParticleSubUVComponent : public UBillboardComponent
{
    DECLARE_CLASS(UParticleSubUVComponent, UBillboardComponent)

public:
    UParticleSubUVComponent();

    virtual UObject* Duplicate(UObject* InOuter) override;
    
    void GetProperties(TMap<FString, FString>& OutProperties) const override;
    void SetProperties(const TMap<FString, FString>& InProperties) override;
    
    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;

    // 텍스처 아틀라스의 셀 수 설정 (행, 열)
    void SetRowColumnCount(int cellsPerRow, int cellsPerColumn);
   
    virtual void SetTexture(const FWString& _fileName) override;
    
    FVector2D GetUVOffset() const { return UVOffset; }
    FVector2D GetUVScale() const { return UVScale; }

protected:

    FVector2D UVScale;
    FVector2D UVOffset;

    // 애니메이션 반복 여부 (Loop)
    bool bIsLoop = true;

    // 현재 애니메이션 프레임 (열, 행 인덱스)
    int indexU = 0;
    int indexV = 0;

    // 누적 시간 (프레임 전환을 위한)
    float elapsedTime = 0.0f;
    // 프레임 당 지속 시간 (밀리초 단위, 필요에 따라 조정)
    float FrameDuration = 75.0f;

    // 텍스처 아틀라스의 셀 수 (행, 열)
    int CellsPerRow = 1;
    int CellsPerColumn = 1;

};
