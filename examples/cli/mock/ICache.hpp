#pragma once

/**
 * @file ICache.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */
#include <iostream>

#include <libnpos/fs/hal/ICache.hpp>

namespace mock
{
    class ICache : public npos::fs::hal::ICache
    {
    public:
        bool init() override
        {
            return true;
        }

        bool enable() override 
        {
            return true;
        }

        bool disable() override 
        {
            return true;
        }
    };
}