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
        bool enable() override 
        {
            std::cout << "Emulator: ICache enable" << std::endl;
            return true;
        }

        bool disable() override 
        {
            std::cout << "Emulator: ICache disable" << std::endl;
            return true;
        }
    };
}