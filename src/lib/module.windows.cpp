#ifdef WINDOWS

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <spdlog/spdlog.h>

#include "address.hpp"
#include "module.hpp"

#include "fnv1a_hash.hpp"

#include "pattern.hpp"

#include <Windows.h>
#include <winternl.h>

std::vector<std::pair<std::string, std::uintptr_t>> module_list{};

Module::Module(std::string_view str, bool read_from_disk, const FindPatternCallbackFn& find_pattern_callback)
{
    GetModuleInfo(str, read_from_disk);
    _find_pattern_callback = find_pattern_callback;
}

void Module::GetModuleInfo(std::string_view mod, bool read_from_disk)
{
    if (module_list.empty()) [[unlikely]]
    {
        const auto pteb = reinterpret_cast<PTEB>(__readgsqword(
                reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)));
        const auto peb = pteb->ProcessEnvironmentBlock;

        for (auto entry = peb->Ldr->InMemoryOrderModuleList.Flink;
             entry != &peb->Ldr->InMemoryOrderModuleList; entry = entry->Flink)
        {
            const auto module_entry = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

            std::wstring_view w_name = module_entry->FullDllName.Buffer;

            auto length = WideCharToMultiByte(CP_UTF8, 0, w_name.data(), w_name.length(), nullptr, 0, nullptr, nullptr);
            std::string name{};

            name.resize(length);
            WideCharToMultiByte(CP_UTF8, 0, w_name.data(), w_name.length(), (LPSTR) name.c_str(), length, nullptr, nullptr);

            std::ranges::replace(name, '\\', '/');

            // only need game dlls, do not need plugin (including metamod) dlls
            if (name.find("game/") == std::string::npos || name.find("addons") != std::string::npos)
                continue;

            auto& mod_info = module_list.emplace_back();

            mod_info.first = name;
            mod_info.second = reinterpret_cast<uintptr_t>(module_entry->DllBase);
        }
    }

    const auto it = std::ranges::find_if(module_list,
                                         [&](const auto& i) {
                                             auto mod_name = i.first.substr(i.first.find_last_of('/'));
                                             std::ranges::transform(mod_name, mod_name.begin(), tolower);
                                             return mod_name.find(mod) != std::string::npos;
                                         });

    if (it == module_list.end())
    {
        spdlog::error("Cannot find any module name that contains {}", mod);
        return;
    }

    const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(it->second);

    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    const auto bytes = reinterpret_cast<std::uint8_t*>(it->second);

    const auto nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(bytes + dos_header->e_lfanew);

    if (nt_header->Signature != IMAGE_NT_SIGNATURE)
        return;

    const std::string_view path = it->first;

    this->_base_address = it->second;
    this->_size = nt_header->OptionalHeader.SizeOfImage;
    this->_module_name = path.substr(path.find_last_of('/') + 1);

    std::vector<std::uint8_t> disk_data{};
    if (read_from_disk)
    {
        std::ifstream stream(it->first, std::ios::in | std::ios::binary);
        if (!stream.good())
            return;

        disk_data.reserve(std::filesystem::file_size(path));
        disk_data.assign((std::istreambuf_iterator(stream)), std::istreambuf_iterator<char>());
    }

    auto section = IMAGE_FIRST_SECTION(nt_header);

    for (auto i = 0; i < nt_header->FileHeader.NumberOfSections; i++, section++)
    {
        const auto is_executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        const auto is_readable = (section->Characteristics & IMAGE_SCN_MEM_READ) != 0;
        const auto is_writable = (section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;

        const auto start = this->_base_address + section->VirtualAddress;
        const auto size = std::min(section->SizeOfRawData, section->Misc.VirtualSize);

        auto& segment = _segments.emplace_back();
        segment.address = start;
        if (is_executable)
            segment.flags |= SegmentFlags::FLAG_X;
        if (is_readable)
            segment.flags |= SegmentFlags::FLAG_R;
        if (is_writable)
            segment.flags |= SegmentFlags::FLAG_W;

        if (read_from_disk && is_executable)
        {
            if (auto original_bytes = GetOriginalBytes(disk_data, start - _base_address, size))
            {
                segment.data = original_bytes.value();
                continue;
            }
            spdlog::error("[{}] Failed to copy bytes in executable section from file, copyting bytes from memory.", _module_name);
        }

        const auto data = reinterpret_cast<std::uint8_t*>(start);
        segment.data = std::vector(&data[0], &data[size]);
    }

    // DumpExports(reinterpret_cast<void*>(_base_address));
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
        auto result = pattern::impl::find_ptr(const_cast<std::uint8_t*>(data.data()), data.size(), ptr_bytes, size);
        {
            if (result.is_valid())
                return segment.address + result.ptr;
        }
    }

    return {};
}

