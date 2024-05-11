#include "modules.hpp"

void mods::Init()
{
    server        = Module("server", true);
    engine        = Module("engine2", true);
    tier0         = Module("tier0");
    network       = Module("networksystem");
    schema_system = Module("schemasystem");
}
