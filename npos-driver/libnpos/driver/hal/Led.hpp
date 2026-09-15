#pragma once

#include <etl/string.h>

namespace npos::driver::hal
{
    class Led
    {
    public:
        using Color = etl::string_view;

        virtual ~Led() = default;

        virtual void on(Color) = 0;
        virtual void off(Color) = 0;
        virtual void toggle(Color) = 0;
    };
}