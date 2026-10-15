#include "libs2_gmock.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

Libs2Mocks *g_libs2_mocks = nullptr;

#include <set>
#include <string>

static std::set<std::string> g_stub_names;

void libs2_clear_stubs()
{
    g_stub_names.clear();
}

void libs2_stub(const char *name)
{
    g_stub_names.insert(name);
    if (strcmp(name, "s2_inclusion_extern_mock.c") == 0) {
        g_stub_names.insert("s2_inclusion_set_timeout");
        g_stub_names.insert("s2_inclusion_stop_timeout");
    } else if (strcmp(name, "s2_event_handler") == 0) {
        g_stub_names.insert("s2_event_handler_mock.c");
    } else if (strcmp(name, "S2_send_frame") == 0) {
        g_stub_names.insert("s2_extern_mock.c");
    } else if (strcmp(name, "s2_keystore_mock.c") == 0) {
        g_stub_names.insert("keystore_network_key_read");
        g_stub_names.insert("keystore_network_key_write");
        g_stub_names.insert("keystore_network_key_clear");
    }
    if (strcmp(name, "s2_inclusion_extern_mock.c") == 0 || strcmp(name, "s2_inclusion_set_timeout") == 0 || strcmp(name, "s2_inclusion_stop_timeout") == 0) {
        stub_inclusion_extern();
    } else if (strcmp(name, "S2_network_key_update") == 0) {
        stub_network_key_update();
    } else if (strcmp(name, "keystore_network_key_read") == 0 || strcmp(name, "keystore_network_key_write") == 0 || strcmp(name, "keystore_network_key_clear") == 0) {
        if (strcmp(name, "keystore_network_key_read") == 0) {
            EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(::testing::_, ::testing::_)).Times(::testing::AnyNumber()).WillRepeatedly(::testing::Return(true));
        } else if (strcmp(name, "keystore_network_key_write") == 0) {
            EXPECT_CALL(*g_libs2_mocks, keystore_network_key_write(::testing::_, ::testing::_)).Times(::testing::AnyNumber()).WillRepeatedly(::testing::Return(true));
        } else {
            EXPECT_CALL(*g_libs2_mocks, keystore_network_key_clear(::testing::_)).Times(::testing::AnyNumber()).WillRepeatedly(::testing::Return(true));
        }
    } else if (strcmp(name, "s2_keystore_mock.c") == 0) {
        stub_keystore_all();
    } else if (strcmp(name, "crypto_scalarmult_curve25519") == 0) {
        stub_crypto_scalarmult();
    } else if (strcmp(name, "tempkey_extract") == 0) {
        stub_tempkey_extract();
    } else if (strcmp(name, "S2_send_frame") == 0) {
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(::testing::AnyNumber()).WillRepeatedly(::testing::Return(1));
    } else if (strcmp(name, "S2_send_data") == 0) {
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(::testing::AnyNumber()).WillRepeatedly(::testing::Return(1));
    } else if (strcmp(name, "s2_event_handler") == 0 || strcmp(name, "s2_event_handler_mock.c") == 0) {
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(::testing::_)).Times(::testing::AnyNumber());
    }
}

bool libs2_is_stubbed(const char *name)
{
    return g_stub_names.count(name) != 0;
}

bool libs2_is_stubbed(const char *name, const char *alt)
{
    return libs2_is_stubbed(name) || libs2_is_stubbed(alt);
}

