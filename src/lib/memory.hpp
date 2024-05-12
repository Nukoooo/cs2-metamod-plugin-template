#pragma once

#include "address.hpp"
#include <memory>

class Memory
{
  public:
    template <size_t Index>
    static Address GetVFuncAddress(Address vtable)
    {
        return vtable.deref().cast<uintptr_t*>()[Index];
    }

    static Address GetVFuncAddress(Address vtable, std::size_t index)
    {
        return vtable.deref().cast<uintptr_t*>()[index];
    }

#ifdef _WIN32
    template <typename T, size_t Index, typename... Args>
    static T VirtualCall(void* vtable, Args... args)
    {
        using fn = T(__thiscall*)(void*, decltype(args)...);
        return GetVFuncAddress<Index>(vtable).template cast<fn>()(vtable, args...);
    }

    template <typename T, size_t Index, typename... Args>
    static T VirtualCdeclCall(void* vtable, Args... args)
    {
        using fn = T(__cdecl*)(void*, decltype(args)...);
        return GetVFuncAddress<Index>(vtable).template cast<fn>()(vtable, args...);
    }

    template <typename T, typename... Args>
    static T VirtualCall(void* vtable, std::size_t index, Args... args)
    {
        using fn = T(__thiscall*)(void*, decltype(args)...);
        return GetVFuncAddress(vtable, index).template cast<fn>()(vtable, args...);
    }

    template <typename T, typename... Args>
    static T VirtualCdeclCall(void* vtable, std::size_t index, Args... args)
    {
        using fn = T(__cdecl*)(void*, decltype(args)...);
        return GetVFuncAddress(vtable, index).template cast<fn>()(vtable, args...);
    }

#elif LINUX
    template <typename T, size_t Index, typename... Args>
    static constexpr T VirtualCall(void* vtable, Args... args)
    {
        using fn = T (*)(void*, decltype(args)...);
        return GetVFuncAddress<Index>(vtable).template cast<fn>()(vtable, args...);
    }

    template <typename T, typename... Args>
    static T VirtualCall(void* vtable, std::size_t index, Args... args)
    {
        using fn = T (*)(void*, decltype(args)...);
        return GetVFuncAddress(vtable, index).template cast<fn>()(vtable, args...);
    }
#endif
};
