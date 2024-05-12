#include "modules.hpp"
#include "gameconfig.hpp"
#include "addresses.hpp"

bool addresses::Initialize()
{
    if (CBaseModelEntity_SetModel = (decltype(CBaseModelEntity_SetModel))(g_pGameConfig->ResolveSignature("CBaseModelEntity_SetModel")); CBaseModelEntity_SetModel == nullptr)
        return false;
    if (NetworkStateChanged = (decltype(NetworkStateChanged))(g_pGameConfig->ResolveSignature("NetworkStateChanged")); NetworkStateChanged == nullptr)
        return false;
    if (StateChanged = (decltype(StateChanged))(g_pGameConfig->ResolveSignature("StateChanged")); StateChanged == nullptr)
        return false;

    return true;
}
