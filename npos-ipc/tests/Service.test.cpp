/**
 * @file Service.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/RecordingApi.hpp"
#include "mock/FakeService.hpp"

#include <CppUTest/TestHarness.h>

TEST_GROUP(Service)
{
    mock::RecordingApi api;
    mock::FakeService service{api};
};

TEST(Service, ExposesItsApi)
{
    POINTERS_EQUAL(&api, &service.getApi());
}

TEST(Service, OidUsesPortOfApiAndAssignedId)
{
    service.setId(5);

    auto oid = service.getOid();

    CHECK_EQUAL(3, oid.portId);
    CHECK_EQUAL(5, oid.serviceId);
}

TEST(Service, OidHasNullServiceUntilIdAssigned)
{
    CHECK_EQUAL(npos::ipc::NULL_SERVICE, service.getOid().serviceId);
}

TEST(Service, InitializeDefaultsToNothing)
{
    struct Minimal : public npos::ipc::Service
    {
        using npos::ipc::Service::Service;
        void handle(npos::ipc::Message&) override {}
        bool accepts(npos::ipc::Message::Type) const override { return false; }
    };

    Minimal minimal(api);
    minimal.initialize();

    CHECK_EQUAL(0, api.sent.size());
}

TEST(Service, ApiRecordsSendAndRespond)
{
    npos::ipc::Message message;
    message.type = 7;

    service.getApi().send(message);
    service.getApi().respond(message);

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(7, api.sent[0].type);
}
