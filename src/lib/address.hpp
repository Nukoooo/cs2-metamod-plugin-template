#pragma once

#include <cstdint>
#include <type_traits>

template <typename type = std::uintptr_t>
struct AddressBase
{
    type ptr = 0;

    AddressBase()
     : ptr {}
    {
    }

    template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    AddressBase(T src)
     : ptr { static_cast<type>(src) }
    {
    }

    template <typename T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
    AddressBase(T src)
     : ptr { reinterpret_cast<type>(src) }
    {
    }

    ~AddressBase() = default;

    operator type() const
    {
        return ptr;
    }

    operator void*() const
    {
        return reinterpret_cast<void*>(ptr);
    }

    operator type()
    {
        return ptr;
    }

    operator void*()
    {
        return reinterpret_cast<void*>(ptr);
    }

    bool is_valid()
    {
#ifdef ENVIRONMENT32
        return ptr >= 0x1000 && ptr < 0xFFFFFFEF;
#else
        return ptr >= 0x1000 && ptr < 0x7FFFFFFEFFFF;
#endif
    }

    [[nodiscard]] bool is_valid() const
    {
#ifdef ENVIRONMENT32
        return ptr >= 0x1000 && ptr < 0xFFFFFFEF;
#else
        return ptr >= 0x1000 && ptr < 0x7FFFFFFEFFFF;
#endif
    }

    template <typename T = type>
    T cast()
    {
        return T(ptr);
    }

    template <typename T = AddressBase<type>>
    T rel(std::intptr_t offset = 0x1)
    {
        type base                   = ptr + offset;
        const auto relative_address = *reinterpret_cast<std::int32_t*>(base);
        base += sizeof(std::int32_t) + relative_address;

        return T(base);
    }

    template <typename T = AddressBase<type>>
    T find_opcode(std::uint8_t opcode, bool forward = true)
    {
        auto base = ptr;

        while (true)
        {
            if (const auto currentOpcode = *reinterpret_cast<std::uint8_t*>(base); currentOpcode == opcode)
                break;

            forward ? ++base : --base;
        }

        return T(base);
    }

    template <typename T = AddressBase<type>>
    T deref(uint8_t count = 1)
    {
        type base = ptr;
        if (count == 0)
            return base;

        while (count--)
        {
            if (T(base).is_valid())
                base = *reinterpret_cast<type*>(base);
        }

        return T(base);
    }

    template <typename T = AddressBase<type>>
    T offset(std::intptr_t offset)
    {
        return T(ptr + offset);
    }
};

using Address = AddressBase<>;
