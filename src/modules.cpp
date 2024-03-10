#include "modules.hpp"

void mods::Init()
{
    server        = arisu::impl::Module("server");
    engine        = arisu::impl::Module("engine2");
    network       = arisu::impl::Module("networksystem");
    tier0         = arisu::impl::Module("tier0");
    schema_system = arisu::impl::Module("schemasystem");
}
