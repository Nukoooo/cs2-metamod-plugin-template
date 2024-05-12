#pragma once

#include "../schema.hpp"
#include "ehandle.h"

struct CGameSceneNode
{
    SCHEMA_FIELD(CEntityInstance *, m_pOwner)
    SCHEMA_FIELD(CGameSceneNode *, m_pParent)
    SCHEMA_FIELD(CGameSceneNode *, m_pChild)
    SCHEMA_FIELD(CNetworkOriginCellCoordQuantizedVector, m_vecOrigin)
    SCHEMA_FIELD(QAngle, m_angRotation)
    SCHEMA_FIELD(float, m_flScale)
    SCHEMA_FIELD(float, m_flAbsScale)
    SCHEMA_FIELD(Vector, m_vecAbsOrigin)
    SCHEMA_FIELD(QAngle, m_angAbsRotation)
    SCHEMA_FIELD(Vector, m_vRenderOrigin)
};


struct CBodyComponent
{

};

struct C_BaseEntity
{

};
