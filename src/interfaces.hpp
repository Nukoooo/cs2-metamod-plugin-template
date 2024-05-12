#pragma once

#include <igameevents.h>
#include "irecipientfilter.h"
#include "iserver.h"
#include "tier1/convar.h"

class CSchemaSystem;

namespace interfaces
{
    inline IServerGameDLL* server = nullptr;
    inline IServerGameClients* gameclients = nullptr;
    inline IVEngineServer* engine = nullptr;
    inline IGameEventManager2* gameevents = nullptr;
    inline ICvar* icvar = nullptr;
    inline CSchemaSystem* schemaSystem = nullptr;
} // namespace interfaces
