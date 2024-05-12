#pragma once
#include "KeyValues.h"
#include "lib/module.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

class CGameConfig
{
  public:
    CGameConfig(const std::string& gameDir, const std::string& path);
    ~CGameConfig();

    bool Init(IFileSystem* filesystem);
    const char* GetLibrary(const std::string& name);
    const char* GetSignature(const std::string& name);
    const char* GetSymbol(const char* name);
    int GetOffset(const std::string& name);
    Module* GetModule(const char* name);
    bool IsSymbol(const char* name);
    void* ResolveSignature(const char* name);

  private:
    std::string m_szGameDir;
    std::string m_szPath;
    KeyValues* m_pKeyValues;
    std::unordered_map<std::string, int> m_umOffsets;
    std::unordered_map<std::string, std::string> m_umSignatures;
    std::unordered_map<std::string, std::string> m_umLibraries;
};

inline std::unique_ptr<CGameConfig> g_pGameConfig;