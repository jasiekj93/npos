#pragma once

/**
 * @file ICache.hpp
 * @author Adrian Szczepanski
 * @date 02-09-2026
 */

namespace npos::fs::hal
{
    class ICache
    {
    public:
        virtual ~ICache() = default;

        virtual bool init() = 0;

        virtual bool enable() = 0;
        virtual bool disable() = 0;
    };
}