#ifdef WINDOWS
#include <format>
#include <print>

#include "address.hpp"
#include "module.hpp"

#include "fnv1a_hash.hpp"
/*#include "logger.hpp"*/
#include "pattern.hpp"

#include <Windows.h>
#include <winternl.h>

std::vector<std::pair<std::string, std::uintptr_t>> module_list{};

arisu::impl::Module::Module(std::string_view str, const FindPatternCallbackFn& find_pattern_callback)
{
    GetModuleNfo(str);
    _find_pattern_callback = find_pattern_callback;
}

void arisu::impl::Module::GetModuleNfo(std::string_view mod)
{
    if (module_list.empty())
    [[unlikely]]
    {
        const auto pteb = reinterpret_cast<PTEB>(__readgsqword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)));
        const auto peb  = pteb->ProcessEnvironmentBlock;

        for (auto entry = peb->Ldr->InMemoryOrderModuleList.Flink; entry != &peb->Ldr->InMemoryOrderModuleList; entry = entry->Flink)
        {
            const auto module_entry = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

            std::wstring_view w_name = module_entry->FullDllName.Buffer;

            std::string name(w_name.begin(), w_name.end());

            // only need game dlls, do not need plugin (including metamod) dlls
            if (!name.contains("game\\") || name.contains("addons"))
                continue;

            auto& mod_info = module_list.emplace_back();

            mod_info.first  = name;
            mod_info.second = reinterpret_cast<uintptr_t>(module_entry->DllBase);
        }
    }

    const auto it = std::ranges::find_if(module_list,
                                         [&](const auto& i)
                                         {
                                             return i.first.contains(mod);
                                         });

    if (it == module_list.end())
        throw std::runtime_error(std::format("Cannot find any module name that contains {}", mod));

    const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(it->second);

    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    const auto bytes = reinterpret_cast<std::uint8_t*>(it->second);

    const auto nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(bytes + dos_header->e_lfanew);

    if (nt_header->Signature != IMAGE_NT_SIGNATURE)
        return;

    this->_base_address = it->second;
    this->_size         = nt_header->OptionalHeader.SizeOfImage;

    auto section = IMAGE_FIRST_SECTION(nt_header);

    for (auto i = 0; i < nt_header->FileHeader.NumberOfSections; i++, section++)
    {
        const auto is_executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;

        if (const auto is_readable = (section->Characteristics & IMAGE_SCN_MEM_READ) != 0; is_executable && is_readable)
        {
            const auto start = this->_base_address + section->VirtualAddress;
            const auto size  = std::min(section->SizeOfRawData, section->Misc.VirtualSize);
            const auto data  = reinterpret_cast<std::uint8_t*>(start);

            auto segment = _segments.emplace_back();

            segment.address = start;
            segment.data    = std::vector(&data[0], &data[size]);
        }
    }

    DumpExports(_base_address);
}

arisu::Address arisu::impl::Module::FindPattern(std::string_view pattern) const
{
    for (auto&& segment : _segments)
    {
        if (auto result = pattern::find(segment.data, pattern))
        {
            if (_find_pattern_callback)
                _find_pattern_callback(pattern, result, _base_address);

            if (result.has_value())
                return segment.address + result.value();
        }
    }

    return {};
}

void* arisu::impl::Module::GetProc(const std::string_view proc_name) const
{
    const auto hash = fnv1a64::hash(proc_name);

    if (const auto it = _exports.find(hash); it != _exports.end())
        return reinterpret_cast<void*>(it->second);

    return nullptr;
}

void* arisu::impl::Module::GetProc(std::uint64_t proc_name_hash) const
{
    if (const auto it = _exports.find(proc_name_hash); it != _exports.end())
        return reinterpret_cast<void*>(it->second);

    return nullptr;
}

void arisu::impl::Module::DumpExports(std::uintptr_t module_base)
{
    const auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_base);

    const auto nt_header = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_base) + dos_header->e_lfanew);

    const auto [export_address_rva, export_size] = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (export_size == 0 || export_address_rva == 0)
        return;

    auto export_directory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(_base_address + export_address_rva);
    const auto names      = reinterpret_cast<uint32_t*>(_base_address + export_directory->AddressOfNames);
    const auto addresses  = reinterpret_cast<uint32_t*>(_base_address + export_directory->AddressOfFunctions);
    const auto ordinals   = reinterpret_cast<std::uint16_t*>(_base_address + export_directory->AddressOfNameOrdinals);

    for (auto i = 0ull; i < export_directory->NumberOfNames; i++)
    {
        const auto export_name = reinterpret_cast<const char*>(_base_address + names[i]);
        const auto address = module_base + addresses[ordinals[i]];

        if (address >= reinterpret_cast<uintptr_t>(export_directory) && address < reinterpret_cast<uintptr_t>(export_directory) + export_size)
            continue;

        auto hash      = fnv1a64::hash(export_name);
        _exports[hash] = address;
    }
}

std::uintptr_t arisu::impl::Module::GetExport(std::uintptr_t module_base, const std::string& name)
{
    // todo: get forward export without duplicating code
    return 0;
}
#endif
