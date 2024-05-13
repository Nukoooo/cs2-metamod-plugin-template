#ifdef LINUX
#include <algorithm>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <fstream>
#include <iostream>
#include <bit>
#include <cstdint>
#include <vector>

#include <elf.h>
#include <link.h>

#include "address.hpp"
#include "module.hpp"

#include "fnv1a_hash.hpp"
#include "pattern.hpp"

std::vector<std::pair<std::string, dl_phdr_info>> module_list {};

Module::Module(std::string_view str, bool read_from_disk, const FindPatternCallbackFn& find_pattern_callback)
{
    GetModuleInfo(str, read_from_disk);
    _find_pattern_callback = find_pattern_callback;
}

void Module::GetModuleInfo(std::string_view mod, bool read_from_disk)
{
    if (module_list.empty()) [[unlikely]]
    {
        dl_iterate_phdr(
          [](struct dl_phdr_info* info, size_t, void*)
          {
              std::string name = info->dlpi_name;

              if (name.rfind(".so") == std::string::npos)
                  return 0;

              if (name.find("/addons/") != std::string::npos)
                  return 0;

              constexpr std::string_view ROOTBIN = "/bin/linuxsteamrt64/";
              constexpr std::string_view GAMEBIN = "/csgo/bin/linuxsteamrt64/";

              bool isFromRootBin = name.find(ROOTBIN) != std::string::npos;
              bool isFromGameBin = name.find(GAMEBIN) != std::string::npos;
              if (!isFromGameBin && !isFromRootBin)
                  return 0;

              auto& mod_info = module_list.emplace_back();

              mod_info.first  = name;
              mod_info.second = *info;
              return 0;
          },
          nullptr);
    }

    const auto it = std::ranges::find_if(module_list,
                                         [&](const auto& i)
                                         {
                                             auto mod_name = i.first.substr(i.first.find_last_of('/') + 1);
                                             return mod_name.find(mod) != std::string::npos;
                                         });

    if (it == module_list.end())
    {
        spdlog::error("Cannot find any module name that contains {}", mod);
        return;
    }

    const std::string_view path = it->first;
    const auto info             = it->second;

    this->_base_address = info.dlpi_addr;
    this->_module_name  = path.substr(path.find_last_of('/') + 1);

    std::vector<std::uint8_t> disk_data {};
    if (read_from_disk)
    {
        std::ifstream stream(it->first, std::ios::in | std::ios::binary);
        if (!stream.good())
        {
            return;
        }
        disk_data.reserve(std::filesystem::file_size(path));
        disk_data.assign((std::istreambuf_iterator(stream)), std::istreambuf_iterator<char>());
    }

    for (auto i = 0; i < info.dlpi_phnum; i++)
    {
        auto address = _base_address + info.dlpi_phdr[i].p_paddr;
        auto size    = info.dlpi_phdr[i].p_filesz;

        auto type               = info.dlpi_phdr[i].p_type;
        auto is_dynamic_section = type == PT_DYNAMIC;

        auto flags = info.dlpi_phdr[i].p_flags;

        auto is_executable = (flags & PF_X) != 0;
        auto is_readable   = (flags & PF_R) != 0;
        auto is_writable   = (flags & PF_W) != 0;

        if (is_dynamic_section)
        {
            DumpExports(reinterpret_cast<void*>(address));
            continue;
        }

        if (type != PT_LOAD)
            continue;

        if (info.dlpi_phdr[i].p_paddr == 0)
            continue;

        auto* data = reinterpret_cast<std::uint8_t*>(address);

        auto& segment = _segments.emplace_back();

        segment.address = address;
        segment.data.reserve(size);
        if (is_executable)
            segment.flags |= SegmentFlags::FLAG_X;
        if (is_readable)
            segment.flags |= SegmentFlags::FLAG_R;
        if (is_writable)
            segment.flags |= SegmentFlags::FLAG_W;

        if (read_from_disk && is_executable)
        {
            if (auto bytes = GetOriginalBytes(disk_data, address - _base_address, size))
            {
                segment.data = bytes.value();
                continue;
            }
            spdlog::error("[{}] Failed to copy bytes in executable section from file, copyting bytes from memory.", _module_name);
        }

        segment.data.assign(&data[0], &data[size]);
        continue;
    }
}

Address Module::FindPattern(std::string_view pattern) const
{
    for (auto&& segment : _segments)
    {
        if ((segment.flags & SegmentFlags::FLAG_X) == 0)
            continue;

        if (auto result = pattern::find(segment.data, pattern))
        {
            if (_find_pattern_callback)
                _find_pattern_callback(pattern, result, _base_address);

            if (result.is_valid())
                return segment.address + result.ptr;
        }
    }

    return {};
}

Address Module::FindString(const std::string& str, bool read_only) const
{
    for (auto&& segment : _segments)
    {
        if ((segment.flags & SegmentFlags::FLAG_X) != 0)
            continue;

        if (read_only && (segment.flags & SegmentFlags::FLAG_W) != 0)
            continue;

        const auto& data = segment.data;

        if (auto result = pattern::impl::find_str(const_cast<std::uint8_t*>(data.data()), data.size(), str, true))
        {
            if (_find_pattern_callback)
                _find_pattern_callback(str, result, _base_address);

            if (result.is_valid())
                return segment.address + result.ptr;
        }
    }

    return {};
}

Address Module::FindPtr(std::uintptr_t ptr, std::uint8_t size) const
{
    auto ptr_bytes = reinterpret_cast<std::uint8_t*>(&ptr);

    for (auto&& segment : _segments)
    {
        if ((segment.flags & SegmentFlags::FLAG_X) != 0)
            continue;

        const auto& data = segment.data;
        auto result      = pattern::impl::find_ptr(const_cast<std::uint8_t*>(data.data()), data.size(), ptr_bytes, size);
        if (result.is_valid())
            return segment.address + result.ptr;
    }

    return {};
}

