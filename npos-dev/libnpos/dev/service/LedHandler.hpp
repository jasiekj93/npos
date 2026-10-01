#pragma once

/**
 * @file LedHandler.hpp
 * @author Adrian Szczepanski
 * @date 01-10-2026
 */

#include <etl/optional.h>
#include <etl/string_view.h>
#include <etl/utility.h>

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/message/Led.hpp>
#include <libnpos/dev/message/Open.hpp>
#include <libnpos/dev/hal/Rtc.hpp>

// namespace npos::dev::service
// {
//     class LedHandler : public ipc::Service
//     {