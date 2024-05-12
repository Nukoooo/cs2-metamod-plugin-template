#pragma once

#include "fnv1a_hash.hpp"
#include "pattern.hpp"

#include <cstdint>
#include <cstdint>
#include <vector>
#include <functional>

struct Segments
{
    Segments() = default;
    Segments(Segments&&) = default;
    Segments(const Segments&) = delete;
    Segments& operator=(const Segments&) = delete;
    Segments& operator=(Segments&&) = delete;

    std::uintptr_t address{ };
    std::vector<std::uint8_t> data{ };
};

class Module
{
  public:
    using FindPatternCallbackFn = std::function<void(std::string_view, Address, std::uintptr_t)>;

  private:
    std::vector<Segments> _segments{ };
    std::uintptr_t _base_address{ };
    std::size_t _size{ };
    FindPatternCallbackFn _find_pattern_callback;
    std::unordered_map<std::uint64_t, std::uintptr_t> _exports{ };
    std::string _module_name{ };

    void GetModuleInfo(std::string_view mod, bool read_from_disk);

  public:
    Module() = default;
    Module(const Module&) = default;
    Module(Module&&) noexcept = default;
    Module& operator=(const Module&) = default;
    Module& operator=(Module&&) = default;

    explicit Module(std::string_view str, bool read_from_disk = false,
                    const FindPatternCallbackFn& find_pattern_callback = nullptr);

    // get rwx segments of a module
    [[nodiscard]] const std::vector<Segments>& GetSegments() const
    {
        return _segments;
    }

    // get base address of a module
    [[nodiscard]] std::uintptr_t Base() const
    {
        return _base_address;
    }

    [[nodiscard]] std::string_view ModuleName() const
    {
        return _module_name;
    }

    [[nodiscard]] Address FindPattern(std::string_view pattern) const;

    [[nodiscard]] void* GetProc(std::string_view proc_name) const;

    [[nodiscard]] void* GetProc(std::uint64_t proc_name_hash) const;

    [[nodiscard]] Address FindVTableByName(std::string_view name);

  private:
    void DumpExports(void* module_base);

    std::optional<std::vector<std::uint8_t>>
    GetOriginalBytes(const std::vector<std::uint8_t>& disk_data, std::uintptr_t rva, std::size_t size);
};
