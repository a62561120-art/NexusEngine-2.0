#pragma once
#include <cstdint>
#include <string>
#include <functional>

namespace Nova {

// Engine-wide integer types
using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8   = int8_t;
using int16  = int16_t;
using int32  = int32_t;
using int64  = int64_t;

// Unique ID type for engine objects
using EntityID = uint64_t;
constexpr EntityID INVALID_ID = 0;

// Simple incrementing UUID generator
class UUID {
public:
    static EntityID Generate() {
        static EntityID counter = 1;
        return counter++;
    }
};

// Type alias for component type IDs
using ComponentTypeID = uint32;

// Counter for component type IDs - defined in Types.cpp
inline uint32 s_componentTypeCounter = 1;

// Tag type to uniquely identify component types at compile time
template<typename T>
ComponentTypeID GetComponentTypeID() {
    static ComponentTypeID id = s_componentTypeCounter++;
    return id;
}

} // namespace Nova
