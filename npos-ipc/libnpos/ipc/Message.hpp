#pragma once

/**
 * @file Message.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <cstdint>

namespace npos::ipc
{
    class Message
    {
    public:
        using Id = uint16_t;

        explicit Message(Id id) : id(id) {}

        inline auto getId() const { return id; }

    private:
        Id id;
    };
}