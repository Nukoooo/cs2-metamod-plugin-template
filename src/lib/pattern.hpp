#pragma once

#include "address.hpp"
#include <optional>
#include <vector>
#include <array>
#include <ranges>

namespace pattern
{
    namespace impl
    {
        using HexData = std::optional<std::uint8_t>;

        [[nodiscard]] static constexpr std::optional<uint8_t> hex_char_to_byte(char c) noexcept
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;

            return std::nullopt;
        }

        template<char Wildcard = '?'>
        [[nodiscard]] static constexpr std::optional<std::uint8_t> parse_hex(std::string_view str) noexcept
        {
            if (str.size() == 1 && str.front() == Wildcard)
                return std::nullopt;

            if (str.size() != 2)
                return std::nullopt;

            auto high = hex_char_to_byte(str[0]);
            auto low = hex_char_to_byte(str[1]);

            // if both are not wildcard, e.g AA
            if (high.has_value() && low.has_value())
            {
                return (high.value() << 4) | low.value();
            }

            return std::nullopt;
            /*
            comment these out when find pattern supports this kind of pattern
            // A?
            else if (high.has_value() && !low.has_value())
            {
                return high.value() << 4;
            }
            // ?A
            else if (!high.has_value() && low.has_value())
            {
                return low.value();
            }
            */
        }

        template<char Delimiter = ' ', char Wildcard = '?'>
        [[nodiscard]] static constexpr std::vector<HexData> parse_pattern(std::string_view pattern) noexcept
        {
            std::vector<HexData> result{ };
            for (const auto& str : pattern | std::views::split(Delimiter))
            {
                const std::string_view token(str.begin(), str.end());
                result.push_back(parse_hex<Wildcard>(token));
            }
            return result;
        }

        // https://stackoverflow.com/a/73014828
        template<auto N>
        static constexpr auto str(const char (& cstr)[N]) noexcept
        {
            std::array<char, N> arr;
            for (std::size_t i = 0; i < N; ++i)
                arr[i] = cstr[i];
            return arr;
        }

        template<auto str>
        constexpr auto make_pattern() noexcept
        {
            const auto sig = impl::parse_pattern(str.data());
            constexpr auto size = impl::parse_pattern(str.data()).size();
            std::array<HexData, size> arr{ };
            for (std::size_t i = 0; i < size; i++)
                arr[i] = sig[i];
            return arr;
        }

        template<char Delimiter = ' ', char Wildcard = '?'>
        struct Pattern
        {
            explicit constexpr Pattern(std::string_view pattern)
            {
                bytes = parse_pattern<Delimiter, Wildcard>(pattern);
                if (!bytes.empty() && bytes.front().has_value())
                    valid = true;
            }

            [[nodiscard]] constexpr std::size_t size() const noexcept
            {
                return bytes.size();
            }

            constexpr const HexData& operator[](std::size_t index) const noexcept
            {
                return bytes[index];
            }

            bool valid;
            std::vector<HexData> bytes;
        };

        Address find_std(std::uint8_t* data, std::size_t size, const std::vector<HexData>& pattern) noexcept;
        Address find_str(std::uint8_t* data, std::size_t size, const std::string& str, bool zero_terminated = false) noexcept;
        Address find_ptr(std::uint8_t* data, std::size_t size, std::uint8_t* ptr, std::uint8_t ptr_size = sizeof(void*)) noexcept;
    }

    Address find(const std::vector<std::uint8_t>& data, const impl::Pattern<>& pattern) noexcept;
    Address find(const std::vector<std::uint8_t>& data, const std::vector<impl::HexData>& pattern) noexcept;
    Address find(const std::vector<std::uint8_t>& data, std::string_view pattern) noexcept;

    using type = impl::Pattern<' ', '?'>;
}