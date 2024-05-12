#pragma once

#include <cstdint>
#include <optional>
#include "../addresses.hpp"
#include "../lib/memory.hpp"
#include "../lib/fnv1a_hash.hpp"

class Z_CBaseEntity;

struct SchemaKey
{
    int32_t offset;
    bool networked;
};

namespace schema
{
    SchemaKey GetOffset(const char* className, std::uint32_t classKey, const char* memberName, std::uint32_t memberKey);
    int16_t FindChainOffset(const char *className);
}

void SetStateChanged(Z_CBaseEntity* pEntity, int offset);

#define DECLARE_SCHEMA_CLASS_BASE(className, isStruct)                                                                                                                                                                                                         \
    typedef className ThisClass;                                                                                                                                                                                                                               \
    static constexpr const char* ThisClassName = #className;                                                                                                                                                                                                   \
    static constexpr bool IsStruct             = isStruct;

#define DECLARE_SCHEMA_CLASS(className) DECLARE_SCHEMA_CLASS_BASE(className, false)

// Use this for classes that can be wholly included within other classes (like CCollisionProperty within CBaseModelEntity)
#define DECLARE_SCHEMA_CLASS_INLINE(className) \
	DECLARE_SCHEMA_CLASS_BASE(className, true)

#define SCHEMA_FIELD_OFFSET(type, varName, extra_offset)                                                                                                                                                                                                       \
    class varName##_prop                                                                                                                                                                                                                                       \
    {                                                                                                                                                                                                                                                          \
      public:                                                                                                                                                                                                                                                  \
        std::add_lvalue_reference_t<type> Get()                                                                                                                                                                                                                \
        {                                                                                                                                                                                                                                                      \
            static constexpr auto datatable_hash = fnv1a32::hash(ThisClassName);                                                                                                                                                                               \
            static constexpr auto prop_hash      = fnv1a32::hash(#varName);                                                                                                                                                                                    \
                                                                                                                                                                                                                                                               \
            static const auto m_key = schema::GetOffset(ThisClassName, datatable_hash, #varName, prop_hash);                                                                                                                                                   \
                                                                                                                                                                                                                                                               \
            static const size_t offset = offsetof(ThisClass, varName);                                                                                                                                                                                         \
            ThisClass* pThisClass      = (ThisClass*)((byte*)this - offset);                                                                                                                                                                                   \
                                                                                                                                                                                                                                                               \
            return *reinterpret_cast<std::add_pointer_t<type>>((uintptr_t)(pThisClass) + m_key.offset + extra_offset);                                                                                                                                         \
        }                                                                                                                                                                                                                                                      \
        void Set(type val)                                                                                                                                                                                                                                     \
        {                                                                                                                                                                                                                                                      \
            static constexpr auto datatable_hash = fnv1a32::hash(ThisClassName);                                                                                                                                                                               \
            static constexpr auto prop_hash      = fnv1a32::hash(#varName);                                                                                                                                                                                    \
                                                                                                                                                                                                                                                               \
            static const auto m_key = schema::GetOffset(ThisClassName, datatable_hash, #varName, prop_hash);                                                                                                                                                   \
                                                                                                                                                                                                                                                               \
            static const auto m_chain = schema::FindChainOffset(ThisClassName);                                                                                                                                                                                \
                                                                                                                                                                                                                                                               \
            static const size_t offset = offsetof(ThisClass, varName);                                                                                                                                                                                         \
            ThisClass* pThisClass      = (ThisClass*)((byte*)this - offset);                                                                                                                                                                                   \
                                                                                                                                                                                                                                                               \
            if (m_chain != 0 && m_key.networked)                                                                                                                                                                                                               \
            {                                                                                                                                                                                                                                                  \
                addresses::NetworkStateChanged((uintptr_t)(pThisClass) + m_chain, m_key.offset + extra_offset, 0xFFFFFFFF);                                                                                                                                    \
            }                                                                                                                                                                                                                                                  \
            else if (m_key.networked)                                                                                                                                                                                                                          \
            {                                                                                                                                                                                                                                                  \
                /* WIP: Works fine for most props, but inlined classes in the middle of a class will                                                                                                                                                           \
                    need to have their this pointer corrected by the offset .*/                                                                                                                                                                                \
                if (!IsStruct)                                                                                                                                                                                                                                 \
                    SetStateChanged((Z_CBaseEntity*)pThisClass, m_key.offset + extra_offset);                                                                                                                                                                  \
                else if (IsPlatformPosix()) /* This is currently broken on windows */                                                                                                                                                                          \
                    Memory::VirtualCall<void>(pThisClass, 1, m_key.offset + extra_offset, 0xFFFFFFFF, 0xFFFF);                                                                                                                                                \
            }                                                                                                                                                                                                                                                  \
            *reinterpret_cast<std::add_pointer_t<type>>((uintptr_t)(pThisClass) + m_key.offset + extra_offset) = val;                                                                                                                                          \
        }                                                                                                                                                                                                                                                      \
        operator std::add_lvalue_reference_t<type>()                                                                                                                                                                                                           \
        {                                                                                                                                                                                                                                                      \
            return Get();                                                                                                                                                                                                                                      \
        }                                                                                                                                                                                                                                                      \
        std::add_lvalue_reference_t<type> operator()()                                                                                                                                                                                                         \
        {                                                                                                                                                                                                                                                      \
            return Get();                                                                                                                                                                                                                                      \
        }                                                                                                                                                                                                                                                      \
        std::add_lvalue_reference_t<type> operator->()                                                                                                                                                                                                         \
        {                                                                                                                                                                                                                                                      \
            return Get();                                                                                                                                                                                                                                      \
        }                                                                                                                                                                                                                                                      \
        void operator()(type val)                                                                                                                                                                                                                              \
        {                                                                                                                                                                                                                                                      \
            Set(val);                                                                                                                                                                                                                                          \
        }                                                                                                                                                                                                                                                      \
        void operator=(type val)                                                                                                                                                                                                                               \
        {                                                                                                                                                                                                                                                      \
            Set(val);                                                                                                                                                                                                                                          \
        }                                                                                                                                                                                                                                                      \
    } varName;

#define SCHEMA_FIELD_POINTER_OFFSET(type, varName, extra_offset)                                                                                                                                                                                               \
    class varName##_prop                                                                                                                                                                                                                                       \
    {                                                                                                                                                                                                                                                          \
      public:                                                                                                                                                                                                                                                  \
        type* Get()                                                                                                                                                                                                                                            \
        {                                                                                                                                                                                                                                                      \
            static constexpr auto datatable_hash = fnv1a32::hash(ThisClassName);                                                                                                                                                                               \
            static constexpr auto prop_hash      = fnv1a32::hash(#varName);                                                                                                                                                                                    \
                                                                                                                                                                                                                                                               \
            static const auto m_key = schema::GetOffset(ThisClassName, datatable_hash, #varName, prop_hash);                                                                                                                                                   \
                                                                                                                                                                                                                                                               \
            static const size_t offset = offsetof(ThisClass, varName);                                                                                                                                                                                         \
            ThisClass* pThisClass      = (ThisClass*)((byte*)this - offset);                                                                                                                                                                                   \
                                                                                                                                                                                                                                                               \
            return reinterpret_cast<std::add_pointer_t<type>>((uintptr_t)(pThisClass) + m_key.offset + extra_offset);                                                                                                                                          \
        }                                                                                                                                                                                                                                                      \
        operator type*()                                                                                                                                                                                                                                       \
        {                                                                                                                                                                                                                                                      \
            return Get();                                                                                                                                                                                                                                      \
        }                                                                                                                                                                                                                                                      \
        type* operator()()                                                                                                                                                                                                                                     \
        {                                                                                                                                                                                                                                                      \
            return Get();                                                                                                                                                                                                                                      \
        }                                                                                                                                                                                                                                                      \
        type* operator->()                                                                                                                                                                                                                                     \
        {                                                                                                                                                                                                                                                      \
            return Get();                                                                                                                                                                                                                                      \
        }                                                                                                                                                                                                                                                      \
    } varName;

// Use this when you want the member's value itself
#define SCHEMA_FIELD(type, varName) SCHEMA_FIELD_OFFSET(type, varName, 0)

// Use this when you want a pointer to a member
#define SCHEMA_FIELD_POINTER(type, varName) SCHEMA_FIELD_POINTER_OFFSET(type, varName, 0)
