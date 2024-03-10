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
        std::uintptr_t _baseAddress{};
        std::size_t _size{};

        void* _handle{};
        void get_module_nfo(std::string_view mod = "");
        bool _loaded{};
        FindPatternCallbackFn _find_pattern_callback;

        std::unordered_map<std::uint64_t, std::uintptr_t> _exports {};

    public:
        Module()                         = default;
        Module(const Module&)            = default;
        Module(Module&&)                 = default;
        Module& operator=(const Module&) = default;
        Module& operator=(Module&&)      = default;

        explicit Module(std::string_view str, const FindPatternCallbackFn& find_pattern_callback = nullptr);

        // get rwx segments of a module
        std::vector<Segments>& get_segments()
        {
            return _segments;
        }

        // get base address of a module
        [[nodiscard]] std::uintptr_t base() const
        {
            return _baseAddress;
        }

        [[nodiscard]] Address find_pattern(std::string_view pattern) const;

        void* get_proc(std::string_view proc_name) const;
        void* get_proc(std::uint64_t proc_name_hash) const;

    private:
        void dump_exports(std::uintptr_t module_base);
        std::uintptr_t get_export(std::uintptr_t module_base, const std::string& name = "");
    };
}