#pragma once

#define _TCHAR_DEFINED
#include <d3d11.h>
#include <wrl/client.h> // For Microsoft::WRL::ComPtr

#include "StatDefine.h"
#include "Container/Array.h"
#include "Container/Map.h"

// Structure to hold queries for one frame's measurement slot
struct FFrameQueries
{
    Microsoft::WRL::ComPtr<ID3D11Query> DisjointQuery;
    Microsoft::WRL::ComPtr<ID3D11Query> StartQuery;
    Microsoft::WRL::ComPtr<ID3D11Query> EndQuery;
    TStatId StatId; // Which measurement this slot was used for
    bool bQueryIssued; // Was StartTimestamp called for this slot in its frame?
};

// Structure to store the final timing result
struct FTimingResult
{
    double ElapsedTimeMs = -3.0; // Initial state
    uint64 LastUpdateFrame = 0; // Frame number when this was last updated
};

// Manages DirectX 11 timestamp queries for GPU profiling.
// Handles query creation, buffering, and asynchronous result retrieval.
class FGPUTimingManager
{
public:
    FGPUTimingManager();
    ~FGPUTimingManager() = default;

    // Non-copyable/movable due to ComPtrs and singleton-like usage pattern
    FGPUTimingManager(const FGPUTimingManager&) = delete;
    FGPUTimingManager& operator=(const FGPUTimingManager&) = delete;
    FGPUTimingManager(FGPUTimingManager&&) = delete;
    FGPUTimingManager& operator=(FGPUTimingManager&&) = delete;

    // Initializes the manager with the D3D device and context.
    // numBufferedFrames should be >= 2, typically 3.
    bool Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, uint32 InNumBufferedFrames = 3);

    // Call at the very beginning of each frame.
    // Retrieves results from previous frames and starts the disjoint query for the current frame.
    void BeginFrame();

    // Called by FGpuScopeCycleCounter constructor. Issues start timestamp.
    void StartTimestamp(const TStatId& StatId);

    // Called by FGpuScopeCycleCounter destructor. Issues end timestamp.
    void StopTimestamp(const TStatId& StatId);

    // Call at the very end of each frame (before Present).
    // Ends the disjoint query for the current frame.
    void EndFrame();

    // Returns the last measured time in milliseconds for a given StatId.
    // Returns a negative value if no data is available yet or an error occurred.
    // Note: This data is typically a few frames old due to GPU latency.
    double GetElapsedTimeMs(const TStatId& StatId) const;

    bool IsInitialized() const { return bInitialized; }

private:
    void RetrieveResults(); // Internal helper to get data from GetData

    bool bInitialized = false;
    uint32 NumBufferedFrames = 0;
    uint32 CurrentFrameIndex = 0; // Index for issuing commands
    uint32 ResultsFrameIndex = 0; // Index for retrieving results

    ID3D11Device* Device = nullptr; // Non-owning pointer
    ID3D11DeviceContext* Context = nullptr; // Non-owning pointer

    TArray<FFrameQueries> FrameQueries;

    // Map to store the latest timing result per StatId
    TMap<FName, FTimingResult> LatestResults;

    // Track the next available query pair index within the current frame
    // Note: This simple index assumes non-nested scopes or only one scope active at a time.
    // For nested scopes, a stack of active queries would be needed.
    uint32 CurrentQueryIndexInFrame = 0;
    const uint32 MAX_QUERIES_PER_FRAME = 16; // Example limit

    uint64 FrameCounter = 0; // To track result freshness
};
