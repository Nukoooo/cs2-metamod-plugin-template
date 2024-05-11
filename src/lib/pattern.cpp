#include "pattern.hpp"
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

std::expected<Address, pattern::Status> pattern::find(const std::vector<std::uint8_t>& data, const std::vector<impl::HexData>& pattern) noexcept
{
    auto result = impl::find_std(const_cast<std::uint8_t*>(data.data()), data.size(), pattern);
    if (!result.is_valid())
        return std::unexpected { NoResult };

    return result;
}

std::expected<Address, pattern::Status> pattern::find(const std::vector<std::uint8_t>& data, const impl::Pattern<>& pattern) noexcept
{
    return find(data, pattern.bytes);
}

std::expected<Address, pattern::Status> pattern::find(const std::vector<std::uint8_t>& data, std::string_view pattern) noexcept
{
    return pattern::find(data, type(pattern));
}