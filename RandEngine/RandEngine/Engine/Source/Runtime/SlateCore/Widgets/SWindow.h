#pragma once

#include "Define.h"

class SWindow
{
public:
    SWindow() = default;
    SWindow(FRect InRect);
    virtual ~SWindow() = default;

    virtual void Initialize(FRect InitRect);
    virtual void OnResize(uint32 InWidth, uint32 InHeight);

    void SetRect(FRect NewRect) { Rect = NewRect; }

    FRect GetRect() const { return Rect; }
    
    virtual bool IsHover(const FPoint& InPoint);
    
    virtual bool OnPressed(const FPoint& InPoint);
    
    virtual bool OnReleased();
    
    bool IsPressed() const { return bIsPressed; }

protected:
    bool bIsHovered = false;
    bool bIsPressed = false;

    FRect Rect;
};

