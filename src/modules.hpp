#pragma once

#include "lib/module.hpp"

namespace mods
{
    void Init();

    inline arisu::impl::Module server{};
    inline arisu::impl::Module engine{};
    inline arisu::impl::Module tier0{};
    inline arisu::impl::Module network{};
    inline arisu::impl::Module schema_system{};
}
