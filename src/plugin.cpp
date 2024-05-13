#include "plugin.hpp"
#include "addresses.hpp"
#include "entity2/entitykeyvalues.h"
#include "gameconfig.hpp"
#include "interfaces.hpp"
#include "modules.hpp"
#include "sdk/schema.hpp"
#include "gamesystem.hpp"

#include "sdk/entity/ccsplayercontroller.h"

#include <spdlog/spdlog.h>

DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_CS2S, "Plogon", 0, LV_MAX, Color());

class GameSessionConfiguration_t
{
};

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK4_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, CPlayerSlot, bool, const char*, uint64);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, ENetworkDisconnectionReason, const char*, uint64, const char*);
SH_DECL_HOOK4_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, CPlayerSlot, char const*, int, uint64);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, CPlayerSlot);
SH_DECL_HOOK6_void(IServerGameClients, OnClientConnected, SH_NOATTRIB, 0, CPlayerSlot, const char*, uint64, const char*, const char*, bool);
SH_DECL_HOOK6(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, CPlayerSlot, const char*, uint64, const char*, bool, CBufferString*);
SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent*, bool);
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, CPlayerSlot, const CCommand&);
SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t&, ISource2WorldSession*, const char*);

Plugin g_plugin(LOG_CS2S);
PLUGIN_EXPOSE(Plugin, g_plugin);
INetworkGameServer* g_pNetworkGameServer = nullptr;

Plugin::Plugin(const LoggingChannelID_t logging)
{
    _log = logging;
}

Plugin::~Plugin() = default;

CGameEntitySystem* GameEntitySystem()
{
    static int offset = g_pGameConfig->GetOffset("GameEntitySystem");
    return *reinterpret_cast<CGameEntitySystem**>(reinterpret_cast<uintptr_t>(g_pGameResourceServiceServer) + offset);
}

bool Plugin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS()

    mods::Init();

    _metamod = ismm;

    ismm->AddListener(this, this);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngineServer2, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2ServerConfig, ISource2ServerConfig, SOURCE2SERVERCONFIG_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameEntities, ISource2GameEntities, SOURCE2GAMEENTITIES_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameClients, IServerGameClients, SOURCE2GAMECLIENTS_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

    CBufferStringGrowable<256> gamedirpath;
    g_pEngineServer2->GetGameDir(gamedirpath);

    std::string_view gamedirpath_sv = gamedirpath.Get();
    std::string_view game_name = gamedirpath_sv.substr(gamedirpath_sv.find_last_of("/\\") + 1);
    spdlog::info("GameDirPath: {}, {}", gamedirpath_sv, game_name);

    g_pGameConfig = std::make_unique<CGameConfig>(game_name.data(), "addons/plugin/plugin.game.txt");
    if (!g_pGameConfig->Init(g_pFullFileSystem))
        return false;

    if (!addresses::Initialize())
    {
        spdlog::error("Failed to initialize addresses");
        return false;
    }

    if (!InitGameSystems())
    {
        spdlog::error("Failed to initialize GameSystem");
        return false;
    }

    SH_ADD_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &Plugin::Hook_GameFrame), true);
    SH_ADD_HOOK(IServerGameClients, ClientActive, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientActive), true);
    SH_ADD_HOOK(IServerGameClients, ClientDisconnect, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientDisconnect), true);
    SH_ADD_HOOK(IServerGameClients, ClientPutInServer, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientPutInServer), true);
    SH_ADD_HOOK(IServerGameClients, ClientSettingsChanged, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientSettingsChanged), false);
    SH_ADD_HOOK(IServerGameClients, OnClientConnected, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_OnClientConnected), false);
    SH_ADD_HOOK(IServerGameClients, ClientConnect, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientConnect), false);
    SH_ADD_HOOK(IServerGameClients, ClientCommand, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientCommand), false);
    SH_ADD_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService, SH_MEMBER(this, &Plugin::Hook_StartupServer), true);

    ConVar_Register(FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL);

    if (late)
    {
        g_pNetworkGameServer = g_pNetworkServerService->GetIGameServer();
        g_pEntitySystem = GameEntitySystem();
        gpGlobals = g_pNetworkGameServer->GetGlobals();
    }

    return true;
}

