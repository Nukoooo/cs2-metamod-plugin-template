#include "pattern.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <string_view>
#include <vector>
#include "simd.hpp"

Address pattern::impl::find_std(std::uint8_t* data, std::size_t size, const std::vector<HexData>& pattern) noexcept
{
    const auto pattern_size = pattern.size();
    std::uint8_t* end       = data + size - pattern_size;
    const auto first_byte   = pattern[0].value();

    for (std::uint8_t* current = data; current <= end; ++current)
    {
        current = std::find(current, end, first_byte);

        if (current == end)
        {
            break;
        }

        if (std::equal(pattern.begin() + 1,
                       pattern.end(),
                       current + 1,
                       [](auto opt, auto byte)
                       {
                           return !opt.has_value() || *opt == byte;
                       }))
        {
            return current - data;
        }
    }

    return {};
}

Address pattern::impl::find_str(std::uint8_t* data, std::size_t size, const std::string& str, bool zero_terminated) noexcept
{
    std::uint8_t* end = data + size;
    const auto str_data     = (uint8_t*)str.c_str();
    const auto str_size     = str.size() + (zero_terminated ? 1 : 0);

    for (std::uint8_t* current = data; current <= end; ++current)
    {
        current = std::find(current, end, *str_data);

        if (current == end)
        {
            break;
        }

        if (std::memcmp(current, str_data, str_size) == 0)
        {
            return current - data;
        }
    }

    return {};
}

Address pattern::impl::find_ptr(std::uint8_t* data, std::size_t size, std::uint8_t* ptr, std::uint8_t ptr_size) noexcept
{
    std::uint8_t* end = data + size;

    for (std::uint8_t* current = data; current <= end; ++current)
    {
        current = std::find(current, end, *ptr);

        if (current == end)
        {
            break;
        }

        if (std::memcmp(current, ptr, ptr_size) == 0)
        {
            return current - data;
        }
    }

    return {};
}

Address pattern::find(const std::vector<std::uint8_t>& data, const std::vector<impl::HexData>& pattern) noexcept
{
    auto result = impl::find_std(const_cast<std::uint8_t*>(data.data()), data.size(), pattern);

    return result;
}

Address pattern::find(const std::vector<std::uint8_t>& data, const impl::Pattern<>& pattern) noexcept
{
    return find(data, pattern.bytes);
}

Address pattern::find(const std::vector<std::uint8_t>& data, std::string_view pattern) noexcept
{
    return pattern::find(data, type(pattern));
}