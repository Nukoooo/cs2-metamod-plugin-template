#pragma once

#include "entitysystem.h"
#include "irecipientfilter.h"
#include "iserver.h"
#include "tier1/convar.h"
#include <igameevents.h>
#include "igameeventsystem.h"

inline IVEngineServer2* g_pEngineServer2 = nullptr;
inline CGlobalVars* gpGlobals = nullptr;
inline CGameEntitySystem* g_pEntitySystem = nullptr;
inline INetworkGameServer* g_pNetworkGameServer = nullptr;
inline IGameEventSystem* g_pGameEventSystem = nullptr;