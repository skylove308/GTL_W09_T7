#pragma once
#include "GizmoBaseComponent.h"


class UGizmoRectangleComponent : public UGizmoBaseComponent
{
    DECLARE_CLASS(UGizmoRectangleComponent, UGizmoBaseComponent)

public:
    UGizmoRectangleComponent() = default;

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
};