Address Module::FindVtable(const std::string& name)
{
    auto decoreated_name = std::to_string(name.length()) + name;
    auto type_info_name  = FindString(decoreated_name, true);
    if (!type_info_name.is_valid())
    {
        spdlog::error("[{}] Cannot find vtable: {}", _module_name, decoreated_name);
        return {};
    }

    auto reference_type = FindPtr(type_info_name.ptr);

    if (!reference_type.is_valid())
    {
        spdlog::error("[{}] reference_type of vtable \"{}\" is invalid", _module_name, vtable_name);
        return {};
    }

    auto type_info = reference_type.offset(-8);
    auto reference = FindPtr(type_info.ptr);
    if (reference.is_valid())
        return reference.offset(8);

    spdlog::error("[{}] Cannot find refercen to typeinfo for vtable: {}", _module_name, decoreated_name);

    return {};
}

void* Module::GetProc(const std::string_view proc_name) const
{
    const auto hash = fnv1a64::hash(proc_name);

    if (const auto it = _exports.find(hash); it != _exports.end())
        return reinterpret_cast<void*>(it->second);

    return nullptr;
}

void* Module::GetProc(std::uint64_t proc_name_hash) const
{
    if (const auto it = _exports.find(proc_name_hash); it != _exports.end())
        return reinterpret_cast<void*>(it->second);

    return nullptr;
}

void Module::DumpExports(void* module_base)
{
    auto dyn = (ElfW(Dyn)*)(module_base);
    // thanks to https://stackoverflow.com/a/57099317
    auto GetNumberOfSymbolsFromGnuHash = [](ElfW(Addr) gnuHashAddress)
    {
        // See https://flapenguin.me/2017/05/10/elf-lookup-dt-gnu-hash/ and
        // https://sourceware.org/ml/binutils/2006-10/msg00377.html
        struct Header
        {
            uint32_t nbuckets;
            uint32_t symoffset;
            uint32_t bloom_size;
            uint32_t bloom_shift;
        };

        auto header               = (Header*)gnuHashAddress;
        const auto bucketsAddress = gnuHashAddress + sizeof(Header) + (sizeof(std::uintptr_t) * header->bloom_size);

        // Locate the chain that handles the largest index bucket.
        uint32_t lastSymbol = 0;
        auto bucketAddress  = (uint32_t*)bucketsAddress;
        for (uint32_t i = 0; i < header->nbuckets; ++i)
        {
            uint32_t bucket = *bucketAddress;
            if (lastSymbol < bucket)
            {
                lastSymbol = bucket;
            }
            bucketAddress++;
        }

        if (lastSymbol < header->symoffset)
        {
            return header->symoffset;
        }

        // Walk the bucket's chain to add the chain length to the total.
        const auto chainBaseAddress = bucketsAddress + (sizeof(uint32_t) * header->nbuckets);
        for (;;)
        {
            auto chainEntry = (uint32_t*)(chainBaseAddress + (lastSymbol - header->symoffset) * sizeof(uint32_t));
            lastSymbol++;

            // If the low bit is set, this entry is the end of the chain.
            if (*chainEntry & 1)
            {
                break;
            }
        }

        return lastSymbol;
    };

    ElfW(Sym) * symbols {};
    ElfW(Word) * hash_ptr {};

    char* string_table {};
    std::size_t symbol_count {};

    while (dyn->d_tag != DT_NULL)
    {
        if (dyn->d_tag == DT_HASH)
        {
            hash_ptr     = reinterpret_cast<ElfW(Word)*>(dyn->d_un.d_ptr);
            symbol_count = hash_ptr[1];
        }
        else if (dyn->d_tag == DT_STRTAB)
        {
            string_table = reinterpret_cast<char*>(dyn->d_un.d_ptr);
        }
        else if (!symbol_count && dyn->d_tag == DT_GNU_HASH)
        {
            symbol_count = GetNumberOfSymbolsFromGnuHash(dyn->d_un.d_ptr);
        }
        else if (dyn->d_tag == DT_SYMTAB)
        {
            symbols = reinterpret_cast<ElfW(Sym)*>(dyn->d_un.d_ptr);

            for (auto i = 0; i < symbol_count; i++)
            {
                if (!symbols[i].st_name)
                {
                    continue;
                }

                if (symbols[i].st_other != 0)
                {
                    continue;
                }

                auto address          = symbols[i].st_value + _base_address;
                std::string_view name = &string_table[symbols[i].st_name];
                auto hash             = fnv1a64::hash(name);
                _exports[hash]        = address;
            }
        }
        dyn++;
    }
}

std::optional<std::vector<std::uint8_t>> Module::GetOriginalBytes(const std::vector<std::uint8_t>& disk_data, std::uintptr_t rva, std::size_t size)
{
    auto get_file_ptr_from_rva = [](std::uint8_t* data, std::uintptr_t address) -> std::optional<std::uintptr_t>
    {
        return reinterpret_cast<std::uintptr_t>(data + address);
    };

    const auto disk_ptr = get_file_ptr_from_rva(const_cast<std::uint8_t*>(disk_data.data()), rva);
    if (!disk_ptr)
        return std::nullopt;

    const auto disk_bytes = reinterpret_cast<std::uint8_t*>(*disk_ptr);
    std::vector<std::uint8_t> result { &disk_bytes[0], &disk_bytes[size] };

    return result;
}
#endif