#pragma once

#include <ISmmPlugin.h>

#include <igameevents.h>
#include <sh_vector.h>
#include <tier0/logging.h>
#include <igameeventsystem.h> // Required by igameevents.h
#include <igameevents.h>

class Plugin : public ISmmPlugin, public IMetamodListener, public IGameEventListener2
{
  public:
    explicit Plugin(LoggingChannelID_t logging);

    ~Plugin() override;

    // no move or copy
    Plugin(Plugin&&) = delete;

    Plugin(const Plugin&) = delete;

    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) override;

    bool Unload(char* error, size_t maxlen) override;

    const char* GetAuthor() override
    {
        return "Nuko";
    }

    const char* GetName() override
    {
        return "Plogon";
    }

    const char* GetDescription() override
    {
        return "Plogon";
    }

    const char* GetURL() override
    {
        return "https://github.com/Nukoooo";
    }

    const char* GetLicense() override
    {
        return "MIT";
    }

    const char* GetVersion() override
    {
        return "0.1";
    }

    const char* GetDate() override
    {
        return "1970-01-01";
    }

    const char* GetLogTag() override
    {
        return "plogon";
    }

    void FireGameEvent(IGameEvent* event) override;
    void OnLevelInit(char const* pMapName, char const* pMapEntities, char const* pOldLevel, char const* pLandmarkName, bool loadGame, bool background) override;
    void OnLevelShutdown() override;
    void Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick);
    void Hook_ClientActive(CPlayerSlot slot, bool bLoadGame, const char* pszName, uint64 xuid);
    void Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char* pszName, uint64 xuid, const char* pszNetworkID);
    void Hook_ClientPutInServer(CPlayerSlot slot, char const* pszName, int type, uint64 xuid);
    void Hook_ClientSettingsChanged(CPlayerSlot slot);
    void Hook_OnClientConnected(CPlayerSlot slot, const char* pszName, uint64 xuid, const char* pszNetworkID, const char* pszAddress, bool bFakePlayer);
    bool Hook_ClientConnect(CPlayerSlot slot, const char* pszName, uint64 xuid, const char* pszNetworkID, bool unk1, CBufferString* pRejectReason);
    void Hook_ClientCommand(CPlayerSlot nSlot, const CCommand& cmd);
    void Hook_StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession*, const char*);

  private:
    ISmmAPI* _metamod{ };
    LoggingChannelID_t _log;
};

PLUGIN_GLOBALVARS();
