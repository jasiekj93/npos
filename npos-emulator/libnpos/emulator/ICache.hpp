#pragma once

/**
 * @file ICache.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */
#include <iostream>

#include <libnpos/fs/hal/ICache.hpp>

namespace npos::emulator
{
    class ICache : public npos::fs::hal::ICache
    {
    public:
        explicit ICache(bool verbose = false) : verbose(verbose) {}

        bool init() override
        {
            if(verbose)
                std::cout << "Emulator: ICache init" << std::endl;
            return true;
        }

        bool enable() override 
        {
            if(verbose)
                std::cout << "Emulator: ICache enable" << std::endl;
            return true;
        }

        bool disable() override 
        {
            if(verbose)
                std::cout << "Emulator: ICache disable" << std::endl;
            return true;
        }

        bool verbose;
    };
}
