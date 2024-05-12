#include "plugin.hpp"
#include "entity2/entitykeyvalues.h"
#include "interfaces.hpp"
#include "modules.hpp"
#include "protobuf/cs_usercmd.pb.h"

#include <cstdio>
#include <format>
#include <safetyhook.hpp>

DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_CS2S, "Plogon", 0, LV_MAX, Color());

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);

SH_DECL_HOOK4_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, CPlayerSlot, bool, const char *, uint64);

SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, ENetworkDisconnectionReason, const char *, uint64, const char *);

SH_DECL_HOOK4_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, CPlayerSlot, char const *, int, uint64);

SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, CPlayerSlot);

SH_DECL_HOOK6_void(IServerGameClients, OnClientConnected, SH_NOATTRIB, 0, CPlayerSlot, const char *, uint64, const char *, const char *, bool);

SH_DECL_HOOK6(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, CPlayerSlot, const char *, uint64, const char *, bool, CBufferString *);

SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, CPlayerSlot, const CCommand &);

Plugin g_plugin(LOG_CS2S);
PLUGIN_EXPOSE(Plugin, g_plugin);

Plugin::Plugin(const LoggingChannelID_t logging) { _log = logging; }

Plugin::~Plugin() = default;

CGameEntitySystem *GameEntitySystem()
{
    static auto address = mods::server.FindPattern("48 8B 1D ? ? ? ? 48 89 1D").rel(3).cast<CGameEntitySystem **>();
    return *address;
}

safetyhook::InlineHook CCSPlayer_MovementServices_RunCommand{};

bool Plugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS()

    mods::Init();

    _metamod = ismm;

    ismm->AddListener(this, this);
    GET_V_IFACE_CURRENT(GetEngineFactory, interfaces::engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER)
    GET_V_IFACE_CURRENT(GetEngineFactory, interfaces::icvar, ICvar, CVAR_INTERFACE_VERSION)
    GET_V_IFACE_ANY(GetServerFactory, interfaces::server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL)
    GET_V_IFACE_ANY(GetServerFactory, interfaces::gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS)
    GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION)

    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, interfaces::server, this, &Plugin::Hook_GameFrame, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientActive, interfaces::gameclients, this, &Plugin::Hook_ClientActive, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, interfaces::gameclients, this, &Plugin::Hook_ClientDisconnect, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, interfaces::gameclients, this, &Plugin::Hook_ClientPutInServer, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, interfaces::gameclients, this, &Plugin::Hook_ClientSettingsChanged, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, OnClientConnected, interfaces::gameclients, this, &Plugin::Hook_OnClientConnected, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, interfaces::gameclients, this, &Plugin::Hook_ClientConnect, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, interfaces::gameclients, this, &Plugin::Hook_ClientCommand, false);

    g_pCVar = interfaces::icvar;
    ConVar_Register(FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL);

    auto result = mods::server.FindVtable("CCSPlayerPawn");
    if (result.is_valid())
    {
        std::cout << std::format("Found vtable CCSPlayerPawn. {:#x}", result.ptr) << std::endl;
    }
    else
    {
        printf("Cannot find CCSPlayerPawn\n");
    }

    return true;
}

bool Plugin::Unload(char *error, size_t maxlen)
{
    CCSPlayer_MovementServices_RunCommand.reset();

    return ISmmPlugin::Unload(error, maxlen);
}

void Plugin::FireGameEvent(IGameEvent *event) {}

void Plugin::Hook_ClientActive(CPlayerSlot slot, bool bLoadGame, const char *pszName, uint64 xuid)
{
    META_CONPRINTF("Hook_ClientActive(%d, %d, \"%s\", %d)\n", slot, bLoadGame, pszName, xuid);
}

void Plugin::Hook_ClientCommand(CPlayerSlot slot, const CCommand &args)
{
    // META_CONPRINTF("Hook_ClientCommand(%d, \"%s\")\n", slot, args.GetCommandString());
}

void Plugin::Hook_ClientSettingsChanged(CPlayerSlot slot)
{
    // META_CONPRINTF("Hook_ClientSettingsChanged(%d)\n", slot);
}

void Plugin::Hook_OnClientConnected(CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, const char *pszAddress, bool bFakePlayer)
{
    // META_CONPRINTF("Hook_OnClientConnected(%d, \"%s\", %d, \"%s\", \"%s\", %d)\n", slot, pszName, xuid, pszNetworkID, pszAddress, bFakePlayer);
}

bool Plugin::Hook_ClientConnect(CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, bool unk1, CBufferString *pRejectReason)
{
    // META_CONPRINTF("Hook_ClientConnect(%d, \"%s\", %d, \"%s\", %d, \"%s\")\n", slot, pszName, xuid, pszNetworkID, unk1, pRejectReason->ToGrowable()->Get());

    RETURN_META_VALUE(MRES_IGNORED, true);
}

void Plugin::Hook_ClientPutInServer(CPlayerSlot slot, char const *pszName, int type, uint64 xuid)
{
    // META_CONPRINTF("Hook_ClientPutInServer(%d, \"%s\", %d, %d, %d)\n", slot, pszName, type, xuid);
}

void Plugin::Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char *pszName, uint64 xuid, const char *pszNetworkID)
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

void Plugin::OnLevelInit(char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
    // META_CONPRINTF("OnLevelInit(%s)\n", pMapName);
}

void Plugin::OnLevelShutdown()
{
    // META_CONPRINTF("OnLevelShutdown()\n");
}