bool Plugin::Unload(char* error, size_t maxlen)
{
    SH_REMOVE_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &Plugin::Hook_GameFrame), true);
    SH_REMOVE_HOOK(IServerGameClients, ClientActive, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientActive), true);
    SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientDisconnect), true);
    SH_REMOVE_HOOK(IServerGameClients, ClientPutInServer, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientPutInServer), true);
    SH_REMOVE_HOOK(IServerGameClients, ClientSettingsChanged, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientSettingsChanged), false);
    SH_REMOVE_HOOK(IServerGameClients, OnClientConnected, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_OnClientConnected), false);
    SH_REMOVE_HOOK(IServerGameClients, ClientConnect, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientConnect), false);
    SH_REMOVE_HOOK(IServerGameClients, ClientCommand, g_pSource2GameClients, SH_MEMBER(this, &Plugin::Hook_ClientCommand), false);
    SH_REMOVE_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService, SH_MEMBER(this, &Plugin::Hook_StartupServer), true);

    return ISmmPlugin::Unload(error, maxlen);
}

void Plugin::FireGameEvent(IGameEvent* event)
{
}

void Plugin::Hook_ClientActive(CPlayerSlot slot, bool bLoadGame, const char* pszName, uint64 xuid)
{
    META_CONPRINTF("Hook_ClientActive(%d, %d, \"%s\", %d)\n", slot, bLoadGame, pszName, xuid);
}

void Plugin::Hook_ClientCommand(CPlayerSlot slot, const CCommand& args)
{
    CCSPlayerController* player = CCSPlayerController::FromSlot(slot);
    // META_CONPRINTF("Hook_ClientCommand(%d, \"%s\")\n", slot, args.GetCommandString());
}

void Plugin::Hook_ClientSettingsChanged(CPlayerSlot slot)
{
    // META_CONPRINTF("Hook_ClientSettingsChanged(%d)\n", slot);
}

void Plugin::Hook_OnClientConnected(CPlayerSlot slot, const char* pszName, uint64 xuid, const char* pszNetworkID, const char* pszAddress, bool bFakePlayer)
{
    // META_CONPRINTF("Hook_OnClientConnected(%d, \"%s\", %d, \"%s\", \"%s\", %d)\n", slot, pszName, xuid, pszNetworkID, pszAddress, bFakePlayer);
}

bool Plugin::Hook_ClientConnect(CPlayerSlot slot, const char* pszName, uint64 xuid, const char* pszNetworkID, bool unk1, CBufferString* pRejectReason)
{
    // META_CONPRINTF("Hook_ClientConnect(%d, \"%s\", %d, \"%s\", %d, \"%s\")\n", slot, pszName, xuid, pszNetworkID, unk1, pRejectReason->ToGrowable()->Get());

    RETURN_META_VALUE(MRES_IGNORED, true);
}

void Plugin::Hook_ClientPutInServer(CPlayerSlot slot, char const* pszName, int type, uint64 xuid)
{
    // META_CONPRINTF("Hook_ClientPutInServer(%d, \"%s\", %d, %d, %d)\n", slot, pszName, type, xuid);
}

void Plugin::Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char* pszName, uint64 xuid, const char* pszNetworkID)
{
    // META_CONPRINTF("Hook_ClientDisconnect(%d, %d, \"%s\", %d, \"%s\")\n", slot, reason, pszName, xuid, pszNetworkID);
}

void Plugin::Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick)
{
    /**
     * simulating:
     * ***********
     * true  | game is ticking
     * false | game is not ticking
     */
}

void Plugin::OnLevelInit(char const* pMapName, char const* pMapEntities, char const* pOldLevel, char const* pLandmarkName, bool loadGame, bool background)
{
    // META_CONPRINTF("OnLevelInit(%s)\n", pMapName);
}

void Plugin::OnLevelShutdown()
{
    // META_CONPRINTF("OnLevelShutdown()\n");
}

void Plugin::Hook_StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession* pSession, const char* pszMapName)
{
    g_pNetworkGameServer = g_pNetworkServerService->GetIGameServer();
    g_pEntitySystem = GameEntitySystem();
    gpGlobals = g_pNetworkGameServer->GetGlobals();
}