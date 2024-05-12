#pragma once

#include "cbaseentity.h"
#include "globaltypes.h"

class CBaseModelEntity : public Z_CBaseEntity
{
  public:
    DECLARE_SCHEMA_CLASS(CBaseModelEntity);

    SCHEMA_FIELD(CCollisionProperty , m_Collision)
    SCHEMA_FIELD(CGlowProperty, m_Glow)
    SCHEMA_FIELD(Color, m_clrRender)
    SCHEMA_FIELD(float, m_flDissolveStartTime)

    void SetModel(const char *szModel)
    {
        addresses::CBaseModelEntity_SetModel(this, szModel);
    }
};