#include "KeyHandle.h"

#include <atomic>


// Always open around this CTOR as we always increment this atomic. If we abort we simply will just grab a new index when trying again
FKeyHandle::FKeyHandle()
{
    static std::atomic<uint32> LastKeyHandleIndex = 1;
    Index = ++LastKeyHandleIndex;

    if (Index == 0)
    {
        // If we are cooking, allow wrap-around
        //if (IsRunningCookCommandlet())
        {
            // Skip indices until it's not 0 anymore as we can't 
            // assign without loss of thread-safety.
            while (Index == 0)
            {
                Index = ++LastKeyHandleIndex;
            }
        }
    }
}

FKeyHandle::FKeyHandle(uint32 SpecificIndex)
    : Index(SpecificIndex)
{}

FKeyHandle FKeyHandle::Invalid()
{
    return FKeyHandle(0);
}
