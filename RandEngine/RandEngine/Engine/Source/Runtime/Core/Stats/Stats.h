#pragma once
#include "StatDefine.h"
#include "Container/String.h"
#include "HAL/PlatformType.h"
#include "UObject/NameTypes.h"

class FGPUTimingManager; // Forward declaration

class FScopeCycleCounter
{
public:
    FScopeCycleCounter(TStatId StatId);
    ~FScopeCycleCounter();

    // 이동 & 복사 생성자 제거
    FScopeCycleCounter(const FScopeCycleCounter&) = delete;
    FScopeCycleCounter& operator=(const FScopeCycleCounter&) = delete;
    FScopeCycleCounter(FScopeCycleCounter&&) = delete;
    FScopeCycleCounter& operator=(FScopeCycleCounter&&) = delete;

    uint64 Finish();

private:
    uint64 StartCycles;

    [[maybe_unused]]
    TStatId UsedStatId;
};

#define QUICK_SCOPE_CYCLE_COUNTER(Stat) \
    static TStatId FStat_##Stat(TEXT(#Stat)); \
    FScopeCycleCounter CycleCount_##Stat(FStat_##Stat);

// RAII class for timing GPU operations using FGpuTimingManager.
// Constructor calls StartTimestamp, Destructor calls StopTimestamp.
class FGPUScopeCycleCounter
{
public:
    // Constructor: Starts the GPU timestamp. Requires the manager instance.
    FGPUScopeCycleCounter(const TStatId& StatId, FGPUTimingManager& GPUTimingManager);

    // Destructor: Stops the GPU timestamp.
    ~FGPUScopeCycleCounter();

    // Non-copyable/movable
    FGPUScopeCycleCounter(const FGPUScopeCycleCounter&) = delete;
    FGPUScopeCycleCounter& operator=(const FGPUScopeCycleCounter&) = delete;
    FGPUScopeCycleCounter(FGPUScopeCycleCounter&&) = delete;
    FGPUScopeCycleCounter& operator=(FGPUScopeCycleCounter&&) = delete;

private:
    FGPUTimingManager& GPUTimingManager;
    TStatId StatId;
    bool bStarted; // Track if Start was successfully called
};

// Macro similar to QUICK_SCOPE_CYCLE_COUNTER for convenience
// Requires a pointer or reference to your FGpuTimingManager instance (e.g., gGpuTimingManager)
#define QUICK_GPU_SCOPE_CYCLE_COUNTER(Stat, GPUTimerMgr) \
    static TStatId FStat_GPU_##Stat(TEXT(#Stat));        \
    FGPUScopeCycleCounter GpuCycleCount_##Stat(FStat_GPU_##Stat, GPUTimerMgr);
