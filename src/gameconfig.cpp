#include "gameconfig.hpp"
#include "modules.hpp"
#include <spdlog/spdlog.h>

CGameConfig::CGameConfig(const std::string& gameDir, const std::string& path)
{
    this->m_szGameDir = gameDir;
    this->m_szPath = path;
    this->m_pKeyValues = new KeyValues("Games");
}

CGameConfig::~CGameConfig()
{
    delete m_pKeyValues;
}

bool CGameConfig::Init(IFileSystem* filesystem)
{
    if (!m_pKeyValues->LoadFromFile(filesystem, m_szPath.c_str(), nullptr))
    {
        spdlog::error("Failed to load gamedata file. {}", m_szPath);
        return false;
    }

    KeyValues* game = m_pKeyValues->FindKey(m_szGameDir.c_str(), false);
    if (game)
    {
#if defined LINUX
        constexpr const char* platform = "linux";
#else
        constexpr const char* platform = "windows";
#endif

        KeyValues* offsets = game->FindKey("Offsets", false);
        if (offsets)
        {
            FOR_EACH_SUBKEY(offsets, it)
            {
                m_umOffsets[it->GetName()] = it->GetInt(platform, -1);
            }
        }

        KeyValues* signatures = game->FindKey("Signatures", false);
        if (signatures)
        {
            FOR_EACH_SUBKEY(signatures, it)
            {
                m_umLibraries[it->GetName()] = std::string(it->GetString("library"));
                m_umSignatures[it->GetName()] = std::string(it->GetString(platform));
            }
        }

        spdlog::info("Loaded {} offsets, {} signatures", m_umOffsets.size(), m_umSignatures.size());
        return true;
    }

    spdlog::error("Failed to find game {} in gamedata.", m_szGameDir);
    return false;
}

const char* CGameConfig::GetSignature(const std::string& name)
{
    auto it = m_umSignatures.find(name);
    if (it == m_umSignatures.end())
    {
        return nullptr;
    }
    return it->second.c_str();
}

int CGameConfig::GetOffset(const std::string& name)
{
    auto it = m_umOffsets.find(name);
    if (it == m_umOffsets.end())
    {
        return -1;
    }
    return it->second;
}

const char* CGameConfig::GetLibrary(const std::string& name)
{
    auto it = m_umLibraries.find(name);
    if (it == m_umLibraries.end())
    {
        return nullptr;
    }
    return it->second.c_str();
}

Module* CGameConfig::GetModule(const char* name)
{
    const char* library = this->GetLibrary(name);
    if (!library)
        return nullptr;

    if (strcmp(library, "engine") == 0)
        return &mods::engine;
    else if (strcmp(library, "server") == 0)
        return &mods::server;
    else if (strcmp(library, "tier0") == 0)
        return &mods::tier0;
    else if (strcmp(library, "networksystem") == 0)
        return &mods::network_system;

    return nullptr;
}

bool CGameConfig::IsSymbol(const char* name)
{
    const char* sigOrSymbol = this->GetSignature(name);
    if (!sigOrSymbol || strlen(sigOrSymbol) <= 0)
    {
        spdlog::error("Missing signature or symbol {}", name);
        return false;
    }
    return sigOrSymbol[0] == '@';
}

const char* CGameConfig::GetSymbol(const char* name)
{
    const char* symbol = this->GetSignature(name);

    if (!symbol || strlen(symbol) <= 1)
    {
        spdlog::error("Missing symbol {}", name);
        return nullptr;
    }
    return symbol + 1;
}

void* CGameConfig::ResolveSignature(const char* name)
{
    auto module = this->GetModule(name);
    if (!module)
    {
        spdlog::error("Invalid module {}", name);
        return nullptr;
    }

    if (this->IsSymbol(name))
    {
        const char* symbol = this->GetSymbol(name);
        if (!symbol)
        {
            spdlog::error("Invalid symbol {}", name);
            return nullptr;
        }
        return module->GetProc(symbol);
    }

    const char* signature = this->GetSignature(name);
    if (!signature)
    {
        spdlog::error("Failed to find signature for {}", name);

        return nullptr;
    }

    auto result = module->FindPattern(signature);
    if (result.is_valid())
    {
        spdlog::info("{} -> {:#x}", name, result.ptr);
        return result;
    }

    spdlog::error("FindPattern doesn't have any result for {}", name);

    return nullptr;
}