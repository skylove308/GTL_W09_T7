#pragma once

#include "Core/HAL/PlatformType.h"
#include "UObject/NameTypes.h" // For FName
#include "Container/Map.h"     // For TMap
#include "Stats.h"             // For TStatId

class FProfilerStatsManager
{
public:
    // Call at the beginning of each frame to clear previous results
    static void BeginFrame()
    {
        CPUStatsMS.Empty();
    }

    // Called by FScopeCycleCounter to record CPU time
    static void AddCpuStat(const TStatId& StatId, const double TimeMs)
    {
        CPUStatsMS.FindOrAdd(StatId.GetName()) = TimeMs;
    }

    // Retrieve CPU time for a given StatId
    static double GetCpuStatMs(const FName& StatName)
    {
        const double* FoundMs = CPUStatsMS.Find(StatName);
        return FoundMs ? *FoundMs : -1.0; // Return -1 if not found
    }

private:
    // Map from Stat Name to elapsed time in milliseconds for the current frame
    static TMap<FName, double> CPUStatsMS;
};
