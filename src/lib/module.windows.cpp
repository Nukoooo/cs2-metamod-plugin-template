#ifdef WINDOWS
#include "address.hpp"
#include "module.hpp"

#include "fnv1a_hash.hpp"
/*#include "logger.hpp"*/
#include "pattern.hpp"

#include <Windows.h>
#include <winnt.h>
#include <winternl.h>

std::unordered_map<std::uintptr_t, std::uintptr_t> module_list {};

arisu::impl::Module::Module(std::string_view str, const FindPatternCallbackFn& find_pattern_callback)
{
    get_module_nfo(str);
    _find_pattern_callback = find_pattern_callback;
}

void arisu::impl::Module::get_module_nfo(std::string_view mod)
{
    if (module_list.empty())
    {
        const auto pteb = reinterpret_cast<PTEB>(__readgsqword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)));
        auto peb        = pteb->ProcessEnvironmentBlock;

        for (auto entry = peb->Ldr->InMemoryOrderModuleList.Flink; entry != &peb->Ldr->InMemoryOrderModuleList; entry = entry->Flink)
        {
            auto module_entry = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
            
        }
    }

    const auto handle = GetModuleHandleA(mod.empty() ? nullptr : mod.data());
    if (!handle)
        return;

    const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(handle);

    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    const auto bytes = reinterpret_cast<std::uint8_t*>(handle);

    const auto nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(bytes + dos_header->e_lfanew);

    if (nt_header->Signature != IMAGE_NT_SIGNATURE)
        return;

    this->_handle      = handle;
    this->_baseAddress = reinterpret_cast<uintptr_t>(handle);
    this->_size        = nt_header->OptionalHeader.SizeOfImage;
    auto section       = IMAGE_FIRST_SECTION(nt_header);

    for (auto i = 0; i < nt_header->FileHeader.NumberOfSections; i++, section++)
    {
        const auto is_executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;

        if (const auto is_readable = (section->Characteristics & IMAGE_SCN_MEM_READ) != 0; is_executable && is_readable)
        {
            const auto start = this->_baseAddress + section->VirtualAddress;
            const auto size  = std::min(section->SizeOfRawData, section->Misc.VirtualSize);
            const auto data  = reinterpret_cast<std::uint8_t*>(start);

            auto segment = _segments.emplace_back();

            segment.address = start;
            segment.data    = std::vector(&data[0], &data[size]);
        }
    }

    dump_exports(_baseAddress);
}

arisu::Address arisu::impl::Module::find_pattern(std::string_view pattern) const
{
    for (auto&& segment : _segments)
    {
        if (auto result = pattern::find(segment.data, pattern))
        {
            if (_find_pattern_callback)
                _find_pattern_callback(pattern, result, _baseAddress);
            if (result.has_value())
                return segment.address + result.value();
        }
    }

    return {};
}

void* arisu::impl::Module::get_proc(const std::string_view proc_name) const
{
    if (!this->_handle)
        return nullptr;

    const auto hash = fnv1a64::hash(proc_name);

    if (const auto it = _exports.find(hash); it != _exports.end())
        return reinterpret_cast<void*>(it->second);

    return nullptr;
}

void* arisu::impl::Module::get_proc(std::uint64_t proc_name_hash) const
{
    if (!this->_handle)
        return nullptr;

    if (const auto it = _exports.find(proc_name_hash); it != _exports.end())
        return reinterpret_cast<void*>(it->second);

    return nullptr;
}

void arisu::impl::Module::dump_exports(std::uintptr_t module_base)
{
    const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_base);

    const auto nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_base) + dos_header->e_lfanew);

    const auto [export_address_rva, export_size] = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (export_size == 0 || export_address_rva == 0)
        return;

    auto export_directory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(_baseAddress + export_address_rva);
    const auto names      = reinterpret_cast<uintptr_t*>(_baseAddress + export_directory->AddressOfNames);
    const auto addresses  = reinterpret_cast<uintptr_t*>(_baseAddress + export_directory->AddressOfFunctions);
    const auto ordinals   = reinterpret_cast<std::uint16_t*>(_baseAddress + export_directory->AddressOfNameOrdinals);

    for (auto i = 0; i < export_directory->NumberOfNames; i++)
    {
        std::string_view export_name = reinterpret_cast<const char*>(_baseAddress + names[i]);

        const auto address = module_base + addresses[ordinals[i]];

        if (address >= reinterpret_cast<uintptr_t>(export_directory) && address < reinterpret_cast<uintptr_t>(export_directory) + export_size)
            continue;
        auto hash      = fnv1a64::hash(export_name);
        _exports[hash] = address;
    }
}

std::uintptr_t arisu::impl::Module::get_export(std::uintptr_t module_base, const std::string& name)
{
    // todo: get forward export without duplicating code
    return 0;
}
#endif
