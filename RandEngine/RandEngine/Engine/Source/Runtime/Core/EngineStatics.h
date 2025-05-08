#pragma once
#include <atomic>
#include "HAL/PlatformType.h"


class UEngineStatics
{
    static std::atomic<uint32> NextUUID;

public:
    static uint32 GenUUID()
    {
        return NextUUID.fetch_add(1, std::memory_order_relaxed);
    }
};
