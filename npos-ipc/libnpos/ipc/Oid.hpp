#pragma once

/**
 * @file Oid.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <cstdint>

namespace npos::ipc
{
    using PortId = uint8_t;
    using ServiceId = uint8_t;

    struct Oid
    {
        PortId portId;
        ServiceId serviceId;
    };

    static constexpr PortId NULL_PORT = 255;
    static constexpr ServiceId NULL_SERVICE = 255;

    static constexpr Oid NULL_OID = {NULL_PORT, NULL_SERVICE};
}