Address Module::FindVtable(const std::string& name)
{
    auto vtable_name = std::format(".?AV{}@@", name);
    auto type_descriptor = FindString(vtable_name, false);
    if (!type_descriptor.is_valid())
    {
        spdlog::error("[{}] Cannot find vtable: {}", _module_name, vtable_name);
        return {};
    }

    type_descriptor = type_descriptor.offset(-0x10);
    std::uint32_t rtti_rva = type_descriptor.ptr - _base_address;

    auto complete_object_locator = FindPtr(rtti_rva, 4);

    if (!complete_object_locator.is_valid())
    {
        spdlog::error("[{}] complete_object_locator of vtable \"{}\" is invalid", _module_name, vtable_name);
        return {};
    }

    // check for header offset
    auto header = complete_object_locator.offset(-0xC);
    if (header.deref().cast<std::int32_t>() != 1)
        return {};
    // check for vtable offset
    if (complete_object_locator.offset(-0x8).deref().cast<std::int32_t>() != 0)
        return {};

    auto vtable = FindPtr(header.ptr);
    if (vtable.is_valid())
        return vtable.offset(8);

    return {};
}

void* Module::GetProc(const std::string_view proc_name) const
{
    return GetProcAddress(reinterpret_cast<HMODULE>(_base_address), proc_name.data());
}

void* Module::GetProc(std::uint64_t proc_name_hash) const
{
    spdlog::error("NO GetProc(std::uint64_t proc_name_hash) ON WINDOWS");
    return nullptr;
}

void Module::DumpExports(void* module_base)
{
    const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_base);

    const auto nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_base) +
                                                               dos_header->e_lfanew);

    const auto [export_address_rva, export_size] = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (export_size == 0 || export_address_rva == 0)
        return;

    auto export_directory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(_base_address + export_address_rva);
    const auto names = reinterpret_cast<uint32_t*>(_base_address + export_directory->AddressOfNames);
    const auto addresses = reinterpret_cast<uint32_t*>(_base_address + export_directory->AddressOfFunctions);
    const auto ordinals = reinterpret_cast<std::uint16_t*>(_base_address + export_directory->AddressOfNameOrdinals);

    for (auto i = 0ull; i < export_directory->NumberOfNames; i++)
    {
        const auto export_name = reinterpret_cast<const char*>(_base_address + names[i]);
        const auto address = reinterpret_cast<std::uintptr_t>(module_base) + addresses[ordinals[i]];

        if (address >= reinterpret_cast<uintptr_t>(export_directory) &&
            address < reinterpret_cast<uintptr_t>(export_directory) + export_size)
            continue;

        auto hash = fnv1a64::hash(export_name);
        _exports[hash] = address;
    }
}

std::optional<std::vector<std::uint8_t>>
Module::GetOriginalBytes(const std::vector<std::uint8_t>& disk_data, std::uintptr_t rva, std::size_t size)
{
    auto get_file_ptr_from_rva = [](std::uint8_t* data, std::uintptr_t address) -> std::optional<std::uintptr_t> {
        // thank you praydog
        // https://github.com/cursey/kananlib/blob/b0323a0b005fc9e3944e0ea36dcc98eda4b84eea/src/Module.cpp#L176

        const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(data);
        const auto nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(&data[dos_header->e_lfanew]);
        auto section = IMAGE_FIRST_SECTION(nt_header);
        for (auto i = 0; i < nt_header->FileHeader.NumberOfSections; i++, section++)
        {
            auto section_size = section->Misc.VirtualSize;
            if (section_size == 0)
                section_size = section->SizeOfRawData;

            if (address >= section->VirtualAddress &&
                address < static_cast<uintptr_t>(section->VirtualAddress) + section_size)
            {
                const auto delta = section->VirtualAddress - section->PointerToRawData;

                return reinterpret_cast<std::uintptr_t>(data + (address - delta));
            }
        }
        return std::nullopt;
    };

    const auto disk_ptr = get_file_ptr_from_rva(const_cast<std::uint8_t*>(disk_data.data()), rva);
    if (!disk_ptr)
        return std::nullopt;

    const auto disk_bytes = reinterpret_cast<std::uint8_t*>(*disk_ptr);
    std::vector<std::uint8_t> result{&disk_bytes[0], &disk_bytes[size]};

    return result;
}

#endif