#pragma once

#include "fnv1a_hash.hpp"
#include "pattern.hpp"

#include <cstdint>
#include <cstdint>
#include <vector>
#include <span>
#include <functional>

namespace arisu::impl
{
    struct Segments
    {
        Segments() = default;

        Segments(const Segments&)            = default;
        Segments(Segments&&)                 = default;
        Segments& operator=(const Segments&) = default;
        Segments& operator=(Segments&&)      = default;

        std::uintptr_t address{};
        std::vector<std::uint8_t> data{};
    };

    class Module
    {
    public:
        using FindPatternCallbackFn = std::function<void(std::string_view, std::expected<Address, pattern::Status>, std::uintptr_t)>;

    private:
        std::vector<Segments> _segments{};
        std::uintptr_t _base_address{};
        std::size_t _size{};
        FindPatternCallbackFn _find_pattern_callback;
        std::unordered_map<std::uint64_t, std::uintptr_t> _exports {};

        void GetModuleNfo(std::string_view mod = "");

    public:
        Module()                         = default;
        Module(const Module&)            = default;
        Module(Module&&)                 = default;
        Module& operator=(const Module&) = default;
        Module& operator=(Module&&)      = default;

        explicit Module(std::string_view str, const FindPatternCallbackFn& find_pattern_callback = nullptr);

        // get rwx segments of a module
        const std::vector<Segments>& GetSegments() const
        {
            return _segments;
        }

        // get base address of a module
        [[nodiscard]] std::uintptr_t Base() const
        {
            return _base_address;
        }

        [[nodiscard]] Address FindPattern(std::string_view pattern) const;

        void* GetProc(std::string_view proc_name) const;
        void* GetProc(std::uint64_t proc_name_hash) const;

    private:
        void DumpExports(std::uintptr_t module_base);
        std::uintptr_t GetExport(std::uintptr_t module_base, const std::string& name = "");
    };
}