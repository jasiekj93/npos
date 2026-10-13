/**
 * @file Oid.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/ipc/Oid.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Oid)
{
};

TEST(Oid, NullOidUsesNullPortAndNullService)
{
    CHECK_EQUAL(npos::ipc::NULL_PORT, npos::ipc::NULL_OID.portId);
    CHECK_EQUAL(npos::ipc::NULL_SERVICE, npos::ipc::NULL_OID.serviceId);
}

TEST(Oid, NullValuesAreTheLastIdentifier)
{
    CHECK_EQUAL(255, npos::ipc::NULL_PORT);
    CHECK_EQUAL(255, npos::ipc::NULL_SERVICE);
}

TEST(Oid, HoldsPortAndService)
{
    npos::ipc::Oid oid = { 4, 9 };

    CHECK_EQUAL(4, oid.portId);
    CHECK_EQUAL(9, oid.serviceId);
}
