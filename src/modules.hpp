#pragma once

#include "lib/module.hpp"

namespace mods
{
    void Init();

    inline Module server {};
    inline Module engine {};
    inline Module tier0 {};
    inline Module network_system {};
    inline Module schema_system {};
}
