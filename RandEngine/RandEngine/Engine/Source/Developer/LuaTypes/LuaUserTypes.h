#pragma once
#include "sol/sol.hpp"


template <typename T>
struct FLuaCustomBind
{
    static void Bind([[maybe_unused]] sol::state& State)
    {
        static_assert(sizeof(T) == 0, "Binding not implemented for this type!");
    }
};
