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
        void handleRequest(npos::ipc::Message&) override {}
        void handleResponse(npos::ipc::Message&) override {}
        bool acceptsRequest(npos::ipc::Message::Type) const override { return false; }
    };

    Minimal minimal(api);
    minimal.initialize();

    CHECK_EQUAL(0, api.sent.size());
}

TEST(Service, ApiRecordsSendAndRespond)
{
    npos::ipc::Message message;
    message.type = 7;

    service.setId(5);

    service.getApi().send(service, message);
    service.getApi().respond(message);

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(7, api.sent[0].type);
}

TEST(Service, ApiSendStampsSenderOidOfService)
{
    npos::ipc::Message message;
    service.setId(5);

    service.getApi().send(service, message);

    CHECK_EQUAL(3, api.sent[0].sender.portId);
    CHECK_EQUAL(5, api.sent[0].sender.serviceId);
}

TEST(Service, UnknownRequestIsAnsweredWithNotSupportedByDefault)
{
    npos::ipc::Message message;
    message.output.status = 0;

    service.handleUnknownRequest(message);

    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, message.output.status);
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, api.responded[0].output.status);
}
