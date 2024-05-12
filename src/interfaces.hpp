#pragma once

#include "entitysystem.h"
#include "irecipientfilter.h"
#include "iserver.h"
#include "tier1/convar.h"
#include <igameevents.h>

inline IVEngineServer2* g_pEngineServer2 = nullptr;
inline CGlobalVars* gpGlobals = nullptr;
inline CGameEntitySystem* g_pEntitySystem = nullptr;
