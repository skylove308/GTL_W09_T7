#pragma once
#include "Define.h"

FVertexTexture quadVertices[] =
{
    {-1.f, 1.f, 0.f, 0.f, 0.f},
    {1.f, 1.f, 0.f, 1.f, 0.f },
    {-1.f, -1.f, 0.f, 0.f, 1.f},
    {1.f, -1.f, 0.f, 1.f, 1.f}
};
    
uint32 quadIndices[] =
{
    0,1,2,
    2,1,3
};
