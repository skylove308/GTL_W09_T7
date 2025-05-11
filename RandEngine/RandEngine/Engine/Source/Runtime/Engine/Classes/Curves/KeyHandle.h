#pragma once
#include "Container/Array.h"
#include "Container/Map.h"
#include "HAL/PlatformType.h"

class FKeyHandle
{
public:
    FKeyHandle();

    static FKeyHandle Invalid();

private:
    /** Private constructor from a specific index - only for use in FKeyHandle::Invalid to avoid allocating new handles unnecessarily */
    explicit FKeyHandle(uint32 SpecificIndex);

    bool operator ==(const FKeyHandle& Other) const
    {
        return Index == Other.Index;
    }

    bool operator !=(const FKeyHandle& Other) const
    {
        return Index != Other.Index;
    }

    friend bool operator<(FKeyHandle A, FKeyHandle B)
    {
        return A.Index < B.Index;
    }

    friend bool operator>(FKeyHandle A, FKeyHandle B)
    {
        return A.Index > B.Index;
    }
    
    uint32 Index;
};


struct FKeyHandleMap
{
public:
    FKeyHandleMap() {}

    // This struct is not copyable.  This must be public or because derived classes are allowed to be copied
    FKeyHandleMap( const FKeyHandleMap& Other ) {}
    // void operator=(const FKeyHandleMap& Other) {}

    // Quickly initializes this map by clearing it and filling with KeyHandles in O(n) time, instead of O(n^2) if Add() was used in a loop.
    //void Initialize(TArrayView<const FKeyHandle> InKeyHandles);

    /** TMap functionality */
    // void Add( const FKeyHandle& InHandle, int32 InIndex );
    // void Empty(int32 ExpectedNumElements = 0);
    // void Remove( const FKeyHandle& InHandle );
    // void Reserve(int32 NumElements);
    //const int32* Find(const FKeyHandle& InHandle) const { return KeyHandlesToIndices.Find(InHandle); }
    // const FKeyHandle* FindKey( int32 KeyIndex ) const;
    //int32 Num() const { return KeyHandlesToIndices.Num(); }
    //const TMap<FKeyHandle, int32>& GetMap() const { return KeyHandlesToIndices; }

    // void SetKeyHandles(int32 Num);

    /** ICPPStructOps implementation */
    // bool Serialize(FArchive& Ar);
    // bool operator==(const FKeyHandleMap& Other) const { return KeyHandles == Other.KeyHandles; }
    // bool operator!=(const FKeyHandleMap& Other) const { return !(*this==Other); }

    // friend FArchive& operator<<(FArchive& Ar,FKeyHandleMap& P)
    // {
    //     P.Serialize(Ar);
    //     return Ar;
    // }
    //
    // // Ensures that all indices have a valid handle and that there are no handles left to invalid indices
    // void EnsureAllIndicesHaveHandles(int32 NumIndices);
	   //
    // // Ensures a handle exists for the specified Index
    // void EnsureIndexHasAHandle(int32 KeyIndex);

private:

    //TMap<FKeyHandle, int32> KeyHandlesToIndices;
    TArray<FKeyHandle> KeyHandles;
};