extern "C" {

uint8_t S2_send_frame(struct S2 *ctxt, const s2_connection_t *conn, uint8_t *buf, uint16_t len)
{
    return g_libs2_mocks->S2_send_frame(ctxt, conn, buf, len);
}

uint8_t S2_send_data(struct S2 *ctx, s2_connection_t *dst, const uint8_t *buf, uint16_t len)
{
    return g_libs2_mocks->S2_send_data(ctx, dst, buf, len);
}

uint8_t S2_network_key_update(struct S2 *ctx, uint32_t key_id, security_class_t class_id, const network_key_t net_key, uint8_t temp_key_expand)
{
    return g_libs2_mocks->S2_network_key_update(ctx, key_id, class_id, net_key, temp_key_expand);
}

void s2_event_handler(zwave_event_t *evt)
{
    g_libs2_mocks->s2_event_handler(evt);
}

uint8_t s2_inclusion_set_timeout(struct S2 *ctxt, uint32_t timeout)
{
    return g_libs2_mocks->s2_inclusion_set_timeout(ctxt, timeout);
}

void s2_inclusion_stop_timeout(void)
{
    g_libs2_mocks->s2_inclusion_stop_timeout();
}

void keystore_public_key_read(uint8_t *buf)
{
    g_libs2_mocks->keystore_public_key_read(buf);
}

void keystore_dynamic_public_key_read(uint8_t *buf)
{
    g_libs2_mocks->keystore_dynamic_public_key_read(buf);
}

void keystore_private_key_read(uint8_t *buf)
{
    g_libs2_mocks->keystore_private_key_read(buf);
}

void keystore_dynamic_private_key_read(uint8_t *buf)
{
    g_libs2_mocks->keystore_dynamic_private_key_read(buf);
}

bool keystore_network_key_read(uint8_t keyclass, uint8_t *buf)
{
    return g_libs2_mocks->keystore_network_key_read(keyclass, buf);
}

bool keystore_network_key_write(uint8_t keyclass, const uint8_t *buf)
{
    return g_libs2_mocks->keystore_network_key_write(keyclass, buf);
}

bool keystore_network_key_clear(uint8_t keyclass)
{
    return g_libs2_mocks->keystore_network_key_clear(keyclass);
}

void crypto_scalarmult_curve25519(uint8_t *r, const uint8_t *s, const uint8_t *p)
{
    g_libs2_mocks->crypto_scalarmult_curve25519(r, s, p);
}

void tempkey_extract(const uint8_t *const ecdh_share_secret, const uint8_t *const auth_tag, uint8_t *pseudo_random_keymat_output)
{
    g_libs2_mocks->tempkey_extract(ecdh_share_secret, auth_tag, pseudo_random_keymat_output);
}

void S2_dbg_printf(const char *format, ...)
{
    (void)format;
}

void S2_send_done_event(struct S2 *ctxt, s2_tx_status_t status)
{
    (void)ctxt;
    (void)status;
}

void S2_msg_received_event(struct S2 *ctxt, s2_connection_t *src, uint8_t *buf, uint16_t len)
{
    (void)ctxt;
    (void)src;
    (void)buf;
    (void)len;
}

uint8_t S2_send_frame_no_cb(struct S2 *ctxt, const s2_connection_t *conn, uint8_t *buf, uint16_t len)
{
    (void)ctxt;
    (void)conn;
    (void)buf;
    (void)len;
    return 1;
}

uint8_t S2_send_frame_multi(struct S2 *ctxt, s2_connection_t *conn, uint8_t *buf, uint16_t len)
{
    (void)ctxt;
    (void)conn;
    (void)buf;
    (void)len;
    return 1;
}

void S2_set_timeout(struct S2 *ctxt, uint32_t interval)
{
    (void)ctxt;
    (void)interval;
}

void S2_stop_timeout(struct S2 *ctxt)
{
    (void)ctxt;
}

void S2_get_hw_random(uint8_t *buf, uint8_t len)
{
    memset(buf, 0, len);
}

void S2_get_commands_supported(node_t lnode, uint8_t class_id, const uint8_t **cmdClasses, uint8_t *length)
{
    (void)lnode;
    (void)class_id;
    static const uint8_t empty[1] = {0};
    *cmdClasses                   = empty;
    *length                       = 0;
}

void S2_notify_nls_state_report(node_t srcNode, uint8_t class_id, bool nls_capability, bool nls_state)
{
    (void)srcNode;
    (void)class_id;
    (void)nls_capability;
    (void)nls_state;
}

int8_t S2_get_nls_node_list(node_t srcNode, bool request, bool *is_last_node, uint16_t *node_id, uint8_t *granted_keys, bool *nls_state)
{
    (void)srcNode;
    (void)request;
    (void)is_last_node;
    (void)node_id;
    (void)granted_keys;
    (void)nls_state;
    return -1;
}

int8_t S2_notify_nls_node_list_report(node_t srcNode, uint16_t id_of_node, uint8_t keys_node_bitmask, bool nls_state)
{
    (void)srcNode;
    (void)id_of_node;
    (void)keys_node_bitmask;
    (void)nls_state;
    return 0;
}

void S2_resynchronization_event(node_t remote_node, sos_event_reason_t reason, uint8_t seqno, node_t local_node)
{
    (void)remote_node;
    (void)reason;
    (void)seqno;
    (void)local_node;
}

void S2_save_nls_state(void) {}

unsigned long clock_time(void)
{
    return 0;
}
}
