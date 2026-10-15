#pragma once

#include <gmock/gmock.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include "S2.h"
#include "S2_external.h"
#include "s2_event.h"
#include "s2_inclusion.h"
#include "s2_keystore.h"
#include "s2_protocol.h"
}

class Libs2Mocks
{
    public:
        MOCK_METHOD(uint8_t, S2_send_frame, (struct S2 *, const s2_connection_t *, uint8_t *, uint16_t));
        MOCK_METHOD(uint8_t, S2_send_data, (struct S2 *, s2_connection_t *, const uint8_t *, uint16_t));
        MOCK_METHOD(uint8_t, S2_network_key_update, (struct S2 *, uint32_t, security_class_t, const uint8_t *, uint8_t));
        MOCK_METHOD(void, s2_event_handler, (zwave_event_t *));
        MOCK_METHOD(uint8_t, s2_inclusion_set_timeout, (struct S2 *, uint32_t));
        MOCK_METHOD(void, s2_inclusion_stop_timeout, ());
        MOCK_METHOD(void, keystore_public_key_read, (uint8_t *));
        MOCK_METHOD(void, keystore_dynamic_public_key_read, (uint8_t *));
        MOCK_METHOD(void, keystore_private_key_read, (uint8_t *));
        MOCK_METHOD(void, keystore_dynamic_private_key_read, (uint8_t *));
        MOCK_METHOD(bool, keystore_network_key_read, (uint8_t, uint8_t *));
        MOCK_METHOD(bool, keystore_network_key_write, (uint8_t, const uint8_t *));
        MOCK_METHOD(bool, keystore_network_key_clear, (uint8_t));
        MOCK_METHOD(void, crypto_scalarmult_curve25519, (uint8_t *, const uint8_t *, const uint8_t *));
        MOCK_METHOD(void, tempkey_extract, (const uint8_t *, const uint8_t *, uint8_t *));
};

extern Libs2Mocks *g_libs2_mocks;

extern "C" void s2_event_handler(zwave_event_t *evt);

void libs2_clear_stubs();
void libs2_stub(const char *name);
bool libs2_is_stubbed(const char *name);
bool libs2_is_stubbed(const char *name, const char *alt);

MATCHER_P2(BufferEq, expected, length, "")
{
    return arg != nullptr && expected != nullptr && memcmp(arg, expected, length) == 0;
}

MATCHER_P(EventMatches, expected, "")
{
    if (arg == nullptr || expected == nullptr) {
        return false;
    }
    if (arg->event_type != expected->event_type) {
        return false;
    }
    switch (expected->event_type) {
        case S2_NODE_INCLUSION_KEX_REPORT_EVENT:
            return arg->evt.s2_event.s2_data.kex_report.security_keys == expected->evt.s2_event.s2_data.kex_report.security_keys && arg->evt.s2_event.s2_data.kex_report.csa == expected->evt.s2_event.s2_data.kex_report.csa;
        case S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT:
            return arg->evt.s2_event.s2_data.challenge_req.granted_keys == expected->evt.s2_event.s2_data.challenge_req.granted_keys && arg->evt.s2_event.s2_data.challenge_req.dsk_length == expected->evt.s2_event.s2_data.challenge_req.dsk_length
                   && arg->evt.s2_event.s2_data.challenge_req.length == expected->evt.s2_event.s2_data.challenge_req.length
                   && memcmp(arg->evt.s2_event.s2_data.challenge_req.public_key, expected->evt.s2_event.s2_data.challenge_req.public_key, expected->evt.s2_event.s2_data.challenge_req.length) == 0;
        case S2_NODE_INCLUSION_COMPLETE_EVENT:
        case S2_NODE_JOINING_COMPLETE_EVENT:
        case S2_JOINING_COMPLETE_NEVER_STARTED_EVENT:
            return arg->evt.s2_event.s2_data.inclusion_complete.exchanged_keys == expected->evt.s2_event.s2_data.inclusion_complete.exchanged_keys;
        case S2_NODE_INCLUSION_FAILED_EVENT:
            return arg->evt.s2_event.s2_data.inclusion_fail.kex_fail_type == expected->evt.s2_event.s2_data.inclusion_fail.kex_fail_type;
        case S2_NODE_INCLUSION_INITIATED_EVENT:
            return true;
        default:
            return false;
    }
}

inline void stub_inclusion_extern()
{
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(::testing::_, ::testing::_)).Times(::testing::AnyNumber()).WillRepeatedly(::testing::Return(0));
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).Times(::testing::AnyNumber());
}

inline void stub_network_key_update()
{
    EXPECT_CALL(*g_libs2_mocks, S2_network_key_update(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(::testing::AnyNumber()).WillRepeatedly(::testing::Return(1));
}

inline void stub_keystore_network()
{
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(::testing::_, ::testing::_)).Times(::testing::AnyNumber()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_write(::testing::_, ::testing::_)).Times(::testing::AnyNumber()).WillRepeatedly(::testing::Return(true));
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_clear(::testing::_)).Times(::testing::AnyNumber()).WillRepeatedly(::testing::Return(true));
}

inline void stub_keystore_all()
{
    stub_keystore_network();
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(::testing::_)).Times(::testing::AnyNumber());
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(::testing::_)).Times(::testing::AnyNumber());
    EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(::testing::_)).Times(::testing::AnyNumber());
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_private_key_read(::testing::_)).Times(::testing::AnyNumber());
}

inline void stub_crypto_scalarmult()
{
    EXPECT_CALL(*g_libs2_mocks, crypto_scalarmult_curve25519(::testing::_, ::testing::_, ::testing::_)).Times(::testing::AnyNumber());
}

inline void stub_tempkey_extract()
{
    EXPECT_CALL(*g_libs2_mocks, tempkey_extract(::testing::_, ::testing::_, ::testing::_)).Times(::testing::AnyNumber());
}
