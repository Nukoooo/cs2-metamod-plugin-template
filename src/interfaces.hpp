#pragma once

#include "iserver.h"
#include "tier1/convar.h"
#include "irecipientfilter.h"
#include <igameevents.h>

namespace interfaces {
    inline IServerGameDLL *server = nullptr;
    inline IServerGameClients *gameclients = nullptr;
    inline IVEngineServer *engine = nullptr;
    inline IGameEventManager2 *gameevents = nullptr;
    inline ICvar *icvar = nullptr;
}
