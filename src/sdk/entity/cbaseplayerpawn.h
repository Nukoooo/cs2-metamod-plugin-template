#pragma once

#include "../../gameconfig.hpp"
#include "cbaseentity.h"
#include "cbasemodelentity.h"
#include "services.h"

class CBasePlayerPawn : public CBaseModelEntity
{
  public:
    DECLARE_SCHEMA_CLASS(CBasePlayerPawn);

    SCHEMA_FIELD(CPlayer_MovementServices*, m_pMovementServices)
    SCHEMA_FIELD(CPlayer_WeaponServices*, m_pWeaponServices)
    SCHEMA_FIELD(CCSPlayer_ItemServices*, m_pItemServices)
    SCHEMA_FIELD(CPlayer_ObserverServices*, m_pObserverServices)
    SCHEMA_FIELD(CHandle<CBasePlayerController>, m_hController)

    void CommitSuicide(bool bExplode, bool bForce)
    {
        static int offset = g_pGameConfig->GetOffset("CBasePlayerPawn_CommitSuicide");
        Memory::VirtualCall<void>(this, offset, bExplode, bForce);
    }

    CBasePlayerController *GetController() { return m_hController.Get(); }
};