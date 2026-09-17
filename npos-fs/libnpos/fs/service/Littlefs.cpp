#include "Littlefs.hpp"

using namespace npos;
using namespace npos::fs;
using namespace npos::fs::service;

Littlefs::Littlefs(hal::Flash& flash, os::message::Bus& bus)
    : os::Service(bus)
    , flash(flash)
{
}

void Littlefs::initalize()
{
    //TODO
}

void Littlefs::onReceive(const ipc::Message& message)
{
    //TODO
}

bool Littlefs::accepts(ipc::Message::Id id) const
{
    //TODO
    return false;
}
