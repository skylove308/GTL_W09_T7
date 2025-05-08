#pragma once
#include "Object.h"


struct FProperty
{
    FProperty(const char* InName, int32 InSize, int32 InOffset)
        : Name(InName)
        , Size(InSize)
        , Offset(InOffset)
    {}

    virtual ~FProperty() = default;

    const char* Name;
    int64 Size;
    int64 Offset;
};


// struct FNumericProperty : public FProperty
// {
//     FNumericProperty(const char* InName, int32 InSize, int32 InOffset)
//         : FProperty(InName, InSize, InOffset)
//     {}
// };
//
// struct FInt8Property : public FNumericProperty
// {
//     FInt8Property(const char* InName, int32 InSize, int32 InOffset)
//         : FNumericProperty(InName, InSize, InOffset)
//     {}
//
//     virtual void Serialize(void* Data) const override
//     {
//         
//     }
// };