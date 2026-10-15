/* © 2014 Silicon Laboratories Inc.
 */

/*
 * test_inclusion.c
 *
 *  Created on: Aug 25, 2015
 *      Author: trasmussen
 */

#include <gtest/gtest.h>
#include "libs2_gmock.hpp"
using ::testing::_;
using ::testing::DoAll;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArrayArgument;

extern "C" {
#include "s2_inclusion.h"
#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "s2_protocol.h"

#include "ZW_classcmd.h"
#include "s2_classcmd.h"
#include "s2_keystore.h"
}

class InclusionTest : public ::testing::Test
{
    protected:
        ::testing::StrictMock<Libs2Mocks> mocks;
        void SetUp() override
        {
            g_libs2_mocks = &mocks;
            libs2_clear_stubs();
        }
        void TearDown() override
        {
            g_libs2_mocks = nullptr;
        }
};

#define ELEM_COUNT(ARRAY) (sizeof(ARRAY) / (sizeof(ARRAY[0])))
#ifdef ZW_CONTROLLER
#define UNIT_TEST_TEMP_KEY_SECURE 5  //< Value identifying index for the temporary key in S2 network key when transmitting secure frames.
#define UNIT_TEST_NETWORK_KEY     6  //< Value identifying index for the temporary key in S2 network key when transmitting secure frames.
#else
#define UNIT_TEST_TEMP_KEY_SECURE 0  //< Value identifying index for the temporary key in S2 network key when transmitting secure frames.
#define UNIT_TEST_NETWORK_KEY     1  //< Value identifying index for the temporary key in S2 network key when transmitting secure frames.
#endif

/** Overview of behavior which should be implemented:
 *  General inclusion:
 *  - Implement SM according to designed flow.
 *  - Identify all events and actions which must be handled and executed during node inclusion.
 *  - It is expected that a common S2 layer resides above the inclusion module (and other S2
 *    related modules) which is responsible for dispatching packet/events based on command.
 *    In the tests it is expected that a learn request will initiate the inclusion flow.
 *  - Early discussions on future direction has been initiated between Torsten, Jakob, and Anders on event generation
 *    Early ideas is to have an event notification system which could be simplified version of SKB in Linux.
 *    This work must progress but in S2_inclusion.c a simple notification system will be taken into use with refactoring possibilities.
 *
 *
 *  Error handling:
 *  - Timeout: it must be ensured that a proper timer API is available for the inclusion state machine is available.
 *             In case a timeout occurs, it must be ensured that the inclusion state machine returns to idle and
 *             appropriate error is notified to callback provided by protocol layer.
 *             Error handler provided by protocol layer is expected to be implemented by application..
 *             The SM will not be responsible for further actions due to timeouts.
 *
 */

/** Current working assumptions:
 *  Lib S2 - message encapsulation/inclusion mode and context handling
 *  -   Currently context is updated with peer, buffer, and length in libs2 protocol (message
 *     encapsulation handling), this should be moved to common place. s2 dispatch layer ?
 *  -   Libs2 S2.c contains the entry point for frames receined, S2_application_command_handler(.),
 *     this should ideally be refactored to upper dispatcher layer which should be common for encapsulation handling and inclusion.
 */

/** Currently identyfied test cases not yet implemented.
 *
 *  - Key Verify failure.
 *
 */
#define UNIT_TEST_BUF_SIZE 256

static uint8_t m_test_mem_pool[4][256];  // Statically allocated chunks of memory that can be freely used during the test.

static uint8_t m_test_public_key_a[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};

static uint8_t m_test_obfuscated_public_key_a[] = {0x00, 0x00, 0x00, 0x00, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};

static uint8_t m_temp_key[] = {0x7E, 0xFE, 0x12, 0x32, 0x45, 0x65, 0x78, 0x98, 0x7E, 0xFE, 0x12, 0x32, 0x45, 0x65, 0x78, 0x98, 0x7E, 0xFE, 0x12, 0x32, 0x45, 0x65, 0x78, 0x98, 0x7E, 0xFE, 0x12, 0x32, 0x45, 0x65, 0x78, 0x98};

static uint8_t m_test_network_key_s2_class_0[] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99};

static uint8_t m_test_network_key_s2_class_1[] = {0xAA, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99};

static uint8_t m_test_network_key_s2_class_2[] = {0xBB, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99};

static uint8_t m_test_network_key_s0[] = {0xCA, 0xFE, 0xBA, 0xBE, 0x44, 0x33, 0x22, 0x11, 0xCA, 0xFE, 0xBA, 0xBE, 0xCC, 0xBB, 0xAA, 0x99};

static s2_connection_t s2_con;
/** Event listener for S2 inclusion.
 *
 *  S2 will use an event listening scheme for posting notifications upwards.
 *  How the notification is stored/handled is outside scope of the state machine, but it is
 *  imagined that such system would implement a queing system to allow inclusion operations
 *  to progress as fast as possible.
 *
 *  This forward declares the mock funtion used, because the callback is a function pointer with no named declaration.
 */

/** Forward declaration of helper classes.
 *  All helper classes are implemented in buttom of file to easily keep them seperated from the test cases.
 */
void helper_func_kex_report_frame_expect(uint8_t scheme, uint8_t curve, uint8_t keys);
void helper_func_kex_get_frame(struct S2 *p_context);
void helper_func_kex_set_frame(struct S2 *p_context, uint8_t scheme, uint8_t keys);
void helper_func_pub_key_frame(struct S2 *p_context);
void helper_func_echo_kex_report_frame(struct S2 *p_context, uint8_t scheme, uint8_t curve, uint8_t keys);
void helper_func_net_key_report_frame(struct S2 *p_context, uint8_t key);
void helper_func_transfer_end_frame(struct S2 *p_context);
void helper_func_verify_idle_state(struct S2 *p_context, uint8_t scheme, uint8_t keys);
void helper_func_idle_state(struct S2 *p_context);
void helper_func_inclusion_initiated_event(void);
static void helper_func_init_s2_conn(void);
static void helper_func_init_s2_conn_lr(void);

/** Verification that the normal flow succeeds in inclusion of a new node when using CSA.
 *
 * It verifies the behavior as seen from an including node (Controller node) as described in SDS11274.
 *
 * When a node is to be included securely it is expected that a ZW_SendData is send.
 * For this the common S2_send_frame(...) defined in s2.h is used, which will be implemented elsewhere.
 */
TEST_F(InclusionTest, test_kex_joining_node_state_machine_csa)
{
    int test_flavour;
    for (test_flavour = 0; test_flavour < 2; test_flavour++) {
        uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key.
                                  0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

        uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                                   0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

        libs2_clear_stubs();
        libs2_stub("s2_inclusion_extern_mock.c");
        libs2_stub("S2_network_key_update");
        libs2_stub("keystore_network_key_read");
        libs2_stub("keystore_network_key_clear");
        libs2_stub("keystore_network_key_write");
        libs2_stub("crypto_scalarmult_curve25519");
        libs2_stub("tempkey_extract");

        /**
         *  Test expectation setup.
         * This section set up the expectations in the system when the inclusion state machine progresses:
         * 1) Starting in idle it is expected that an S2 frame KEX1 will be transmitted based on external event when a node id has been assigned.
         * 2) After S2 KEX Get has been transmitted, the FSM awaits a S2 KEX Report as response.
         * 3) After S2 KEX Report is received then S2 KEX Set shall be sent.
         * 4) After S2 KEX Set  has been transmitted, the FSM awaits a Public KeyB from joining node.
         * 5) Exchange of public keys.
         *    a) Public KeyB is received from joining node A.
         *       - Public KeyB must be pushed upwards as event to be presented to user for further verification.
         *         If node is rejected, then node is part of network insecurely. Should this be changed ? Maybe force a node exclude.
         *         Currently that use case is outside the scope of libs2.
         *       - When Public KeyB is received, the controller must send its public KeyA.
         *         Public KeyB shall be provided by the application.
         *       - Using Pub KeyB and Private KeyA then a temporary key, KeyT is derived from the x coordinate of the shared secret.
         *         This is done in security sub module (currently under development)
         *       - The security context must be re-initialized with temporary derived key.
         *    b) Public KeyA is transmitted from including node A, the state machine will await 'Echo (KEX Set)'.
         * 6) Echoing of KEX frames.
         *    a) A 'Echo(KEX Set)' is expected to be received from the joining node, and it must be
         *       verified that 'Echo(KEX Set)' is identical to the KEX Set sent at step 3).
         *    b) 'Echo(KEX Report)' with content identical to packet received in 3) shall be replied to the joining node.
         * x) Nonce exchange: When joining node wants to send 'Echo(KEX Set)' the S2 protocol layer will exchange a nonce prior to communication.
         * 7) Network key exchange:
         *    a) Joining node will request the network key, by sending a 'Network Key Get'.
         *    b) Including node shall reply with a 'Network Key Set'.
         *    c) Joining node shall confirm with a 'Network Key Verify' which is encrypted using the the key obtained by 'NetworkKey-expand'
         *       - Including node must verify that it can succesfully decrypt the message using the key.
         *    d) Including node shall reply with a 'Transfer End'.
         *    e) Joining node shall reply with a 'Transfer End' if no more keys shall be exchanged.
         *
         */

        // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
        zwave_event_t *p_expected_inc_init_event = (zwave_event_t *)m_test_mem_pool[2];
        p_expected_inc_init_event->event_type    = S2_NODE_INCLUSION_INITIATED_EVENT;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_init_event))).RetiresOnSaturation();

        // Expect a S2 KEX Get to be sent.
        uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2,
                                         KEX_REPORT,
                                         0x06,   // reserved | NLS support | Request CSA | Echo
                                         0x02,   // Supported schemes. Scheme 0 and 2.
                                         0x01,   // Supported ECDH Profiles, bit0=1 is curve 25519 value.
                                         0x82};  // Requested keys bits. Security 2 class 1, Security 0 network key.

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // Before exchange of public keys, then we expect that our public key is requested from the keystore.
        EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        uint8_t s2_public_key_frame[3 + sizeof(public_key_b)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x00, 0x00, 0x00};  // Key exchange received from slave - public key for secure exchange of LTK. Note the first two bytes should not be transmitted on authenticated/access keys.
        memcpy(&s2_public_key_frame[3], &public_key_b[0], sizeof(public_key_b));

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_public_key_frame, sizeof(s2_public_key_frame)), sizeof(s2_public_key_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // When receiving public key A, then we expect an event to be pushed up and both our public and private keys are requested from the keystore.
        zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
        p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_obfuscated_public_key_a);
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0x04;
        memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_obfuscated_public_key_a, sizeof(m_test_obfuscated_public_key_a));
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

        EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
        EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        // Expect Echo(KEX Report) to be sent.
        uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x03, 0x02, 0x01, 0x82};  // Note: Echo flag set.

        if (test_flavour == 1) {
            /* Test the special case where a reserved bit is set in the KEX_SET */

            /* Expect the reserved bit 0x10 to be set in the KEX_SET_ECHO */
            S2_echo_kex_set_frame[2] |= 0x10;
        }

        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        uint8_t s2_net_key_get_2_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};  // Keys requested, Security2, class 2 - Security0, network key.
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_2_frame, sizeof(s2_net_key_get_2_frame)), sizeof(s2_net_key_get_2_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        //  // After receiving the network key, we expect to get a call to update the context to use the new key when transmitting Net key verify.
        //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
        //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
        //  p_mock->expect_arg[1].pointer = m_test_network_key_s2_class_1;

        uint8_t s2_net_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_frame, sizeof(s2_net_key_verify_frame)), sizeof(s2_net_key_verify_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        //  // After transmitting Net key verify, we switch back to the tempporary key.
        //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
        //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
        //  p_mock->expect_arg[1].pointer = m_temp_key;

        uint8_t s2_net_key_get_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x80};  // Keys requested, Security2, class 2 - Security0, network key.
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_0_frame, sizeof(s2_net_key_get_0_frame)), sizeof(s2_net_key_get_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // After receiving the network key, we expect to get a call to update the context to use the new key when transmitting Net key verify.
        //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
        //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
        //  p_mock->expect_arg[1].pointer = m_test_network_key_s0;

        uint8_t s2_net_key_verify_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_0_frame, sizeof(s2_net_key_verify_0_frame)), sizeof(s2_net_key_verify_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // After transmitting Net key verify, we switch back to the tempporary key.
        //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
        //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
        //  p_mock->expect_arg[1].pointer = m_temp_key;

        // Expect S2 Transfer End to be sent.
        uint8_t S2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x01};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_transfer_end_frame, sizeof(S2_transfer_end_frame)), sizeof(S2_transfer_end_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
        zwave_event_t *p_expected_complete_event                                          = (zwave_event_t *)m_test_mem_pool[1];
        p_expected_complete_event->event_type                                             = S2_NODE_JOINING_COMPLETE_EVENT;
        p_expected_complete_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0x82;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_complete_event))).RetiresOnSaturation();

        /**
         *  Test execution.
         */
        struct S2 s2_context;

        s2_context.inclusion_state = S2_INC_IDLE;
        // memcpy(s2_context.temp_network_key, m_temp_key, sizeof(s2_context.temp_network_key));

        s2_inclusion_init(0x02, 0x01, 0x82);
        s2_inclusion_set_event_handler(s2_event_handler);
        helper_func_init_s2_conn();

        s2_inclusion_joining_start(&s2_context, &s2_con, 0x01);

        // KEX Get frame received.
        uint8_t S2_kex_get_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
        s2_context.buf             = S2_kex_get_frame;
        s2_context.length          = sizeof(S2_kex_get_frame);
        s2_con.class_id            = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // KEX report is expeted to be transmitted. Let's make the s2_send_frame a success.
        s2_inclusion_send_done(&s2_context, 1);

        // KEX Set frame received.
        // bit0: echo field not set, bit1-7: Reserved.
        // Selected schemes: scheme 0 and scheme 2.
        // Selected curve25519
        // Keys to exchange, Security2, class 2 - Security0, network key.
        uint8_t s2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x02, 0x02, 0x01, 0x82};

        if (test_flavour == 1) {
            /* Test the special case where a reserved bit is set in the KEX_SET */
            s2_kex_set_frame[2] |= 0x10;
        }

        s2_context.buf    = s2_kex_set_frame;
        s2_context.length = sizeof(s2_kex_set_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        uint8_t public_key_frame[3 + sizeof(m_test_public_key_a)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x01};  // Key exchange received from slave - public key for secure exchange of LTK.
        memcpy(&public_key_frame[3], m_test_obfuscated_public_key_a, sizeof(m_test_obfuscated_public_key_a));
        s2_context.buf    = public_key_frame;
        s2_context.length = sizeof(public_key_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // After the received public key is pushed upwards in the system, then it is expected to receive a challenge response from the upper level.
        // The challenge response should result in correct pub key being set on the context.
        s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, DSK_CSA_CHALLENGE_LENGTH);
        EXPECT_EQ(0, memcmp(s2_context.public_key, m_test_public_key_a, sizeof(m_test_public_key_a)));

        // After receiving public key from joining node, the upper layer must specify our public key to be send. (This can also be done on initialization, but must happen no later than the received event).

        // Echo(KEX Report) frame received.
        // bit0: echo field set, bit1-7: Reserved.
        // Selected schemes: scheme 0 and scheme 2.
        // Selected curve25519
        // Keys to exchange, Security2, class 2 - Security0, network key.
        uint8_t s2_echo_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x07, 0x02, 0x01, 0x82};

        s2_context.buf    = s2_echo_kex_report_frame;
        s2_context.length = sizeof(s2_echo_kex_report_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // Network Key Report frame received.
        uint8_t s2_net_key_report_frame[3 + sizeof(m_test_network_key_s2_class_1)] = {
          COMMAND_CLASS_SECURITY_2,
          SECURITY_2_NETWORK_KEY_REPORT,
          0x02,
        };  // Keys requested, Security2, class 2 - Security0, network key.
        memcpy(&s2_net_key_report_frame[3], m_test_network_key_s2_class_1, sizeof(m_test_network_key_s2_class_1));
        s2_context.buf    = s2_net_key_report_frame;
        s2_context.length = sizeof(s2_net_key_report_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // S2 Transfer end frame received.
        // bit0: Key request complete not set,
        // bit1: Key verified set,
        // bit2-7: Reserved.
        uint8_t s2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x02};
        s2_context.buf                  = s2_transfer_end_frame;
        s2_context.length               = sizeof(s2_transfer_end_frame);
        s2_con.class_id                 = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // Network Key Report frame received.
        uint8_t s2_net_key_report_0_frame[3 + sizeof(m_test_network_key_s0)] = {
          COMMAND_CLASS_SECURITY_2,
          SECURITY_2_NETWORK_KEY_REPORT,
          0x80,
        };  // Keys requested, Security2, class 2 - Security0, network key.
        memcpy(&s2_net_key_report_0_frame[3], m_test_network_key_s0, sizeof(m_test_network_key_s0));
        s2_context.buf    = s2_net_key_report_0_frame;
        s2_context.length = sizeof(s2_net_key_report_0_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // S2 Transfer end frame received.
        // bit0: Key request complete not set,
        // bit1: Key verified set,
        // bit2-7: Reserved.
        s2_context.buf    = s2_transfer_end_frame;
        s2_context.length = sizeof(s2_transfer_end_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_send_done(&s2_context, true);

    } /* for(test_flavour) */
}

TEST_F(InclusionTest, test_kex_joining_node_state_machine_authenticated_lr)
{
    int test_flavour;
    for (test_flavour = 0; test_flavour < 2; test_flavour++) {
        uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key.
                                  0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

        uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                                   0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

        libs2_clear_stubs();
        libs2_stub("s2_inclusion_extern_mock.c");
        libs2_stub("S2_network_key_update");
        libs2_stub("keystore_network_key_read");
        libs2_stub("keystore_network_key_clear");
        libs2_stub("keystore_network_key_write");
        libs2_stub("crypto_scalarmult_curve25519");
        libs2_stub("tempkey_extract");

        /**
         *  Test expectation setup.
         * This section set up the expectations in the system when the inclusion state machine progresses:
         * 1) Starting in idle it is expected that an S2 frame KEX1 will be transmitted based on external event when a node id has been assigned.
         * 2) After S2 KEX Get has been transmitted, the FSM awaits a S2 KEX Report as response.
         * 3) After S2 KEX Report is received then S2 KEX Set shall be sent.
         * 4) After S2 KEX Set  has been transmitted, the FSM awaits a Public KeyB from joining node.
         * 5) Exchange of public keys.
         *    a) Public KeyB is received from joining node A.
         *       - Public KeyB must be pushed upwards as event to be presented to user for further verification.
         *         If node is rejected, then node is part of network insecurely. Should this be changed ? Maybe force a node exclude.
         *         Currently that use case is outside the scope of libs2.
         *       - When Public KeyB is received, the controller must send its public KeyA.
         *         Public KeyB shall be provided by the application.
         *       - Using Pub KeyB and Private KeyA then a temporary key, KeyT is derived from the x coordinate of the shared secret.
         *         This is done in security sub module (currently under development)
         *       - The security context must be re-initialized with temporary derived key.
         *    b) Public KeyA is transmitted from including node A, the state machine will await 'Echo (KEX Set)'.
         * 6) Echoing of KEX frames.
         *    a) A 'Echo(KEX Set)' is expected to be received from the joining node, and it must be
         *       verified that 'Echo(KEX Set)' is identical to the KEX Set sent at step 3).
         *    b) 'Echo(KEX Report)' with content identical to packet received in 3) shall be replied to the joining node.
         * x) Nonce exchange: When joining node wants to send 'Echo(KEX Set)' the S2 protocol layer will exchange a nonce prior to communication.
         * 7) Network key exchange:
         *    a) Joining node will request the network key, by sending a 'Network Key Get'.
         *    b) Including node shall reply with a 'Network Key Set'.
         *    c) Joining node shall confirm with a 'Network Key Verify' which is encrypted using the the key obtained by 'NetworkKey-expand'
         *       - Including node must verify that it can succesfully decrypt the message using the key.
         *    d) Including node shall reply with a 'Transfer End'.
         *    e) Joining node shall reply with a 'Transfer End' if no more keys shall be exchanged.
         *
         */

        // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
        zwave_event_t *p_expected_inc_init_event = (zwave_event_t *)m_test_mem_pool[2];
        p_expected_inc_init_event->event_type    = S2_NODE_INCLUSION_INITIATED_EVENT;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_init_event))).RetiresOnSaturation();

        // Expect a S2 KEX Get to be sent.
        uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2,
                                         KEX_REPORT,
                                         0x04,   // reserved | NLS support | Request CSA | Echo
                                         0x02,   // Supported schemes. Scheme 0 and 2.
                                         0x01,   // Supported ECDH Profiles, bit0=1 is curve 25519 value.
                                         0x02};  // Requested keys bits. Security 2 class 3 (Long Range Authenticated uses normal Authenticated bit on the radio)

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // Before exchange of public keys, then we expect that our public key is requested from the keystore.
        EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        uint8_t s2_public_key_frame[3 + sizeof(public_key_b)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x00, 0x00, 0x00};  // Key exchange received from slave - public key for secure exchange of LTK. Note the first two bytes should not be transmitted on authenticated/access keys.
        memcpy(&s2_public_key_frame[5], &public_key_b[2], sizeof(public_key_b) - 2);

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_public_key_frame, sizeof(s2_public_key_frame)), sizeof(s2_public_key_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // When receiving public key A, then we expect an event to be pushed up and both our public and private keys are requested from the keystore.
        zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
        p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x02;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
        memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

        EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
        EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        // Expect Echo(KEX Report) to be sent.
        uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x02};  // Note: Echo flag set.

        if (test_flavour == 1) {
            /* Test the special case where a reserved bit is set in the KEX_SET */

            /* Expect the reserved bit 0x10 to be set in the KEX_SET_ECHO */
            S2_echo_kex_set_frame[2] |= 0x10;
        }

        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        uint8_t s2_net_key_get_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};  // Keys requested, Security2, class 3
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_0_frame, sizeof(s2_net_key_get_0_frame)), sizeof(s2_net_key_get_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // After receiving the network key, we expect to get a call to update the context to use the new key when transmitting Net key verify.
        //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
        //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
        //  p_mock->expect_arg[1].pointer = m_test_network_key_s0;

        uint8_t s2_net_key_verify_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_0_frame, sizeof(s2_net_key_verify_0_frame)), sizeof(s2_net_key_verify_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // After transmitting Net key verify, we switch back to the tempporary key.
        //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
        //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
        //  p_mock->expect_arg[1].pointer = m_temp_key;

        // Expect S2 Transfer End to be sent.
        uint8_t S2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x01};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_transfer_end_frame, sizeof(S2_transfer_end_frame)), sizeof(S2_transfer_end_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
        zwave_event_t *p_expected_complete_event                                          = (zwave_event_t *)m_test_mem_pool[1];
        p_expected_complete_event->event_type                                             = S2_NODE_JOINING_COMPLETE_EVENT;
        p_expected_complete_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0x02;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_complete_event))).RetiresOnSaturation();

        /**
         *  Test execution.
         */
        struct S2 s2_context;

        s2_context.inclusion_state = S2_INC_IDLE;
        // memcpy(s2_context.temp_network_key, m_temp_key, sizeof(s2_context.temp_network_key));

        s2_inclusion_init(0x02, 0x01, 0x02);
        s2_inclusion_set_event_handler(s2_event_handler);
        helper_func_init_s2_conn_lr();

        s2_inclusion_joining_start(&s2_context, &s2_con, 0x00);

        // KEX Get frame received.
        uint8_t S2_kex_get_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
        s2_context.buf             = S2_kex_get_frame;
        s2_context.length          = sizeof(S2_kex_get_frame);
        s2_con.class_id            = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // KEX report is expeted to be transmitted. Let's make the s2_send_frame a success.
        s2_inclusion_send_done(&s2_context, 1);

        // KEX Set frame received.
        // bit0: echo field not set, bit1-7: Reserved.
        // Selected schemes: scheme 0 and scheme 2.
        // Selected curve25519
        // Keys to exchange, Security2, class 3 (Long Range Authenticated)
        uint8_t s2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x00, 0x02, 0x01, 0x02};

        if (test_flavour == 1) {
            /* Test the special case where a reserved bit is set in the KEX_SET */
            s2_kex_set_frame[2] |= 0x10;
        }

        s2_context.buf    = s2_kex_set_frame;
        s2_context.length = sizeof(s2_kex_set_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        uint8_t public_key_frame[3 + sizeof(m_test_public_key_a)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x01};  // Key exchange received from slave - public key for secure exchange of LTK.
        memcpy(&public_key_frame[3], m_test_public_key_a, sizeof(m_test_public_key_a));
        s2_context.buf    = public_key_frame;
        s2_context.length = sizeof(public_key_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // After the received public key is pushed upwards in the system, then it is expected to receive a challenge response from the upper level.
        // The challenge response should result in correct pub key being set on the context.
        s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);
        EXPECT_EQ(0, memcmp(s2_context.public_key, m_test_public_key_a, sizeof(m_test_public_key_a)));

        // After receiving public key from joining node, the upper layer must specify our public key to be send. (This can also be done on initialization, but must happen no later than the received event).

        // Echo(KEX Report) frame received.
        // bit0: echo field set, bit1-7: Reserved.
        // Selected schemes: scheme 0 and scheme 2.
        // Selected curve25519
        // Keys to exchange, Security2, class 3 (Long Range Authenticated)
        uint8_t s2_echo_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x05, 0x02, 0x01, 0x02};

        s2_context.buf    = s2_echo_kex_report_frame;
        s2_context.length = sizeof(s2_echo_kex_report_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // Network Key Report frame received.
        uint8_t s2_net_key_report_frame[3 + sizeof(m_test_network_key_s2_class_1)] = {
          COMMAND_CLASS_SECURITY_2,
          SECURITY_2_NETWORK_KEY_REPORT,
          0x02,
        };  // Keys requested, Security2, class 3 using non-LR bits even for LR
        memcpy(&s2_net_key_report_frame[3], m_test_network_key_s2_class_1, sizeof(m_test_network_key_s2_class_1));
        s2_context.buf    = s2_net_key_report_frame;
        s2_context.length = sizeof(s2_net_key_report_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // S2 Transfer end frame received.
        // bit0: Key request complete not set,
        // bit1: Key verified set,
        // bit2-7: Reserved.
        uint8_t s2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x02};
        s2_context.buf                  = s2_transfer_end_frame;
        s2_context.length               = sizeof(s2_transfer_end_frame);
        s2_con.class_id                 = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_send_done(&s2_context, true);

    } /* for(test_flavour) */
}
TEST_F(InclusionTest, test_kex_joining_node_state_machine_ssa)
{
    int test_flavour;
    for (test_flavour = 0; test_flavour < 2; test_flavour++) {
        uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key.
                                  0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

        uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                                   0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

        libs2_clear_stubs();
        libs2_stub("s2_inclusion_extern_mock.c");
        libs2_stub("S2_network_key_update");
        libs2_stub("keystore_network_key_read");
        libs2_stub("keystore_network_key_clear");
        libs2_stub("keystore_network_key_write");
        libs2_stub("crypto_scalarmult_curve25519");
        libs2_stub("tempkey_extract");

        /**
         *  Test expectation setup.
         * This section set up the expectations in the system when the inclusion state machine progresses:
         * 1) Starting in idle it is expected that an S2 frame KEX1 will be transmitted based on external event when a node id has been assigned.
         * 2) After S2 KEX Get has been transmitted, the FSM awaits a S2 KEX Report as response.
         * 3) After S2 KEX Report is received then S2 KEX Set shall be sent.
         * 4) After S2 KEX Set  has been transmitted, the FSM awaits a Public KeyB from joining node.
         * 5) Exchange of public keys.
         *    a) Public KeyB is received from joining node A.
         *       - Public KeyB must be pushed upwards as event to be presented to user for further verification.
         *         If node is rejected, then node is part of network insecurely. Should this be changed ? Maybe force a node exclude.
         *         Currently that use case is outside the scope of libs2.
         *       - When Public KeyB is received, the controller must send its public KeyA.
         *         Public KeyB shall be provided by the application.
         *       - Using Pub KeyB and Private KeyA then a temporary key, KeyT is derived from the x coordinate of the shared secret.
         *         This is done in security sub module (currently under development)
         *       - The security context must be re-initialized with temporary derived key.
         *    b) Public KeyA is transmitted from including node A, the state machine will await 'Echo (KEX Set)'.
         * 6) Echoing of KEX frames.
         *    a) A 'Echo(KEX Set)' is expected to be received from the joining node, and it must be
         *       verified that 'Echo(KEX Set)' is identical to the KEX Set sent at step 3).
         *    b) 'Echo(KEX Report)' with content identical to packet received in 3) shall be replied to the joining node.
         * x) Nonce exchange: When joining node wants to send 'Echo(KEX Set)' the S2 protocol layer will exchange a nonce prior to communication.
         * 7) Network key exchange:
         *    a) Joining node will request the network key, by sending a 'Network Key Get'.
         *    b) Including node shall reply with a 'Network Key Set'.
         *    c) Joining node shall confirm with a 'Network Key Verify' which is encrypted using the the key obtained by 'NetworkKey-expand'
         *       - Including node must verify that it can succesfully decrypt the message using the key.
         *    d) Including node shall reply with a 'Transfer End'.
         *    e) Joining node shall reply with a 'Transfer End' if no more keys shall be exchanged.
         *
         */

        // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
        zwave_event_t *p_expected_inc_init_event = (zwave_event_t *)m_test_mem_pool[2];
        p_expected_inc_init_event->event_type    = S2_NODE_INCLUSION_INITIATED_EVENT;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_init_event))).RetiresOnSaturation();

        // Expect a S2 KEX Get to be sent.
        uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2,
                                         KEX_REPORT,
                                         0x04,   // reserved | NLS support | Request CSA | Echo
                                         0x02,   // Supported schemes. Scheme 0 and 2.
                                         0x01,   // Supported ECDH Profiles, bit0=1 is curve 25519 value.
                                         0x82};  // Requested keys bits. Security 2 class 1, Security 0 network key.

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // Before exchange of public keys, then we expect that our public key is requested from the keystore.
        EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        uint8_t s2_public_key_frame[3 + sizeof(public_key_b)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x00, 0x00, 0x00};  // Key exchange received from slave - public key for secure exchange of LTK. Note the first two bytes should not be transmitted on authenticated/access keys.
        memcpy(&s2_public_key_frame[5], &public_key_b[2], sizeof(public_key_b) - 2);

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_public_key_frame, sizeof(s2_public_key_frame)), sizeof(s2_public_key_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // When receiving public key A, then we expect an event to be pushed up and both our public and private keys are requested from the keystore.
        zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
        p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
        memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

        EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
        EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        // Expect Echo(KEX Report) to be sent.
        uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};  // Note: Echo flag set.

        if (test_flavour == 1) {
            /* Test the special case where a reserved bit is set in the KEX_SET */

            /* Expect the reserved bit 0x10 to be set in the KEX_SET_ECHO */
            S2_echo_kex_set_frame[2] |= 0x10;
        }

        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        uint8_t s2_net_key_get_2_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};  // Keys requested, Security2, class 2 - Security0, network key.
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_2_frame, sizeof(s2_net_key_get_2_frame)), sizeof(s2_net_key_get_2_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        //  // After receiving the network key, we expect to get a call to update the context to use the new key when transmitting Net key verify.
        //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
        //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
        //  p_mock->expect_arg[1].pointer = m_test_network_key_s2_class_1;

        uint8_t s2_net_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_frame, sizeof(s2_net_key_verify_frame)), sizeof(s2_net_key_verify_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        //  // After transmitting Net key verify, we switch back to the tempporary key.
        //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
        //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
        //  p_mock->expect_arg[1].pointer = m_temp_key;

        uint8_t s2_net_key_get_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x80};  // Keys requested, Security2, class 2 - Security0, network key.
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_0_frame, sizeof(s2_net_key_get_0_frame)), sizeof(s2_net_key_get_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // After receiving the network key, we expect to get a call to update the context to use the new key when transmitting Net key verify.
        //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
        //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
        //  p_mock->expect_arg[1].pointer = m_test_network_key_s0;

        uint8_t s2_net_key_verify_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_0_frame, sizeof(s2_net_key_verify_0_frame)), sizeof(s2_net_key_verify_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // After transmitting Net key verify, we switch back to the tempporary key.
        //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
        //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
        //  p_mock->expect_arg[1].pointer = m_temp_key;

        // Expect S2 Transfer End to be sent.
        uint8_t S2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x01};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_transfer_end_frame, sizeof(S2_transfer_end_frame)), sizeof(S2_transfer_end_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
        zwave_event_t *p_expected_complete_event                                          = (zwave_event_t *)m_test_mem_pool[1];
        p_expected_complete_event->event_type                                             = S2_NODE_JOINING_COMPLETE_EVENT;
        p_expected_complete_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0x82;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_complete_event))).RetiresOnSaturation();

        /**
         *  Test execution.
         */
        struct S2 s2_context;

        s2_context.inclusion_state = S2_INC_IDLE;
        // memcpy(s2_context.temp_network_key, m_temp_key, sizeof(s2_context.temp_network_key));

        s2_inclusion_init(0x02, 0x01, 0x82);
        s2_inclusion_set_event_handler(s2_event_handler);
        helper_func_init_s2_conn();

        s2_inclusion_joining_start(&s2_context, &s2_con, 0x00);

        // KEX Get frame received.
        uint8_t S2_kex_get_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
        s2_context.buf             = S2_kex_get_frame;
        s2_context.length          = sizeof(S2_kex_get_frame);
        s2_con.class_id            = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // KEX report is expeted to be transmitted. Let's make the s2_send_frame a success.
        s2_inclusion_send_done(&s2_context, 1);

        // KEX Set frame received.
        // bit0: echo field not set, bit1-7: Reserved.
        // Selected schemes: scheme 0 and scheme 2.
        // Selected curve25519
        // Keys to exchange, Security2, class 2 - Security0, network key.
        uint8_t s2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x00, 0x02, 0x01, 0x82};

        if (test_flavour == 1) {
            /* Test the special case where a reserved bit is set in the KEX_SET */
            s2_kex_set_frame[2] |= 0x10;
        }

        s2_context.buf    = s2_kex_set_frame;
        s2_context.length = sizeof(s2_kex_set_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        uint8_t public_key_frame[3 + sizeof(m_test_public_key_a)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x01};  // Key exchange received from slave - public key for secure exchange of LTK.
        memcpy(&public_key_frame[3], m_test_public_key_a, sizeof(m_test_public_key_a));
        s2_context.buf    = public_key_frame;
        s2_context.length = sizeof(public_key_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // After the received public key is pushed upwards in the system, then it is expected to receive a challenge response from the upper level.
        // The challenge response should result in correct pub key being set on the context.
        s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);
        EXPECT_EQ(0, memcmp(s2_context.public_key, m_test_public_key_a, sizeof(m_test_public_key_a)));

        // After receiving public key from joining node, the upper layer must specify our public key to be send. (This can also be done on initialization, but must happen no later than the received event).

        // Echo(KEX Report) frame received.
        // bit0: echo field set, bit1-7: Reserved.
        // Selected schemes: scheme 0 and scheme 2.
        // Selected curve25519
        // Keys to exchange, Security2, class 2 - Security0, network key.
        uint8_t s2_echo_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x05, 0x02, 0x01, 0x82};

        s2_context.buf    = s2_echo_kex_report_frame;
        s2_context.length = sizeof(s2_echo_kex_report_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // Network Key Report frame received.
        uint8_t s2_net_key_report_frame[3 + sizeof(m_test_network_key_s2_class_1)] = {
          COMMAND_CLASS_SECURITY_2,
          SECURITY_2_NETWORK_KEY_REPORT,
          0x02,
        };  // Keys requested, Security2, class 2 - Security0, network key.
        memcpy(&s2_net_key_report_frame[3], m_test_network_key_s2_class_1, sizeof(m_test_network_key_s2_class_1));
        s2_context.buf    = s2_net_key_report_frame;
        s2_context.length = sizeof(s2_net_key_report_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // S2 Transfer end frame received.
        // bit0: Key request complete not set,
        // bit1: Key verified set,
        // bit2-7: Reserved.
        uint8_t s2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x02};
        s2_context.buf                  = s2_transfer_end_frame;
        s2_context.length               = sizeof(s2_transfer_end_frame);
        s2_con.class_id                 = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // Network Key Report frame received.
        uint8_t s2_net_key_report_0_frame[3 + sizeof(m_test_network_key_s0)] = {
          COMMAND_CLASS_SECURITY_2,
          SECURITY_2_NETWORK_KEY_REPORT,
          0x80,
        };  // Keys requested, Security2, class 2 - Security0, network key.
        memcpy(&s2_net_key_report_0_frame[3], m_test_network_key_s0, sizeof(m_test_network_key_s0));
        s2_context.buf    = s2_net_key_report_0_frame;
        s2_context.length = sizeof(s2_net_key_report_0_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // S2 Transfer end frame received.
        // bit0: Key request complete not set,
        // bit1: Key verified set,
        // bit2-7: Reserved.
        s2_context.buf    = s2_transfer_end_frame;
        s2_context.length = sizeof(s2_transfer_end_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_send_done(&s2_context, true);

    } /* for(test_flavour) */
}

/** This test case is identical to @ref test_kex_joining_node_state_machine except that the
 *  joining node is sending the complete public key. For authenticated keys, the first two bytes are masked out.
 *
 * It verifies the behavior as seen from an including node (Controller node) as described in SDS11274.
 *
 * When a node is to be included securely it is expected that a ZW_SendData is send.
 * For this the common S2_send_frame(...) defined in s2.h is used, which will be implemented elsewhere.
 */
TEST_F(InclusionTest, test_kex_joining_node_state_machine_unauthenticated)
{
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_read");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_write");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    /**
     *  Test expectation setup.
     * This section set up the expectations in the system when the inclusion state machine progresses:
     * 1) Starting in idle it is expected that an S2 frame KEX1 will be transmitted based on external event when a node id has been assigned.
     * 2) After S2 KEX Get has been transmitted, the FSM awaits a S2 KEX Report as response.
     * 3) After S2 KEX Report is received then S2 KEX Set shall be sent.
     * 4) After S2 KEX Set  has been transmitted, the FSM awaits a Public KeyB from joining node.
     * 5) Exchange of public keys.
     *    a) Public KeyB is received from joining node A.
     *       - Public KeyB must be pushed upwards as event to be presented to user for further verification.
     *         If node is rejected, then node is part of network insecurely. Should this be changed ? Maybe force a node exclude.
     *         Currently that use case is outside the scope of libs2.
     *       - When Public KeyB is received, the controller must send its public KeyA.
     *         Public KeyB shall be provided by the application.
     *       - Using Pub KeyB and Private KeyA then a temporary key, KeyT is derived from the x coordinate of the shared secret.
     *         This is done in security sub module (currently under development)
     *       - The security context must be re-initialized with temporary derived key.
     *    b) Public KeyA is transmitted from including node A, the state machine will await 'Echo (KEX Set)'.
     * 6) Echoing of KEX frames.
     *    a) A 'Echo(KEX Set)' is expected to be received from the joining node, and it must be
     *       verified that 'Echo(KEX Set)' is identical to the KEX Set sent at step 3).
     *    b) 'Echo(KEX Report)' with content identical to packet received in 3) shall be replied to the joining node.
     * x) Nonce exchange: When joining node wants to send 'Echo(KEX Set)' the S2 protocol layer will exchange a nonce prior to communication.
     * 7) Network key exchange:
     *    a) Joining node will request the network key, by sending a 'Network Key Get'.
     *    b) Including node shall reply with a 'Network Key Set'.
     *    c) Joining node shall confirm with a 'Network Key Verify' which is encrypted using the the key obtained by 'NetworkKey-expand'
     *       - Including node must verify that it can succesfully decrypt the message using the key.
     *    d) Including node shall reply with a 'Transfer End'.
     *    e) Joining node shall reply with a 'Transfer End' if no more keys shall be exchanged.
     *
     */

    // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
    zwave_event_t *p_expected_inc_init_event = (zwave_event_t *)m_test_mem_pool[2];
    p_expected_inc_init_event->event_type    = S2_NODE_INCLUSION_INITIATED_EVENT;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_init_event))).RetiresOnSaturation();

    // Expect a S2 KEX Get to be sent.
    uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2,
                                     KEX_REPORT,
                                     0x04,   // reserved | NLS support | Request CSA | Echo
                                     0x02,   // Supported schemes. Scheme 0 and 2.
                                     0x01,   // Supported ECDH Profiles, bit0=1 is curve 25519 value.
                                     0x01};  // Requested keys bits. Security 2 class 0 key.
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Before exchange of public keys, then we expect that our public key is requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    uint8_t s2_public_key_frame[3 + sizeof(public_key_b)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x00};  // Key exchange received from slave - public key for secure exchange of LTK.
    memcpy(&s2_public_key_frame[3], public_key_b, sizeof(public_key_b));
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_public_key_frame, sizeof(s2_public_key_frame)), sizeof(s2_public_key_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving public key A, then we expect an event to be pushed up and both our public and private keys are requested from the keystore.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x01;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
    memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // Expect Echo(KEX Report) to be sent.
    uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x01};  // Note: Echo flag set.
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_get_2_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x01};  // Keys requested, Security2, class 2 - Security0, network key.
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_2_frame, sizeof(s2_net_key_get_2_frame)), sizeof(s2_net_key_get_2_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    //  // After receiving the network key, we expect to get a call to update the context to use the new key when transmitting Net key verify.
    //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
    //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
    //  p_mock->expect_arg[1].pointer = m_test_network_key_s2_class_1;

    uint8_t s2_net_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_frame, sizeof(s2_net_key_verify_frame)), sizeof(s2_net_key_verify_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect S2 Transfer End to be sent.
    uint8_t S2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x01};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_transfer_end_frame, sizeof(S2_transfer_end_frame)), sizeof(S2_transfer_end_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
    zwave_event_t *p_expected_complete_event                                          = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_complete_event->event_type                                             = S2_NODE_JOINING_COMPLETE_EVENT;
    p_expected_complete_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0x01;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_complete_event))).RetiresOnSaturation();

    /**
     *  Test execution.
     */
    struct S2 s2_context;

    s2_context.inclusion_state = S2_INC_IDLE;
    // memcpy(s2_context.temp_network_key, m_temp_key, sizeof(s2_context.temp_network_key));

    s2_inclusion_init(0x02, 0x01, 0x01);
    s2_inclusion_set_event_handler(s2_event_handler);
    helper_func_init_s2_conn();

    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    // KEX Get frame received.
    uint8_t S2_kex_get_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
    s2_context.buf             = S2_kex_get_frame;
    s2_context.length          = sizeof(S2_kex_get_frame);
    s2_con.class_id            = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // KEX report is expeted to be transmitted. Let's make the s2_send_frame a success.
    s2_inclusion_send_done(&s2_context, 1);

    // KEX Set frame received.
    // bit0: echo field not set, bit1-7: Reserved.
    // Selected schemes: scheme 0 and scheme 2.
    // Selected curve25519
    // Keys to exchange, Security2, class 2 - Security0, network key.
    uint8_t s2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x00, 0x02, 0x01, 0x01};
    s2_context.buf             = s2_kex_set_frame;
    s2_context.length          = sizeof(s2_kex_set_frame);
    s2_con.class_id            = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    uint8_t public_key_frame[3 + sizeof(m_test_public_key_a)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x01};  // Key exchange received from slave - public key for secure exchange of LTK.
    memcpy(&public_key_frame[3], m_test_public_key_a, sizeof(m_test_public_key_a));
    s2_context.buf    = public_key_frame;
    s2_context.length = sizeof(public_key_frame);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    // After receiving public key from joining node, the upper layer must specify our public key to be send. (This can also be done on initialization, but must happen no later than the received event).

    // Echo(KEX Report) frame received.
    // bit0: echo field set, bit1-7: Reserved.
    // Selected schemes: scheme 0 and scheme 2.
    // Selected curve25519
    // Keys to exchange, Security2, class 2 - Security0, network key.
    uint8_t s2_echo_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x05, 0x02, 0x01, 0x01};
    s2_context.buf                     = s2_echo_kex_report_frame;
    s2_context.length                  = sizeof(s2_echo_kex_report_frame);
    s2_con.class_id                    = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Network Key Report frame received.
    uint8_t s2_net_key_report_frame[3 + sizeof(m_test_network_key_s2_class_1)] = {
      COMMAND_CLASS_SECURITY_2,
      SECURITY_2_NETWORK_KEY_REPORT,
      0x01,
    };  // Keys requested, Security2, class 2 - Security0, network key.
    memcpy(&s2_net_key_report_frame[3], m_test_network_key_s2_class_1, sizeof(m_test_network_key_s2_class_0));
    s2_context.buf    = s2_net_key_report_frame;
    s2_context.length = sizeof(s2_net_key_report_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // S2 Transfer end frame received.
    // bit0: Key request complete not set,
    // bit1: Key verified set,
    // bit2-7: Reserved.
    uint8_t s2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x02};
    s2_context.buf                  = s2_transfer_end_frame;
    s2_context.length               = sizeof(s2_transfer_end_frame);
    s2_con.class_id                 = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    //  // S2 Transfer end frame received.
    //  // bit0: Key request complete not set,
    //  // bit1: Key verified set,
    //  // bit2-7: Reserved.
    //  s2_context.buf    = s2_transfer_end_frame;
    //  s2_context.length = sizeof(s2_transfer_end_frame);
    //  s2_con.class_id = 0x00;
    //  s2_inclusion_post_event_peer(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, true);
}

/** Test case for ensuring correct timer handling during inclusion.
 *
 *  Purpose: In order to ensure correct inclusion flow and avoid deadlock during inclusion it is
 *           important that in case messages are never received during inclusion, then the state
 *           machine must return to idle a push a corresponding event.
 *
 *  Not to test: This test case will only verify expectations related to the timer handling.
 *               Thus content of frames sent between the nodes during inclusion are not
 *               relevant for this test as they are verified in: test_kex_joining_node_state_machine()
 *               NOTE: messages related to Timeout will be verified in this test.
 */
TEST_F(InclusionTest, test_kex_joining_timer_handling)
{
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;

    libs2_clear_stubs();
    libs2_stub("S2_send_frame");
    libs2_stub("S2_send_data");
    libs2_stub("s2_event_handler");
    libs2_stub("S2_network_key_update");
    libs2_stub("s2_keystore_mock.c");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_init(0x02, 0x01, 0x82);
    s2_inclusion_set_event_handler(s2_event_handler);
    helper_func_init_s2_conn();

    /** Timer start expected on every transmitted frame.
     * For flow chart, see SDS11274.
     * Timer values:
     * - TB1, TB2, TB3, TB5, TB6, TB7 : 10s
     * - TBI1                         : 240s
     */

    // When neighbour discovery completes we expect the timer to be stopped.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).RetiresOnSaturation();

    // After neighbor discovery we expect to receive the KEX Get within 30 seconds.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(_, 3000)).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving next frame we expect the timer to be stopped.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).RetiresOnSaturation();

    // After transmitting KEX Report we expect a reset of the timer of 240s to allow for user interaction on remote side.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(_, 24000)).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving next frame we expect the timer to be stopped.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).RetiresOnSaturation();

    // After transmitting Public Key we expect a reset of the timer.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(_, 1000)).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving next frame we expect the timer to be stopped.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).RetiresOnSaturation();

    // After receiving Public Key we shall push event upwards and start an accept/reject timer for the user.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(_, 24000)).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving next frame we expect the timer to be stopped.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).RetiresOnSaturation();

    // After transmitting echo(KEX Set) we expect a reset of the timer.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(_, 1000)).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving next frame we expect the timer to be stopped.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).RetiresOnSaturation();

    // First round network key exchange.
    // After transmitting Network Key Get we expect a reset of the timer.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(_, 1000)).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving next frame we expect the timer to be stopped.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).RetiresOnSaturation();

    // After transmitting Network Key Verify we expect a reset of the timer.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(_, 1000)).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving next frame we expect the timer to be stopped.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).RetiresOnSaturation();

    // Second round network key exchange.
    // After transmitting Network Key Get we expect a reset of the timer.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(_, 1000)).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving next frame we expect the timer to be stopped.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).RetiresOnSaturation();

    // After transmitting Network Key Verify we expect a reset of the timer.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(_, 1000)).WillOnce(Return(1)).RetiresOnSaturation();
    // Network key exchange completed.

    // When receiving send done as confirmation of the final transfer end, we expect the timer to be stopped.
    EXPECT_CALL(*g_libs2_mocks, s2_inclusion_stop_timeout()).RetiresOnSaturation();

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    s2_inclusion_neighbor_discovery_complete(&s2_context);

    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, 1);
    helper_func_kex_set_frame(&s2_context, 0x02, 0x82);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, 1);
    helper_func_pub_key_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    s2_inclusion_send_done(&s2_context, 1);
    helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x82);
    s2_inclusion_post_event(&s2_context, &s2_con);

    // First round network key exchange.
    s2_inclusion_send_done(&s2_context, 1);
    helper_func_net_key_report_frame(&s2_context, 0x02);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, 1);
    helper_func_transfer_end_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);
    // Second round network key exchange.
    s2_inclusion_send_done(&s2_context, 1);
    helper_func_net_key_report_frame(&s2_context, 0x80);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, 1);
    helper_func_transfer_end_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);
    // Network key exchange completed.
    s2_inclusion_send_done(&s2_context, 1);
}

/** Test case for ensuring correct timer handling on timeout during inclusion in case a timeout
 *  occurs after any state.
 *
 *  Purpose: In order to ensure correct inclusion flow and avoid deadlock during inclusion it is
 *           important that in case messages are never received during inclusion, then the state
 *           machine must return to idle and push a corresponding event.
 *
 *           The event being pushed depends on the state in which the
 *           timeout happens.  If a joining node has received a KEX
 *           get, a timeout indicates fail.  Before the first KEX get,
 *           a timeout indicates non-secure inclusion.
 *
 *  Test: This test case will verify expectations related to the timer handling.
 *        If an including node does not respond within the timeout to a S2 inclusion frame then the
 *        state machine must return to idle and push the corresponding event upwards to application.
 *        After timeout, we expect the system to retry node inclusion.
 *        As this test case must covers timeout in ANY state, it will loop over the expectations and
 *        decrement a counter to go one step further on each loop.
 */
#define NO_OF_STATES 6
TEST_F(InclusionTest, test_kex_inclusion_timeout_all_states)
{
    uint32_t i;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_write");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);
    helper_func_init_s2_conn();
    s2_inclusion_init(0x02, 0x01, 0x82);

    /** The loop which will set up the expectation based on the round. */
    for (i = 0; i < NO_OF_STATES; i++) {
        // Node id (first step in inclusion) has been assigned.
        // Continue with secure inclusion.
        s2_inclusion_joining_start(&s2_context, &s2_con, 0);
        /* State is now S2_AWAITING_KEX_GET */
        if (i == 0) {
            // This test will trigger a timeout to the inclusion module after which we expect to receive a
            // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
            zwave_event_t *p_expected_event                                          = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type                                             = S2_JOINING_COMPLETE_NEVER_STARTED_EVENT;
            p_expected_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Trigger a timeout.
            s2_inclusion_notify_timeout(&s2_context);
            continue;  // if first iteration, continue here to test timeout for timeout.
        }

        helper_func_inclusion_initiated_event();
        /* State is now S2_AWAITING_KEX_SET */

        // Expect a S2 KEX Report to be sent.
        uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x82};
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        helper_func_kex_get_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);
        if (i == 1) {
            // This test will trigger a timeout to the inclusion module after which we expect to receive a
            // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
            zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Trigger a timeout.
            s2_inclusion_notify_timeout(&s2_context);
            continue;  // if second iteration, break here to test timeout for timeout.
        }

        // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
        EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        // When the KEX Set is received, we expect public keys to be exchanges.
        uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                        PUBLIC_KEY_REPORT,
                                        0x00,  // Including node bit not set
                                        0x00,
                                        0x00,
                                        0xCC,
                                        0xDD,
                                        0xEE,
                                        0xFF,
                                        0x11,
                                        0x22,  // Public key as being fetch from keystore.
                                        0xAA,
                                        0xBB,
                                        0xCC,
                                        0xDD,
                                        0xEE,
                                        0xFF,
                                        0x11,
                                        0x22,
                                        0xAA,
                                        0xBB,
                                        0xCC,
                                        0xDD,
                                        0xEE,
                                        0xFF,
                                        0x11,
                                        0x22,
                                        0xAA,
                                        0xBB,
                                        0xCC,
                                        0xDD,
                                        0xEE,
                                        0xFF,
                                        0x11,
                                        0x22};

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        helper_func_kex_set_frame(&s2_context, 0x02, 0x82);
        s2_inclusion_post_event(&s2_context, &s2_con);
        if (i == 2) {
            // This test will trigger a timeout to the inclusion module after which we expect to receive a
            // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
            zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Trigger a timeout.
            s2_inclusion_notify_timeout(&s2_context);
            continue;  // if third iteration, break here to test timeout for timeout.
        }

        // When the public key A is received, we expect that the event from libs2 contains the key in order to present it for the operator.
        // Therefore we copy the key minus header frame into expected data and push an event upwards.
        zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[1];
        p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
        memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

        // When receiving public key A, then we expect that both our public and private keys are requested from the keystore.
        EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
        EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        // When public key is received we expect an echo(KEX Set) to be sent.
        uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        helper_func_pub_key_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);
        if (i == 3) {
            uint8_t i;
            // The ECHO Kex Set is a special case where both a send done/failed can be returned but packet is dropped by remote side in case it waits for user input.
            // Each timeoyt/time between retry is 10 seconds. User input timeout is 240 seconds, thus the system should not abort inclusion until 240s/10s = 24 timeouts has occured.
            for (i = 0; i < 23; i++) {
                // When timeout occurs we expect a retry of the echo(KEX Set) to be sent.
                uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};
                EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(0)).RetiresOnSaturation();

                // Trigger a timeout.
                s2_inclusion_notify_timeout(&s2_context);
            }

            // This test will trigger a timeout to the inclusion module after which we expect to receive a
            // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
            zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Trigger the final timeout.
            s2_inclusion_notify_timeout(&s2_context);
            continue;  // if fourth iteration, break here to test timeout for timeout.
        }

        // Expect Net Key Get to be sent.
        uint8_t S2_net_key_get_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_net_key_get_frame, sizeof(S2_net_key_get_frame)), sizeof(S2_net_key_get_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x82);
        s2_inclusion_post_event(&s2_context, &s2_con);
        if (i == 4) {
            // This test will trigger a timeout to the inclusion module after which we expect to receive a
            // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
            zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Trigger a timeout.
            s2_inclusion_notify_timeout(&s2_context);
            continue;  // if fifth iteration, break here to test timeout for timeout.
        }

        // Expect Net Key Verify to be sent.
        uint8_t S2_network_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_network_key_verify_frame, sizeof(S2_network_key_verify_frame)), sizeof(S2_network_key_verify_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        helper_func_net_key_report_frame(&s2_context, 0x02);
        s2_inclusion_post_event(&s2_context, &s2_con);
        if (i == 5) {
            // This test will trigger a timeout to the inclusion module after which we expect to receive a
            // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
            zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Trigger a timeout.
            s2_inclusion_notify_timeout(&s2_context);
            continue;  // if sixth iteration, break here to test timeout for timeout.
        }
    }

    // After receiving a timeout it is expected that the inclusion state machine is back in idle and
    // that we can initiate a new inclusion which will also trigger a new set timeout call.
    helper_func_verify_idle_state(&s2_context, 0x02, 0x82);
}

/** Test case for ensuring correct handling when frames are received in wrong order during inclusion.
 *  This could either be implementation error or more critical, someone trying to find weakness in
 *  the system. Therefore the system must silently return to idle.
 *
 *  An error should still be returned upwards to the application.
 */
TEST_F(InclusionTest, test_kex_inclusion_invalid_frame_order_all_states)
{
    uint32_t i;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_write");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);
    s2_inclusion_init(0x02, 0x01, 0x82);

    /** The loop which will set up the expectation based on the round. */
    for (i = 0; i < 5; i++) {
        // Node id (first step in inclusion) has been assigned.
        // Continue with secure inclusion.
        s2_inclusion_joining_start(&s2_context, &s2_con, 0);
        if (i == 0) {
            uint8_t s2_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x00, 0x02, 0x01, 0x01};

            // This test will trigger a timeout to the inclusion module after which we expect to receive a
            // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
            zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Let's try to directly got to KEX Set.
            memcpy((uint8_t *)s2_context.buf, s2_frame, sizeof(s2_frame));
            s2_context.length = sizeof(s2_frame);
            s2_con.class_id   = 0xFF;
            s2_inclusion_post_event(&s2_context, &s2_con);
            continue;  // if first iteration, continue here to test timeout for timeout.
        }

        helper_func_inclusion_initiated_event();

        // Expect a S2 KEX Report to be sent.
        uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x82};
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        helper_func_kex_get_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);
        if (i == 1) {
            // The invalid frame - Let's send a KEX Report the opposite direction.
            uint8_t s2_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x00, 0x02, 0x01, 0x03};

            // This test will trigger a timeout to the inclusion module after which we expect to receive a
            // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
            zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Send the invalid frame.
            memcpy((uint8_t *)s2_context.buf, s2_frame, sizeof(s2_frame));
            s2_context.length = sizeof(s2_frame);
            s2_con.class_id   = 0xFF;
            s2_inclusion_post_event(&s2_context, &s2_con);
            continue;  // if second iteration, break here to test timeout for timeout.
        }

        // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
        EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        // When the KEX Set is received, we expect public keys to be exchanges.
        uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                        PUBLIC_KEY_REPORT,
                                        0x00,  // Including node bit not set
                                        0x00,
                                        0x00,
                                        0xCC,
                                        0xDD,
                                        0xEE,
                                        0xFF,
                                        0x11,
                                        0x22,  // Public key as being fetch from keystore.
                                        0xAA,
                                        0xBB,
                                        0xCC,
                                        0xDD,
                                        0xEE,
                                        0xFF,
                                        0x11,
                                        0x22,
                                        0xAA,
                                        0xBB,
                                        0xCC,
                                        0xDD,
                                        0xEE,
                                        0xFF,
                                        0x11,
                                        0x22,
                                        0xAA,
                                        0xBB,
                                        0xCC,
                                        0xDD,
                                        0xEE,
                                        0xFF,
                                        0x11,
                                        0x22};

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        helper_func_kex_set_frame(&s2_context, 0x02, 0x82);
        s2_inclusion_post_event(&s2_context, &s2_con);
        if (i == 2) {
            // In case including node has for unknown reason (could be timeout) has restarted the
            // inclusion, we could experience an unexpected KEX Get.
            uint8_t s2_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x01, 0x02, 0x01, 0x82};

            zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Send the invalid frame.
            memcpy((uint8_t *)s2_context.buf, s2_frame, sizeof(s2_frame));
            s2_context.length = sizeof(s2_frame);
            s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
            s2_inclusion_post_event(&s2_context, &s2_con);
            continue;  // if third iteration, break here to test timeout for timeout.
        }

        // When the public key A is received, we expect that the event from libs2 contains the key in order to present it for the operator.
        // Therefore we copy the key minus header frame into expected data and push an event upwards.
        zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[1];
        p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
        memcpy((uint8_t *)p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

        // When receiving public key A, then we expect that both our public and private keys are requested from the keystore.
        EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
        EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        // When public key is received we expect an echo(KEX Set) to be sent.
        uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};
        S2_echo_kex_set_frame[0]        = COMMAND_CLASS_SECURITY_2;
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        helper_func_pub_key_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);
        if (i == 3) {
            uint8_t s2_frame[19] = {
              COMMAND_CLASS_SECURITY_2,
              SECURITY_2_NETWORK_KEY_REPORT,
              0x02,
            };

            // This test will trigger a timeout to the inclusion module after which we expect to receive a
            // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
            zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Send the invalid frame.
            memcpy((uint8_t *)s2_context.buf, s2_frame, sizeof(s2_frame));
            s2_context.length = sizeof(s2_frame);
            s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
            s2_inclusion_post_event(&s2_context, &s2_con);
            continue;
        }

        // Expect Net Key Get to be sent.
        uint8_t S2_net_key_get_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_net_key_get_frame, sizeof(S2_net_key_get_frame)), sizeof(S2_net_key_get_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x82);
        s2_inclusion_post_event(&s2_context, &s2_con);
        if (i == 4) {
            uint8_t s2_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END};

            // This test will trigger a timeout to the inclusion module after which we expect to receive a
            // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
            zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
            p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
            EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

            // Send the invalid frame.
            memcpy((uint8_t *)s2_context.buf, s2_frame, sizeof(s2_frame));
            s2_context.length = sizeof(s2_frame);
            s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
            s2_inclusion_post_event(&s2_context, &s2_con);
            continue;
        }

        // Expect Net Key Verify to be sent.
        uint8_t S2_network_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_network_key_verify_frame, sizeof(S2_network_key_verify_frame)), sizeof(S2_network_key_verify_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        helper_func_net_key_report_frame(&s2_context, 0x02);
        s2_inclusion_post_event(&s2_context, &s2_con);
    }

    // After receiving a timeout it is expected that the inclusion state machine is back in idle and
    // that we can initiate a new inclusion which will also trigger a new set timeout call.
    helper_func_verify_idle_state(&s2_context, 0x02, 0x82);
}

/** Test case for ensuring that the inclusion state machine will not abort if an older inclusion
 *  frame is received multiple times, as example due to retransmission or routing.
 */
TEST_F(InclusionTest, test_kex_inclusion_duplicate_frame_all_states)
{
    uint32_t i;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_write");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);
    s2_inclusion_init(0x02, 0x01, 0x82);

    /******************************************************************************
     * Mock expectation section                                                   *
     * - Even though frames are received multiple times then the duplicate frames *
     *   are expected to be ignored and hence the wit is only expected that each  *
     *   frame is transmitted once from the joining node.                         *
     ******************************************************************************/
    helper_func_inclusion_initiated_event();

    // Expect a S2 KEX Report to be sent.
    uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x82};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When the KEX Set is received, we expect public keys to be exchanges.
    uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                    PUBLIC_KEY_REPORT,
                                    0x00,  // Including node bit not set
                                    0x00,
                                    0x00,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,  // Public key as being fetch from keystore.
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22};

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When the public key A is received, we expect that the event from libs2 contains the key in order to present it for the operator.
    // Therefore we copy the key minus header frame into expected data and push an event upwards.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
    memcpy((uint8_t *)p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    // When receiving public key A, then we expect that both our public and private keys are requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When public key is received we expect an echo(KEX Set) to be sent.
    uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};
    S2_echo_kex_set_frame[0]        = COMMAND_CLASS_SECURITY_2;
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect Net Key Get to be sent.
    uint8_t S2_net_key_get_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_net_key_get_frame, sizeof(S2_net_key_get_frame)), sizeof(S2_net_key_get_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect Net Key Verify to be sent.
    uint8_t S2_network_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_network_key_verify_frame, sizeof(S2_network_key_verify_frame)), sizeof(S2_network_key_verify_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_get_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x80};  // Keys requested, Security2, class 2 - Security0, network key.
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_0_frame, sizeof(s2_net_key_get_0_frame)), sizeof(s2_net_key_get_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_verify_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_0_frame, sizeof(s2_net_key_verify_0_frame)), sizeof(s2_net_key_verify_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect S2 Transfer End to be sent.
    uint8_t S2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x01};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_transfer_end_frame, sizeof(S2_transfer_end_frame)), sizeof(S2_transfer_end_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
    zwave_event_t *p_expected_complete_event                                          = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_complete_event->event_type                                             = S2_NODE_JOINING_COMPLETE_EVENT;
    p_expected_complete_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0x82;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_complete_event))).RetiresOnSaturation();

    /******************
     * Start the test *
     ******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    /** The loop which will set up the expectation based on the round. */
    //  for (i = 4; i < NO_OF_STATES; i++)
    for (i = 0; i < 7; i++) {
        helper_func_kex_get_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        if (i == 0)
            continue;

        helper_func_kex_set_frame(&s2_context, 0x02, 0x82);
        s2_inclusion_post_event(&s2_context, &s2_con);

        if (i == 1)
            continue;

        helper_func_pub_key_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        if (i == 2) {
            // After the received public key is pushed upwards in the system, then it is expected to receive a challenge response from the upper level.
            s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

            continue;
        }

        helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x82);
        s2_inclusion_post_event(&s2_context, &s2_con);

        if (i == 3)
            continue;

        // First round network key exchange.
        helper_func_net_key_report_frame(&s2_context, 0x02);
        s2_inclusion_post_event(&s2_context, &s2_con);

        if (i == 4)
            continue;

        helper_func_transfer_end_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        // Second round network key exchange.
        helper_func_net_key_report_frame(&s2_context, 0x80);
        s2_inclusion_post_event(&s2_context, &s2_con);

        if (i == 5)
            continue;

        helper_func_transfer_end_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);
        // Network key exchange completed.

        s2_inclusion_send_done(&s2_context, true);
    }
}

/** Test case for ensuring correct handling of node inclusion in case the including node is
 *  requesting the joining node to use schemes not requested in KEX Report.
 *
 *  The test will utilize two test vectors, where first component is the KEX Report, and second is
 *  the KEX Set which is incompatible with the KEX Report.
 *
 *  Purpose: In order to ensure a node can be correctly included there must be a match between the
 *           schemes supported by both inclusion and joining node. If this cannot be established
 *           between the node, then it must be ensured that appropriate failure is returned.
 */
TEST_F(InclusionTest, test_kex_inclusion_error_schemes)
{
    uint32_t i;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    uint8_t s2_kex_report_frame[6];
    uint8_t s2_kex_set_frame[6];
    struct S2 s2_context;

    uint8_t scheme_scheme_fail_test_vector[][4] = {
      {0x02, 0x01, 0x00, 0x01},  // Requested Scheme 1 = true, key request don't care. Invalid scheme in Set, This should return FAIL_KEY_REPORT.
      {0x02, 0x01, 0x01, 0x01},  // Requested Scheme 1 = true, key request don't care. Invalid scheme in Set, This should return FAIL_KEY_REPORT.
      {0x02, 0x01, 0x03, 0x01},  // Requested Scheme 1 = true, key request don't care. Invalid scheme in Set, This should return FAIL_KEY_REPORT.
      {0x02, 0x01, 0x04, 0x01},  // Requested Scheme 1 = true, key request don't care. Invalid scheme in Set, This should return FAIL_KEY_REPORT.
      {0x02, 0x01, 0x06, 0x01},  // Requested Scheme 1 = true, key request don't care. Invalid scheme in Set, This should return FAIL_KEY_REPORT.
      {0x02, 0x01, 0x80, 0x01},  // Requested Scheme 1 = true, key request don't care. Invalid scheme in Set, This should return FAIL_KEY_REPORT.
      {0x02, 0x01, 0xFF, 0x01},  // Requested Scheme 1 = true, key request don't care. Invalid scheme in Set, This should return FAIL_KEY_REPORT.
    };

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    for (i = 0; i < ELEM_COUNT(scheme_scheme_fail_test_vector); i++) {

        /****************************
         * Mock expectation section *
         ****************************/
        s2_inclusion_init(scheme_scheme_fail_test_vector[i][0], 0x01, scheme_scheme_fail_test_vector[i][1]);

        helper_func_inclusion_initiated_event();

        // After including node has sent a KEX Get, a response with above settings is expected.
        s2_kex_report_frame[0]   = COMMAND_CLASS_SECURITY_2;
        s2_kex_report_frame[1]   = KEX_REPORT;
        s2_kex_report_frame[2]   = 0x04,                                  // reserved | NLS support | Request CSA | Echo
          s2_kex_report_frame[3] = scheme_scheme_fail_test_vector[i][0];  // Supported schemes.
        s2_kex_report_frame[4]   = 0x01,                                  // Supported ECDH Profiles, bit0=1 is curve 25519 value.
          s2_kex_report_frame[5] = scheme_scheme_fail_test_vector[i][1];  // Requested keys bit.
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_kex_report_frame, sizeof(s2_kex_report_frame)), sizeof(s2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // Expect a S2 KEX Fail as the including node received an unsupported combination of security schemes.
        uint8_t S2_kex_set_error_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_FAIL, 0x02};  // KEX_FAIL_KEX_SCHEME = 0x02
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_set_error_frame, sizeof(S2_kex_set_error_frame)), sizeof(S2_kex_set_error_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        zwave_event_t *p_expected_event                                     = (zwave_event_t *)m_test_mem_pool[0];
        p_expected_event->event_type                                        = S2_NODE_INCLUSION_FAILED_EVENT;
        p_expected_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type = 0x02;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

        /*******************
         * Testing section *
         *******************/

        // Node id (first step in inclusion) has been assigned.
        // Continue with secure inclusion.
        s2_inclusion_joining_start(&s2_context, &s2_con, 0);

        // KEX Get frame received.
        helper_func_kex_get_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        // KEX Set frame received - With an unsupported/not requested scheme set.
        s2_kex_set_frame[0]   = COMMAND_CLASS_SECURITY_2;
        s2_kex_set_frame[1]   = KEX_SET;
        s2_kex_set_frame[2]   = 0x04,                                  // reserved | NLS support | Request CSA | Echo
          s2_kex_set_frame[3] = scheme_scheme_fail_test_vector[i][2];  // Supported schemes.
        s2_kex_set_frame[4]   = 0x01,                                  // Supported ECDH Profiles, bit0=1 is curve 25519 value.
          s2_kex_set_frame[5] = scheme_scheme_fail_test_vector[i][3];  // Requested keys bit.

        s2_context.buf    = s2_kex_set_frame;
        s2_context.length = sizeof(s2_kex_set_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_send_done(&s2_context, 1);
    }

    // Retry node inclusion in order to verify we are in idle.
    helper_func_verify_idle_state(&s2_context, 0x02, 0x01);

    //  s2_kex_set_frame[0] = s2_kex_set_frame[0];
}

/** Test case for ensuring that a proper error message is returned in case the inclusion state
 *  machine is initialized with an invalid set of schemes.
 */
TEST_F(InclusionTest, test_kex_inclusion_init_errors)
{
    uint8_t ret_val;

    uint8_t valid_schemes_bits = 0x02;                 // Scheme 1 supported.
    uint8_t valid_curve_bits   = 0x01;                 // Curve25519 supported.
    uint8_t valid_keys_bits    = SECURITY_2_KEY_MASK;  // Security 2, class 0, 1, 2, 3(LR), 4(LR) keys and Security 0 network key.
    uint8_t i;

    for (i = 0; i < 255; i++) {
        if ((~valid_schemes_bits & i) || (i == 0)) {
            ret_val = s2_inclusion_init(i, 0x01, 0x81);
            EXPECT_EQ(0x02, ret_val);
        }

        if (((~valid_schemes_bits & i) == 0) && (valid_schemes_bits & i)) {
            ret_val = s2_inclusion_init(i, 0x01, 0x81);
            EXPECT_EQ(0x00, ret_val);
        }
    }

    for (i = 0; i < 255; i++) {
        if ((~valid_curve_bits & i) || (i == 0)) {
            ret_val = s2_inclusion_init(0x02, i, 0x81);
            EXPECT_EQ(0x03, ret_val);
        }

        if ((~valid_curve_bits & i) == 0 && (valid_curve_bits & i)) {
            ret_val = s2_inclusion_init(0x02, i, 0x81);
            EXPECT_EQ(0x00, ret_val);
        }
    }

    for (i = 0; i < 255; i++) {
        if ((~valid_keys_bits & i) || (i == 0)) {
            ret_val = s2_inclusion_init(0x02, 0x01, i);
            EXPECT_EQ(0x01, ret_val);
        }

        if ((~valid_keys_bits & i) == 0 && (valid_keys_bits & i)) {
            ret_val = s2_inclusion_init(0x02, 0x01, i);
            EXPECT_EQ(0x00, ret_val);
        }
    }
}

/** Test case for ensuring correct handling of node inclusion in case the node is not supporting
 *  curve 25519, or other curves requested are not (yet) supported by the controller.
 *
 *  Purpose: In order to ensure a node can be correctly included there must be a match between the
 *           curves supported by both inclusion and joining node. If the curve supported by joining
 *           node is not matching a curve on inclusion node a failure is returned.
 *
 *  Test: This test case will a KEX_FAIL_CURVES is returned when invalid set of schemes is requested.
 */
TEST_F(InclusionTest, test_kex_inclusion_error_curves)
{
    zwave_event_t *p_expected_event;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    uint8_t i;
    struct S2 s2_context;

    uint8_t s2_kex_report_frame[6];
    uint8_t s2_kex_set_frame[6];
    uint8_t curves_fail_test_vector[][2] = {
      {0x01, 0x00},  // Supported curve25519 = true, No curves selected, This should return FAIL_KEY_CURVES.
      {0x01, 0x02},  // Supported curve25519 = true, Unknown/unsupported curve selected, This should return FAIL_KEY_CURVES.
      {0x01, 0xF0},  // Supported curve25519 = true, Multiple unknown/unsupported curve selected, This should return FAIL_KEY_CURVES.
      {0x01, 0x11},  // Supported curve25519 = true, Multiple unknown/unsupported but also a supported curve selected, This should return FAIL_KEY_CURVES.
    };

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    for (i = 0; i < ELEM_COUNT(curves_fail_test_vector); i++) {

        /****************************
         * Mock expectation section *
         ****************************/
        s2_inclusion_init(0x02, curves_fail_test_vector[i][0], 0x01);

        helper_func_inclusion_initiated_event();

        // After including node has sent a KEX Get, a response with above settings is expected.
        s2_kex_report_frame[0]   = COMMAND_CLASS_SECURITY_2;
        s2_kex_report_frame[1]   = KEX_REPORT;
        s2_kex_report_frame[2]   = 0x04,                           // reserved | NLS support | Request CSA | Echo
          s2_kex_report_frame[3] = 0x02;                           // Supported schemes.
        s2_kex_report_frame[4]   = curves_fail_test_vector[i][0],  // Supported ECDH Profiles, bit0=1 is curve 25519 value.
          s2_kex_report_frame[5] = 0x01;                           // Requested keys bit, Class1.
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_kex_report_frame, sizeof(s2_kex_report_frame)), sizeof(s2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // Expect a S2 KEX Fail as the including node received an unsupported combination of security schemes.
        uint8_t S2_kex_set_error_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_FAIL, 0x03};  // KEX_FAIL_KEX_CURVES = 0x03
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_set_error_frame, sizeof(S2_kex_set_error_frame)), sizeof(S2_kex_set_error_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        p_expected_event                                                    = (zwave_event_t *)m_test_mem_pool[0];
        p_expected_event->event_type                                        = S2_NODE_INCLUSION_FAILED_EVENT;
        p_expected_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type = 0x03;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

        /*******************
         * Testing section *
         *******************/

        // Node id (first step in inclusion) has been assigned.
        // Continue with secure inclusion.
        s2_inclusion_joining_start(&s2_context, &s2_con, 0);

        // KEX Get frame received.
        helper_func_kex_get_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        // KEX Set frame received - With an unsupported/not requested scheme set.
        s2_kex_set_frame[0]   = COMMAND_CLASS_SECURITY_2;
        s2_kex_set_frame[1]   = KEX_SET;
        s2_kex_set_frame[2]   = 0x00,                           // bit 0 is echo field, rest are reserved
          s2_kex_set_frame[3] = 0x02;                           // Supported schemes.
        s2_kex_set_frame[4]   = curves_fail_test_vector[i][1],  // Supported ECDH Profiles, bit0=1 is curve 25519 value.
          s2_kex_set_frame[5] = 0x01;                           // Requested keys bit.

        s2_context.buf    = s2_kex_set_frame;
        s2_context.length = sizeof(s2_kex_set_frame);
        s2_con.class_id   = 0xFF;

        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_send_done(&s2_context, 1);
    }

    // Retry node inclusion.
    helper_func_verify_idle_state(&s2_context, 0x02, 0x01);

    //  s2_kex_set_frame[0] = s2_kex_set_frame[0];
}

/** Test case for ensuring correct handling of node inclusion in case there is an invalid combination
 *  of key report/key set request.
 *
 *  Purpose: In order to ensure a node can be correctly included there must be a match between the
 *           keys supported by both inclusion and joining node and the request for keys to exchange.
 *
 *  Test: This test case will a KEX_FAIL_KEY is returned when invalid set of schemes/keys is requested.
 */
TEST_F(InclusionTest, test_kex_inclusion_error_keys)
{
    uint32_t i;
    zwave_event_t *p_expected_event;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    uint8_t s2_kex_report_frame[6];
    uint8_t s2_kex_set_frame[6];
    struct S2 s2_context;

    uint8_t s2_kex_error_frame[]              = {COMMAND_CLASS_SECURITY_2, KEX_FAIL, 0x01};  // KEX_FAIL_KEX_KEY = 0x01
    uint8_t key_request_fail_test_vector[][2] = {
      {0x80, 0x83},  // Key request S2 = no,   key request S0 = yes. Keys granted, S2 = yes, S0 = yes. This should return FAIL_KEY_REPORT.
      {0x01, 0x03},  // Key request S2 = some, key request S0 = no.  Keys granted, S2 = all, S0 = no.  This should return FAIL_KEY_REPORT.
      {0x02, 0x03},  // Key request S2 = some, key request S0 = no.  Keys granted, S2 = all, S0 = no.  This should return FAIL_KEY_REPORT.
      {0x81, 0x03},  // Key request S2 = some, key request S0 = yes. Keys granted, S2 = all, S0 = no.  This should return FAIL_KEY_REPORT.
      {0x82, 0x03},  // Key request S2 = some, key request S0 = yes. Keys granted, S2 = all, S0 = no.  This should return FAIL_KEY_REPORT.
      {0x83, 0x04},  // Key request S2 = yes,  key request S0 = yes. Keys granted, Sx = yes, S0 = no.  This should return FAIL_KEY_REPORT.  };
      {0x83, 0xFF},  // Key request S2 = yes,  key request S0 = yes. Keys granted, Sx = yes, S0 = no.  This should return FAIL_KEY_REPORT.  };
      {0x83, 0x7C},  // Key request S2 = yes,  key request S0 = yes. Keys granted, Sx = yes, S0 = no.  This should return FAIL_KEY_REPORT.  };
      {0x03, 0x07},  // Key request S2 = yes , key request S0 = no.  Keys granted, S2 = yes, Sx = yes. This should return FAIL_KEY_REPORT.
    };

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    for (i = 0; i < ELEM_COUNT(key_request_fail_test_vector); i++) {

        /****************************
         * Mock expectation section *
         ****************************/
        s2_inclusion_init(0x02, 0x01, key_request_fail_test_vector[i][0]);

        helper_func_inclusion_initiated_event();

        // After including node has sent a KEX Get, a response with above settings is expected.
        s2_kex_report_frame[0]   = COMMAND_CLASS_SECURITY_2;
        s2_kex_report_frame[1]   = KEX_REPORT;
        s2_kex_report_frame[2]   = 0x04,                                // reserved | NLS support | Request CSA | Echo
          s2_kex_report_frame[3] = 0x02;                                // Supported schemes.
        s2_kex_report_frame[4]   = 0x01,                                // Supported ECDH Profiles, bit0=1 is curve 25519 value.
          s2_kex_report_frame[5] = key_request_fail_test_vector[i][0];  // Requested keys bit.
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_kex_report_frame, sizeof(s2_kex_report_frame)), sizeof(s2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // Expect a S2 KEX Fail as the including node received an unsupported combination of security schemes.
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_kex_error_frame, sizeof(s2_kex_error_frame)), sizeof(s2_kex_error_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        p_expected_event                                                    = (zwave_event_t *)m_test_mem_pool[0];
        p_expected_event->event_type                                        = S2_NODE_INCLUSION_FAILED_EVENT;
        p_expected_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type = 0x01;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

        /*******************
         * Testing section *
         *******************/

        // Node id (first step in inclusion) has been assigned.
        // Continue with secure inclusion.
        s2_inclusion_joining_start(&s2_context, &s2_con, 0);

        // KEX Get frame received.
        helper_func_kex_get_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        // KEX Set frame received - With an unsupported/not requested scheme set.
        s2_kex_set_frame[0]   = COMMAND_CLASS_SECURITY_2;
        s2_kex_set_frame[1]   = KEX_SET;
        s2_kex_set_frame[2]   = 0x00,                                // bit 0 is echo field, rest are reserved
          s2_kex_set_frame[3] = 0x02;                                // Supported schemes.
        s2_kex_set_frame[4]   = 0x01,                                // Supported ECDH Profiles, bit0=1 is curve 25519 value.
          s2_kex_set_frame[5] = key_request_fail_test_vector[i][1];  // Granted keys bit.

        s2_context.buf    = s2_kex_set_frame;
        s2_context.length = sizeof(s2_kex_set_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_send_done(&s2_context, 1);
    }

    // Retry node inclusion.
    helper_func_verify_idle_state(&s2_context, 0x02, 0x03);

    //  s2_kex_set_frame[0] = s2_kex_set_frame[0];
}

/** Test case for ensuring correct handling of node inclusion in case the including node is
 *  transmitting its public key without including node flag set, TO#7554.
 *
 *  Purpose: In order to ensure correct key exchange it is important that the nodes agrees on who
 *  is including/joining node.
 *
 *  Test: This test case will ensure that the state machine returns to idle as the wrong including
 *  bit is identical to a unexpected frame at this stages in the state machine.
 */
TEST_F(InclusionTest, test_kex_inclusion_error_public_key_including_bit)
{
    zwave_event_t *p_expected_event;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    static uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                                     0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    /****************************
     * Mock expectation section *
     ****************************/
    s2_inclusion_init(0x02, 0x01, 0x83);

    helper_func_inclusion_initiated_event();

    // Expect a S2 KEX Report to be sent.
    static uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x83};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

    // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When the KEX Set is received, we expect public keys to be exchanges.
    static uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                           PUBLIC_KEY_REPORT,
                                           0x00,  // Including node bit not set
                                           0x00,
                                           0x00,
                                           0xCC,
                                           0xDD,
                                           0xEE,
                                           0xFF,
                                           0x11,
                                           0x22,  // Public key as being fetch from keystore.
                                           0xAA,
                                           0xBB,
                                           0xCC,
                                           0xDD,
                                           0xEE,
                                           0xFF,
                                           0x11,
                                           0x22,
                                           0xAA,
                                           0xBB,
                                           0xCC,
                                           0xDD,
                                           0xEE,
                                           0xFF,
                                           0x11,
                                           0x22,
                                           0xAA,
                                           0xBB,
                                           0xCC,
                                           0xDD,
                                           0xEE,
                                           0xFF,
                                           0x11,
                                           0x22};
    // If the node uses only unauthenticated keys, then full public key is expected, else the first two bytes are zero'ed out.

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(0)).RetiresOnSaturation();

    p_expected_event                                                    = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_event->event_type                                        = S2_NODE_INCLUSION_FAILED_EVENT;
    p_expected_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type = 0;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

    /*******************
     * Testing section *
     *******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_kex_set_frame(&s2_context, 0x02, 0x83);
    s2_inclusion_post_event(&s2_context, &s2_con);

    frame_buffer[0] = COMMAND_CLASS_SECURITY_2;
    frame_buffer[1] = PUBLIC_KEY_REPORT;
    frame_buffer[2] = 0x00;
    memcpy(&frame_buffer[3], m_test_public_key_a, sizeof(m_test_public_key_a));
    s2_context.length = 3 + sizeof(m_test_public_key_a);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_verify_idle_state(&s2_context, 0x02, 0x83);
}

/** Test case for ensuring correct handling of node inclusion in case the including node is
 *  transmitting frames with reserved bits set (which should be ignored).
 */
TEST_F(InclusionTest, test_kex_inclusion_public_key_reserved_bits)
{
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_write");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);
    s2_inclusion_init(0x02, 0x01, 0x82);

    /******************************************************************************
     * Mock expectation section                                                   *
     * - Even though frames are received multiple times then the duplicate frames *
     *   are expected to be ignored and hence the wit is only expected that each  *
     *   frame is transmitted once from the joining node.                         *
     ******************************************************************************/
    helper_func_inclusion_initiated_event();

    // Expect a S2 KEX Report to be sent.
    uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x82};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When the KEX Set is received, we expect public keys to be exchanges.
    uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                    PUBLIC_KEY_REPORT,
                                    0x00,  // Including node bit not set
                                    0x00,
                                    0x00,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,  // Public key as being fetch from keystore.
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22};

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When the public key A is received, we expect that the event from libs2 contains the key in order to present it for the operator.
    // Therefore we copy the key minus header frame into expected data and push an event upwards.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
    memcpy((uint8_t *)p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    // When receiving public key A, then we expect that both our public and private keys are requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When public key is received we expect an echo(KEX Set) to be sent.
    uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};
    S2_echo_kex_set_frame[0]        = COMMAND_CLASS_SECURITY_2;
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect Net Key Get to be sent.
    uint8_t S2_net_key_get_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_net_key_get_frame, sizeof(S2_net_key_get_frame)), sizeof(S2_net_key_get_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect Net Key Verify to be sent.
    uint8_t S2_network_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_network_key_verify_frame, sizeof(S2_network_key_verify_frame)), sizeof(S2_network_key_verify_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_get_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x80};  // Keys requested, Security2, class 2 - Security0, network key.
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_0_frame, sizeof(s2_net_key_get_0_frame)), sizeof(s2_net_key_get_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_verify_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_0_frame, sizeof(s2_net_key_verify_0_frame)), sizeof(s2_net_key_verify_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect S2 Transfer End to be sent.
    uint8_t S2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x01};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_transfer_end_frame, sizeof(S2_transfer_end_frame)), sizeof(S2_transfer_end_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
    zwave_event_t *p_expected_complete_event                                          = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_complete_event->event_type                                             = S2_NODE_JOINING_COMPLETE_EVENT;
    p_expected_complete_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0x82;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_complete_event))).RetiresOnSaturation();

    /******************
     * Start the test *
     ******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_kex_set_frame(&s2_context, 0x02, 0x82);
    s2_inclusion_post_event(&s2_context, &s2_con);

    frame_buffer[0] = COMMAND_CLASS_SECURITY_2;
    frame_buffer[1] = PUBLIC_KEY_REPORT;
    frame_buffer[2] = 0xA5;  // Setting a couple of reserved bits which should be ignored on remote side.
    memcpy(&frame_buffer[3], m_test_public_key_a, sizeof(m_test_public_key_a));
    s2_context.buf    = frame_buffer;
    s2_context.length = 3 + sizeof(m_test_public_key_a);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // After the received public key is pushed upwards in the system, then it is expected to receive a challenge response from the upper level.
    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x82);
    s2_inclusion_post_event(&s2_context, &s2_con);

    // First round network key exchange.
    helper_func_net_key_report_frame(&s2_context, 0x02);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_transfer_end_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Second round network key exchange.
    helper_func_net_key_report_frame(&s2_context, 0x80);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_transfer_end_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);
    // Network key exchange completed.

    s2_inclusion_send_done(&s2_context, true);
}

/** Test case for ensuring correct handling of node inclusion in case the received echo KEX Report
 *  did not match previuos sent packet.
 *
 *  Test: This test case will verify that a KEX_FAIL_AUTH is returned if the echo'ed KEX Report dod not match.
 */
TEST_F(InclusionTest, test_kex_inclusion_error_echo_kex_report)
{
    uint32_t i;
    zwave_event_t *p_expected_event;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    static uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                                     0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t echo_kex_report_fail_test_vector[][4] = {
      // Scheme errors.
      {0x83, 0x00, 0x01, 0x83},  // Key request S2 = yes, key request S0 = yes. Echo report, scheme=unknown/mixed, curve=curve25519, keys=S2 - class 0/1, S0. This should return KEX_FAIL_AUTH.
      {0x83, 0x01, 0x01, 0x83},  // Key request S2 = yes, key request S0 = yes. Echo report, scheme=unknown/mixed, curve=curve25519, keys=S2 - class 0/1, S0. This should return KEX_FAIL_AUTH.
      {0x83, 0x03, 0x01, 0x83},  // Key request S2 = yes, key request S0 = yes. Echo report, scheme=unknown/mixed, curve=curve25519, keys=S2 - class 0/1, S0. This should return KEX_FAIL_AUTH.
      {0x83, 0x80, 0x01, 0x83},  // Key request S2 = yes, key request S0 = yes. Echo report, scheme=unknown/mixed, curve=curve25519, keys=S2 - class 0/1, S0. This should return KEX_FAIL_AUTH.
      {0x83, 0xFF, 0x01, 0x83},  // Key request S2 = yes, key request S0 = yes. Echo report, scheme=unknown/mixed, curve=curve25519, keys=S2 - class 0/1, S0. This should return KEX_FAIL_AUTH.
      // Curve errors.
      {0x83, 0x02, 0x00, 0x82},  // Key request S2 = yes, key request S0 = yes. Echo report, scheme=1, curve=unknown/mixed, keys=S2 - class 0/1, S0. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x02, 0x82},  // Key request S2 = yes, key request S0 = yes. Echo report, scheme=1, curve=unknown/mixed, keys=S2 - class 0/1, S0. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x03, 0x82},  // Key request S2 = yes, key request S0 = yes. Echo report, scheme=1, curve=unknown/mixed, keys=S2 - class 0/1, S0. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x80, 0x82},  // Key request S2 = yes, key request S0 = yes. Echo report, scheme=1, curve=unknown/mixed, keys=S2 - class 0/1, S0. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0xFF, 0x82},  // Key request S2 = yes, key request S0 = yes. Echo report, scheme=1, curve=unknown/mixed, keys=S2 - class 0/1, S0. This should return KEX_FAIL_AUTH.

      // Key errors.
      {0x83, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x01, 0x80},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x01, 0x81},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x01, 0x82},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x01, 0x01},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x01, 0x02},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x01, 0x03},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x01, 0xFF},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x83, 0x02, 0x01, 0x7C},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.

      {0x81, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x81, 0x02, 0x01, 0x80},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x81, 0x02, 0x01, 0x82},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x81, 0x02, 0x01, 0x83},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x81, 0x02, 0x01, 0x01},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x81, 0x02, 0x01, 0x02},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x81, 0x02, 0x01, 0x03},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x81, 0x02, 0x01, 0xFF},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x81, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x81, 0x02, 0x01, 0x7C},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.

      {0x82, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x82, 0x02, 0x01, 0x80},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x82, 0x02, 0x01, 0x81},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x82, 0x02, 0x01, 0x83},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x82, 0x02, 0x01, 0x01},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x82, 0x02, 0x01, 0x02},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x82, 0x02, 0x01, 0x03},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x82, 0x02, 0x01, 0xFF},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x82, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x82, 0x02, 0x01, 0x7C},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.

      {0x80, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x80, 0x02, 0x01, 0x81},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x80, 0x02, 0x01, 0x82},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x80, 0x02, 0x01, 0x83},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x80, 0x02, 0x01, 0x01},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x80, 0x02, 0x01, 0x02},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x80, 0x02, 0x01, 0x03},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x80, 0x02, 0x01, 0xFF},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x80, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x80, 0x02, 0x01, 0x7C},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.

      {0x01, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x01, 0x02, 0x01, 0x80},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x01, 0x02, 0x01, 0x81},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x01, 0x02, 0x01, 0x82},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x01, 0x02, 0x01, 0x83},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x01, 0x02, 0x01, 0x02},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x01, 0x02, 0x01, 0x03},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x01, 0x02, 0x01, 0xFF},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x01, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x01, 0x02, 0x01, 0x7C},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.

      {0x02, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x02, 0x02, 0x01, 0x80},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x02, 0x02, 0x01, 0x81},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x02, 0x02, 0x01, 0x82},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x02, 0x02, 0x01, 0x83},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x02, 0x02, 0x01, 0x01},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x02, 0x02, 0x01, 0x03},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x02, 0x02, 0x01, 0xFF},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x02, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x02, 0x02, 0x01, 0x7C},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.

      {0x03, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x03, 0x02, 0x01, 0x80},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x03, 0x02, 0x01, 0x81},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x03, 0x02, 0x01, 0x82},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x03, 0x02, 0x01, 0x83},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x03, 0x02, 0x01, 0x01},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x03, 0x02, 0x01, 0x02},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x03, 0x02, 0x01, 0xFF},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x03, 0x02, 0x01, 0x00},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
      {0x03, 0x02, 0x01, 0x7C},  // ECHO KEX Report - Error in key field. This should return KEX_FAIL_AUTH.
    };

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    /** The loop which will set up the expectation based on the round. */
    for (i = 0; i < ELEM_COUNT(echo_kex_report_fail_test_vector); i++) {

        /****************************
         * Mock expectation section *
         ****************************/
        s2_inclusion_init(0x02, 0x01, echo_kex_report_fail_test_vector[i][0]);

        helper_func_inclusion_initiated_event();

        // Expect a S2 KEX Report to be sent.
        static uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x00};
        S2_kex_report_frame[5]               = echo_kex_report_fail_test_vector[i][0];
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
        // Use dynamic keypair if only S0 and/or S2 unauthenticated are requested
        if (0 != (echo_kex_report_fail_test_vector[i][0] & ~(KEY_CLASS_S0 | KEY_CLASS_S2_UNAUTHENTICATED))) {
            EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();
        } else {
            EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();
        }

        // When the KEX Set is received, we expect public keys to be exchanges.
        static uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                               PUBLIC_KEY_REPORT,
                                               0x00,  // Including node bit not set
                                               0xAA,
                                               0xBB,
                                               0xCC,
                                               0xDD,
                                               0xEE,
                                               0xFF,
                                               0x11,
                                               0x22,  // Public key as being fetch from keystore.
                                               0xAA,
                                               0xBB,
                                               0xCC,
                                               0xDD,
                                               0xEE,
                                               0xFF,
                                               0x11,
                                               0x22,
                                               0xAA,
                                               0xBB,
                                               0xCC,
                                               0xDD,
                                               0xEE,
                                               0xFF,
                                               0x11,
                                               0x22,
                                               0xAA,
                                               0xBB,
                                               0xCC,
                                               0xDD,
                                               0xEE,
                                               0xFF,
                                               0x11,
                                               0x22};
        // If the node uses only unauthenticated keys, then full public key is expected, else the first two bytes are zero'ed out.
        if (0x06 & echo_kex_report_fail_test_vector[i][0]) {
            S2_pub_key_B_frame[3] = 0x00;
            S2_pub_key_B_frame[4] = 0x00;
        } else {
            S2_pub_key_B_frame[3] = 0xAA;
            S2_pub_key_B_frame[4] = 0xBB;
        }

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // When the public key A is received, we expect that the event from libs2 contains the key in order to present it for the operator.
        // Therefore we copy the key minus header frame into expected data and push an event upwards.
        zwave_event_t *p_expected_inc_req_event = (zwave_event_t *)m_test_mem_pool[1];
        p_expected_inc_req_event->event_type    = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
        //    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.security_keys = echo_kex_report_fail_test_vector[i][0];
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = echo_kex_report_fail_test_vector[i][0];
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
        memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

        // When receiving public key A, then we expect that both our public and private keys are requested from the keystore.
        // Use dynamic keypair if only S0 and/or S2 unauthenticated are requested
        if (0 != (echo_kex_report_fail_test_vector[i][0] & ~(KEY_CLASS_S0 | KEY_CLASS_S2_UNAUTHENTICATED))) {
            EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
        } else {
            EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
        }

        // Use dynamic keypair if only S0 and/or S2 unauthenticated are requested
        if (0 != (echo_kex_report_fail_test_vector[i][0] & ~(KEY_CLASS_S0 | KEY_CLASS_S2_UNAUTHENTICATED))) {
            EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();
        } else {
            EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();
        }

        // When public key is received we expect an echo(KEX Set) to be sent.
        static uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x00};
        S2_echo_kex_set_frame[5]               = echo_kex_report_fail_test_vector[i][0];
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // Expect a S2 KEX Fail as the including node received an Key Set command.
        uint8_t S2_kex_set_error_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_FAIL, 0x07};  // KEX_FAIL_AUTH = 0x07
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_set_error_frame, sizeof(S2_kex_set_error_frame)), sizeof(S2_kex_set_error_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        p_expected_event                                                    = (zwave_event_t *)m_test_mem_pool[0];
        p_expected_event->event_type                                        = S2_NODE_INCLUSION_FAILED_EVENT;
        p_expected_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type = 0x07;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

        /*******************
         * Testing section *
         *******************/

        // Node id (first step in inclusion) has been assigned.
        // Continue with secure inclusion.
        s2_inclusion_joining_start(&s2_context, &s2_con, 0);

        helper_func_kex_get_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        helper_func_kex_set_frame(&s2_context, 0x02, echo_kex_report_fail_test_vector[i][0]);
        s2_inclusion_post_event(&s2_context, &s2_con);

        helper_func_pub_key_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

        helper_func_echo_kex_report_frame(&s2_context, echo_kex_report_fail_test_vector[i][1], echo_kex_report_fail_test_vector[i][2], echo_kex_report_fail_test_vector[i][3]);
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_send_done(&s2_context, 1);
    }

    // After receiving a timeout it is expected that the inclusion state machine is back in idle and
    // that we can initiate a new inclusion which will also trigger a new set timeout call.
    //  helper_func_verify_idle_state(&s2_context, 0x02, 0x82);
}

/**
 */
TEST_F(InclusionTest, test_kex_inclusion_invalid_key_report_set)
{
    zwave_event_t *p_expected_event;
    uint32_t i;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    static uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                                     0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t key_report_fail_test_vector[][2] = {
      {0x80, 0x83},  // Key request S0. Key report multiple. This should return KEX_FAIL_KEY_SET.
      {0x80, 0x01},  // Key request S0. Key report single, wrong key. This should return KEX_FAIL_KEY_SET.
      {0x80, 0x02},  // Key request S0. Key report single, wrong key. This should return KEX_FAIL_KEY_SET.
      {0x80, 0x08},  // Key request S0. Key report single, wrong key. This should return KEX_FAIL_KEY_SET.
      {0x01, 0x83},  // Key request S2 - class 0. Key report multiple. This should return KEX_FAIL_KEY_SET.
      {0x01, 0x02},  // Key request S2 - class 0. Key report single, wrong key. This should return KEX_FAIL_KEY_SET.
      {0x01, 0x03},  // Key request S2 - class 0. Key report multiple. This should return KEX_FAIL_KEY_SET.
      {0x01, 0x80},  // Key request S2 - class 0. Key report single, wrong key. This should return KEX_FAIL_KEY_SET.
      {0x02, 0x83},  // Key request S2 - class 1. Key report multiple. This should return KEX_FAIL_KEY_SET.
      {0x02, 0x01},  // Key request S2 - class 1. Key report single, wrong key. This should return KEX_FAIL_KEY_SET.
      {0x02, 0x03},  // Key request S2 - class 1. Key report multiple. This should return KEX_FAIL_KEY_SET.
      {0x02, 0x80},  // Key request S2 - class 1. Key report single, wrong key. This should return KEX_FAIL_KEY_SET.
    };

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    /** The loop which will set up the expectation based on the round. */
    for (i = 0; i < ELEM_COUNT(key_report_fail_test_vector); i++) {

        /****************************
         * Mock expectation section *
         ****************************/
        s2_inclusion_init(0x02, 0x01, key_report_fail_test_vector[i][0]);

        helper_func_inclusion_initiated_event();

        // Expect a S2 KEX Report to be sent.
        static uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x00};
        S2_kex_report_frame[5]               = key_report_fail_test_vector[i][0];
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
        // If any of the authenticated keys are requested, we expect the primary keypair to be used. Otherwise
        // the dynamic keypair.
        if ((KEY_CLASS_S2_ACCESS | KEY_CLASS_S2_AUTHENTICATED) & key_report_fail_test_vector[i][0]) {
            EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();
        } else {
            EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();
        }

        // When the KEX Set is received, we expect public keys to be exchanges.
        static uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                               PUBLIC_KEY_REPORT,
                                               0x00,  // Including node bit not set
                                               0xAA,
                                               0xBB,
                                               0xCC,
                                               0xDD,
                                               0xEE,
                                               0xFF,
                                               0x11,
                                               0x22,  // Public key as being fetch from keystore.
                                               0xAA,
                                               0xBB,
                                               0xCC,
                                               0xDD,
                                               0xEE,
                                               0xFF,
                                               0x11,
                                               0x22,
                                               0xAA,
                                               0xBB,
                                               0xCC,
                                               0xDD,
                                               0xEE,
                                               0xFF,
                                               0x11,
                                               0x22,
                                               0xAA,
                                               0xBB,
                                               0xCC,
                                               0xDD,
                                               0xEE,
                                               0xFF,
                                               0x11,
                                               0x22};
        // If the node uses only unauthenticated keys, then full public key is expected, else the first two bytes are zero'ed out.
        if ((KEY_CLASS_S2_ACCESS | KEY_CLASS_S2_AUTHENTICATED) & key_report_fail_test_vector[i][0]) {
            S2_pub_key_B_frame[3] = 0x00;
            S2_pub_key_B_frame[4] = 0x00;
        } else {
            S2_pub_key_B_frame[3] = 0xAA;
            S2_pub_key_B_frame[4] = 0xBB;
        }

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // When the public key A is received, we expect that the event from libs2 contains the key in order to present it for the operator.
        // Therefore we copy the key minus header frame into expected data and push an event upwards.
        zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[1];
        p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = key_report_fail_test_vector[i][0];
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
        memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

        // When receiving public key A, then we expect that both our public and private keys are requested from the keystore.
        if ((KEY_CLASS_S2_ACCESS | KEY_CLASS_S2_AUTHENTICATED) & key_report_fail_test_vector[i][0]) {
            EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
        } else {
            EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
        }
        if ((KEY_CLASS_S2_ACCESS | KEY_CLASS_S2_AUTHENTICATED) & key_report_fail_test_vector[i][0]) {
            EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();
        } else {
            EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();
        }

        // When public key is received we expect an echo(KEX Set) to be sent.
        static uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x00};
        S2_echo_kex_set_frame[5]               = key_report_fail_test_vector[i][0];
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // Expect Net Key Get to be sent.
        static uint8_t S2_net_key_get_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x00};
        S2_net_key_get_frame[2]               = key_report_fail_test_vector[i][0];
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_net_key_get_frame, sizeof(S2_net_key_get_frame)), sizeof(S2_net_key_get_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // Expect a S2 KEX Fail as the including node received an Key Set command.
        uint8_t S2_kex_set_error_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_FAIL, 0x0A};  // KEX_FAIL_KEY_REPORT = 0x0A
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_set_error_frame, sizeof(S2_kex_set_error_frame)), sizeof(S2_kex_set_error_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // This event is not expected, until a send frame done (or failed) is received.
        p_expected_event                                                    = (zwave_event_t *)m_test_mem_pool[0];
        p_expected_event->event_type                                        = S2_NODE_INCLUSION_FAILED_EVENT;
        p_expected_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type = 0x0A;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

        /*******************
         * Testing section *
         *******************/

        // Node id (first step in inclusion) has been assigned.
        // Continue with secure inclusion.
        s2_inclusion_joining_start(&s2_context, &s2_con, 0);

        helper_func_kex_get_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        helper_func_kex_set_frame(&s2_context, 0x02, key_report_fail_test_vector[i][0]);
        s2_inclusion_post_event(&s2_context, &s2_con);

        helper_func_pub_key_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

        helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, key_report_fail_test_vector[i][0]);
        s2_inclusion_post_event(&s2_context, &s2_con);

        helper_func_net_key_report_frame(&s2_context, key_report_fail_test_vector[i][1]);
        s2_inclusion_post_event(&s2_context, &s2_con);

        s2_inclusion_send_done(&s2_context, 1);
    }

    // After receiving a timeout it is expected that the inclusion state machine is back in idle and
    // that we can initiate a new inclusion which will also trigger a new set timeout call.
    //  helper_func_verify_idle_state(&s2_context, 0x02, 0x82);
}

/** Test case for ensuring correct handling of node inclusion in all cases when there is a valid combination
 *  of scheme support and key request.
 *
 *  Purpose: In order to ensure a node can be correctly included there must be a match between the
 *           schemes supported by both inclusion and joining node and the request for keys to exchange.
 *
 *  Test: This test case will ensure a KEX Set is transmitted when a valid set of schemes/keys is requested.
 */
TEST_F(InclusionTest, test_kex_inclusion_valid_keys)
{
    uint32_t i;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    uint8_t s2_kex_report_frame[6];
    uint8_t s2_kex_set_frame[6];
    struct S2 s2_context;

    //  uint8_t S2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x00, 0x03, 0x01, 0x84}; // Scheme 0 and 2, curve25519, S2, key 2, and S0 network key.
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being set by upper layer.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t key_request_pass_test_vector[][4] = {
      {0x02, 0x01, 0x02, 0x01},  // Scheme 1 = true, key request S2 = yes, key request S0 = no.  Valid request.
      {0x02, 0x02, 0x02, 0x02},  // Scheme 1 = true, key request S2 = yes, key request S0 = no.  Valid request.
      {0x02, 0x03, 0x02, 0x03},  // Scheme 1 = true, key request S2 = yes, key request S0 = no.  Valid request.
      {0x02, 0x80, 0x02, 0x80},  // Scheme 1 = true, key request S2 = no,  key request S0 = yes.   Valid request.
      {0x02, 0x81, 0x02, 0x81},  // Scheme 1 = true, key request S2 = yes, key request S0 = yes. Valid request.
      {0x02, 0x82, 0x02, 0x82},  // Scheme 1 = true, key request S2 = yes, key request S0 = yes. Valid request.
      {0x02, 0x83, 0x02, 0x83},  // Scheme 1 = true, key request S2 = yes, key request S0 = yes. Valid request.
      // Subset only granted
      {0x02, 0x03, 0x02, 0x01},  // Key request S2 = yes, key request S0 = no.  Granted S2 = yes, S0 = no.  Valid request.
      {0x02, 0x03, 0x02, 0x02},  // Key request S2 = yes, key request S0 = no.  Granted S2 = yes, S0 = no.  Valid request.
      {0x02, 0x81, 0x02, 0x80},  // Key request S2 = yes, key request S0 = yes. Granted S2 = yes, S0 = yes. Valid request.
      {0x02, 0x82, 0x02, 0x80},  // Key request S2 = yes, key request S0 = yes. Granted S2 = no,  S0 = yes. Valid request.
      {0x02, 0x81, 0x02, 0x01},  // Key request S2 = yes, key request S0 = yes. Granted S2 = yes, S0 = no.  Valid request.
      {0x02, 0x82, 0x02, 0x02},  // Key request S2 = yes, key request S0 = yes. Granted S2 = yes, S0 = no.  Valid request.
      {0x02, 0x83, 0x02, 0x81},  // Key request S2 = yes, key request S0 = yes. Granted S2 = yes, S0 = yes. Valid request.
      {0x02, 0x83, 0x02, 0x82},  // Key request S2 = yes, key request S0 = yes. Granted S2 = yes, S0 = yes. Valid request.
      {0x02, 0x83, 0x02, 0x03},  // Key request S2 = yes, key request S0 = yes. Granted S2 = yes, S0 = no.  Valid request.
      {0x02, 0x83, 0x02, 0x80},  // Key request S2 = yes, key request S0 = yes. Granted S2 = no,  S0 = yes. Valid request.
      {0x02, 0x83, 0x02, 0x01},  // Key request S2 = yes, key request S0 = yes. Granted S2 = yes, S0 = no.  Valid request.
      {0x02, 0x83, 0x02, 0x02},  // Key request S2 = yes, key request S0 = yes. Granted S2 = yes, S0 = no.  Valid request.
    };

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("s2_event_handler");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    for (i = 0; i < ELEM_COUNT(key_request_pass_test_vector); i++) {

        /****************************
         * Mock expectation section *
         ****************************/
        s2_inclusion_init(key_request_pass_test_vector[i][0], 0x01, key_request_pass_test_vector[i][1]);

        // After including node has sent a KEX Get, a response with above settings is expected.
        s2_kex_report_frame[0]   = COMMAND_CLASS_SECURITY_2;
        s2_kex_report_frame[1]   = KEX_REPORT;
        s2_kex_report_frame[2]   = 0x04,                                // reserved | NLS support | Request CSA | Echo
          s2_kex_report_frame[3] = key_request_pass_test_vector[i][0];  // Supported schemes.
        s2_kex_report_frame[4]   = 0x01,                                // Supported ECDH Profiles, bit0=1 is curve 25519 value.
          s2_kex_report_frame[5] = key_request_pass_test_vector[i][1];  // Requested keys bit.
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_kex_report_frame, sizeof(s2_kex_report_frame)), sizeof(s2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        // Before exchange of public keys, then we expect that our public key is requested from the keystore.
        // If any of the authenticated keys are requested, we expect the primary keypair to be used. Otherwise
        // the dynamic keypair.
        if ((KEY_CLASS_S2_ACCESS | KEY_CLASS_S2_AUTHENTICATED) & key_request_pass_test_vector[i][3]) {
            EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();
        } else {
            EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();
        }

        uint8_t s2_public_key_frame[3 + sizeof(public_key_b)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x00};  // Key exchange received from slave - public key for secure exchange of LTK.
        memcpy(&s2_public_key_frame[3], public_key_b, sizeof(public_key_b));
        // If the node uses only unauthenticated keys, then full public key is expected, else the first two bytes are zero'ed out.
        if ((KEY_CLASS_S2_ACCESS | KEY_CLASS_S2_AUTHENTICATED) & key_request_pass_test_vector[i][3]) {
            s2_public_key_frame[3] = 0x00;
            s2_public_key_frame[4] = 0x00;
        }

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_public_key_frame, sizeof(s2_public_key_frame)), sizeof(s2_public_key_frame))).WillOnce(Return(0)).RetiresOnSaturation();

        /*******************
         * Testing section *
         *******************/

        // Node id (first step in inclusion) has been assigned.
        // Continue with secure inclusion.
        s2_inclusion_joining_start(&s2_context, &s2_con, 0);

        // KEX Get frame received.
        helper_func_kex_get_frame(&s2_context);
        s2_inclusion_post_event(&s2_context, &s2_con);

        // KEX Set frame received - With an unsupported/not requested scheme set.
        s2_kex_set_frame[0]   = COMMAND_CLASS_SECURITY_2;
        s2_kex_set_frame[1]   = KEX_SET;
        s2_kex_set_frame[2]   = 0x00,                                // bit 0 is echo field, rest are reserved
          s2_kex_set_frame[3] = key_request_pass_test_vector[i][2];  // Supported schemes.
        s2_kex_set_frame[4]   = 0x01,                                // Supported ECDH Profiles, bit0=1 is curve 25519 value.
          s2_kex_set_frame[5] = key_request_pass_test_vector[i][3];  // Granted keys bit.

        s2_context.buf    = s2_kex_set_frame;
        s2_context.length = sizeof(s2_kex_set_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        helper_func_idle_state(&s2_context);
    }
}

/** Test case for ensuring correct timer handling on timeout while in idle.
 */
TEST_F(InclusionTest, test_kex_inclusion_timeout_when_idle)
{
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("keystore_network_key_clear");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);
    s2_inclusion_init(0x02, 0x01, 0x01);

    // No timeout event/node inclusion failure is expected as we are already in idle.

    // When receiving the KEX Get an inclusion initiated event is expected.
    helper_func_inclusion_initiated_event();

    // After timeout we will issue a node inclusion just to insure we were in idle and that a node
    // may be included after timeout.
    uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x01};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

    /*******************
     * Testing section *
     *******************/

    // Trigger a timeout.
    s2_inclusion_notify_timeout(&s2_context);

    // Node id (first step in inclusion) has been assigned.
    // Let's initiate a secure inclusion after the timeout to verify the state of the system.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    // KEX Get frame received.
    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);
}

/** Test case for ensuring correct handling when an invalid frame containing an internal event code is received.
 *  Example: Receiving a frame containg 0x81 would not be valid (indicates JOINING_START internally)
 *
 *  Such frame should be silently ignored.
 */
TEST_F(InclusionTest, test_kex_inclusion_invalid_frame_idle)
{
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;

    // Invalid joining start frame.
    uint8_t s2_invalid_joining_frame[] = {COMMAND_CLASS_SECURITY_2, 0x02, 0x01, 0x00, 0x00, 0x00};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);
    s2_inclusion_init(0x02, 0x01, 0x82);

    memcpy((uint8_t *)s2_context.buf, s2_invalid_joining_frame, sizeof(s2_invalid_joining_frame));
    s2_context.length = sizeof(s2_invalid_joining_frame);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Let's try to see if we can get the system to join us (should not be possible).
    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);
}

/** This test case ensure that the system continues to send ECHO KEX Set until a ECHO KEX Report is received regardless if send done fails or succeeds.
 *  When ECHO KEX Report is received the state machine continues.
 */
TEST_F(InclusionTest, test_kex_inclusion_echo_kex_set_retry)
{
    //  zwave_event_t  * p_expected_event;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    static uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                                     0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t s2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x81};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_write");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    /****************************
     * Mock expectation section *
     ****************************/
    s2_inclusion_init(0x02, 0x01, 0x81);

    helper_func_inclusion_initiated_event();

    // Expect a S2 KEX Report to be sent.
    static uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x81};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

    // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When the KEX Set is received, we expect public keys to be exchanges.
    static uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                           PUBLIC_KEY_REPORT,
                                           0x00,  // Including node bit not set
                                           0xAA,
                                           0xBB,
                                           0xCC,
                                           0xDD,
                                           0xEE,
                                           0xFF,
                                           0x11,
                                           0x22,  // Public key as being fetch from keystore.
                                           0xAA,
                                           0xBB,
                                           0xCC,
                                           0xDD,
                                           0xEE,
                                           0xFF,
                                           0x11,
                                           0x22,
                                           0xAA,
                                           0xBB,
                                           0xCC,
                                           0xDD,
                                           0xEE,
                                           0xFF,
                                           0x11,
                                           0x22,
                                           0xAA,
                                           0xBB,
                                           0xCC,
                                           0xDD,
                                           0xEE,
                                           0xFF,
                                           0x11,
                                           0x22};

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(0)).RetiresOnSaturation();

    // When the public key A is received, we expect that the event from libs2 contains the key in order to present it for the operator.
    // Therefore we copy the key minus header frame into expected data and push an event upwards.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x81;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
    memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    // When receiving public key A, then we expect that both our public and private keys are requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // Catch-all must be declared first so later BufferEq expects take precedence (GMock newest-first).
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), _, _)).WillOnce(Return(0)).RetiresOnSaturation();

    // When public key is received we expect an echo(KEX Set) to be sent (and retried until ECHO KEX Report).
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_echo_kex_set_frame, sizeof(s2_echo_kex_set_frame)), sizeof(s2_echo_kex_set_frame))).WillOnce(Return(0)).WillOnce(Return(0)).WillOnce(Return(0)).RetiresOnSaturation();

    // Expect Net Key Get to be sent.
    static uint8_t S2_net_key_get_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x01};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_net_key_get_frame, sizeof(S2_net_key_get_frame)), sizeof(S2_net_key_get_frame))).WillOnce(Return(0)).RetiresOnSaturation();
    //  p_mock->expect_arg[2].pointer = s2_echo_kex_set_frame; // Ideally, this should be updated to be identically to replayed received KEX Set.
    //  p_mock->expect_arg[3].value   = sizeof(s2_echo_kex_set_frame);

    /*******************
     * Testing section *
     *******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_kex_set_frame(&s2_context, 0x02, 0x81);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_pub_key_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    // Succesfull transmission, however, retries are expected until ECHO KEX Report is received (or 'n' retries due to timeouts).
    s2_inclusion_send_done(&s2_context, 1);
    s2_inclusion_notify_timeout(&s2_context);

    // Failed transmission, however, retries are expected until ECHO KEX Report is received (or 'n' retries due to timeouts).
    s2_inclusion_send_done(&s2_context, 0);
    s2_inclusion_notify_timeout(&s2_context);

    // Succesfull transmission, however, retries are expected until ECHO KEX Report is received (or 'n' retries due to timeouts).
    s2_inclusion_send_done(&s2_context, 1);

    // This time the echo report is received before the timeout.
    helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x81);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_net_key_report_frame(&s2_context, 0x01);
    s2_inclusion_post_event(&s2_context, &s2_con);

    // After receiving a timeout it is expected that the inclusion state machine is back in idle and
    // that we can initiate a new inclusion which will also trigger a new set timeout call.
    //  helper_func_verify_idle_state(&s2_context, 0x02, 0x82);
}

/** This test case verifies the handling of the keystore during inclusion.
 *
 *  Expected behavior:
 *  - Clearing of all keys when entering learn mode.
 *  - Temporary key is calculated and updated in s2 context.
 *  - Writing of network keys to keystore when receiving them.
 *  - Restore of all keys when inclusion is complete.
 */
TEST_F(InclusionTest, test_inclusion_keystore_handling)
{
    //  zwave_event_t  * p_expected_event;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    static uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                                     0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t shared_secret[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x0F, 0xED, 0xCB, 0xA9, 0x87, 0x65, 0x43, 0x21, 0x10, 0x29, 0x38, 0x47, 0x56, 0x65, 0x74, 0x83, 0x38, 0x63, 0x97, 0x47, 0x82, 0x12, 0x53, 0xc4};

    uint8_t expected_auth_tag[64];

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_send_frame");
    libs2_stub("S2_send_data");
    libs2_stub("s2_event_handler");
    //  libs2_stub("S2_network_key_update");
    //  libs2_stub("crypto_scalarmult_curve25519");
    //  libs2_stub("tempkey_extract");
    //  libs2_stub("keystore_network_key_clear");
    //  libs2_stub("keystore_network_key_write");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    /****************************
     * Mock expectation section *
     ****************************/
    s2_inclusion_init(0x02, 0x01, 0x87);

    // When entering joining start, we expect all keys to be cleared.
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_clear(_)).WillOnce(Return(true)).RetiresOnSaturation();

    // When exchanging public keys, we expect a call to keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // After receiving public key A, we expect calls to keystore and crypto lib to extract temporary key.
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, crypto_scalarmult_curve25519(_, _, _)).WillOnce(SetArrayArgument<0>(shared_secret, shared_secret + 32)).RetiresOnSaturation();

    memcpy(expected_auth_tag, m_test_public_key_a, 32);
    memcpy(&expected_auth_tag[32], public_key_b, 32);

    EXPECT_CALL(*g_libs2_mocks, tempkey_extract(_, _, _)).WillOnce(SetArrayArgument<2>(m_temp_key, m_temp_key + 32)).RetiresOnSaturation();

    // S2 protocol should be updated with the temporary key.
    EXPECT_CALL(*g_libs2_mocks, S2_network_key_update(_, _, _, _, _)).WillOnce(Return(1)).RetiresOnSaturation();

    // When the network key is received we expect a write to the keystore and an update to the protocol, Security 2 / Class 0 key.
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_write(_, _)).WillOnce(Return(true)).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, S2_network_key_update(_, _, _, _, _)).WillOnce(Return(1)).RetiresOnSaturation();

    // When the network key is received we expect a write to the keystore and an update to the protocol, Security 0 Network key.
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_write(_, _)).WillOnce(Return(true)).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, S2_network_key_update(_, _, _, _, _)).WillOnce(Return(1)).RetiresOnSaturation();

    // After network key exchange has completed, we expect the node to fetch all S2 keys available from the keystore and update the S2 context.
    // 0x80 is an S0 key and should not be passed into S2_network_key_update.
    // Keys granted and stored: 0x81, Keys rejected and thus unavailable: 0x06
    // Upon restore, all keys will be fetched from keystore, but only 0x81 are available. And only 0x01 is an S2 key.
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(0x01, _)).WillOnce(DoAll(SetArrayArgument<1>(m_test_network_key_s2_class_0, m_test_network_key_s2_class_0 + 16), Return(true))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, S2_network_key_update(_, _, _, _, _)).WillOnce(Return(1)).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(0x02, _)).WillOnce(DoAll(SetArrayArgument<1>(m_test_mem_pool[0], m_test_mem_pool[0] + 16), Return(false))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(0x04, _)).WillOnce(DoAll(SetArrayArgument<1>(m_test_mem_pool[1], m_test_mem_pool[1] + 16), Return(false))).RetiresOnSaturation();

#ifdef ZW_CONTROLLER
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(0x08, _)).WillOnce(DoAll(SetArrayArgument<1>(m_test_mem_pool[0], m_test_mem_pool[0] + 16), Return(false))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(0x10, _)).WillOnce(DoAll(SetArrayArgument<1>(m_test_mem_pool[1], m_test_mem_pool[1] + 16), Return(false))).RetiresOnSaturation();
#endif

    /*******************
     * Testing section *
     *******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_kex_set_frame(&s2_context, 0x02, 0x81);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_pub_key_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    // Transmision succeeds.
    s2_inclusion_send_done(&s2_context, 1);

    helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x87);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_net_key_report_frame(&s2_context, 0x01);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_transfer_end_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_net_key_report_frame(&s2_context, 0x80);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_transfer_end_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Transmision succeeds.
    s2_inclusion_send_done(&s2_context, 1);
}

/** Test case for ensuring that a received KEX Fail from peer is being pushed upwards.
 *
 *  Test: This test case will a KEX_FAIL_KEY is returned when invalid set of schemes/keys is requested.
 */
TEST_F(InclusionTest, test_kex_inclusion_received_kex_fail_frame)
{
    zwave_event_t *p_expected_event;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;

    uint8_t s2_kex_report_frame[6] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x84};
    uint8_t s2_kex_error_frame[]   = {COMMAND_CLASS_SECURITY_2, KEX_FAIL, 0x01};  // KEX_FAIL_KEX_KEY = 0x01

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    /****************************
     * Mock expectation section *
     ****************************/
    s2_inclusion_init(0x02, 0x01, 0x84);

    helper_func_inclusion_initiated_event();

    // After including node has sent a KEX Get, a response is expected.
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_kex_report_frame, sizeof(s2_kex_report_frame)), sizeof(s2_kex_report_frame))).WillOnce(Return(0)).RetiresOnSaturation();

    p_expected_event                                                    = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_event->event_type                                        = S2_NODE_INCLUSION_FAILED_EVENT;
    p_expected_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type = 0x01;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

    /*******************
     * Testing section *
     *******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    // KEX Get frame received.
    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_context.buf    = s2_kex_error_frame;
    s2_context.length = sizeof(s2_kex_error_frame);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Retry node inclusion.
    helper_func_verify_idle_state(&s2_context, 0x02, 0x84);
}

/** Test case for ensuring that secure learn mode can be aborted.
 *  This feature is requested on nodes that might support both Security 0 and security 2, but where
 *  it is unknown which inclusion mode will be activated.
 */
TEST_F(InclusionTest, test_kex_inclusion_abort)
{
    struct S2 s2_context;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    /****************************
     * Mock expectation section *
     ****************************/
    s2_inclusion_init(0x02, 0x01, 0x84);

    /*******************
     * Testing section *
     *******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    // Security 0 learn mode was initiated, abort current inclusion.
    s2_inclusion_abort(&s2_context);

    // Retry node inclusion.
    helper_func_verify_idle_state(&s2_context, 0x02, 0x84);
}

/** During inclusion of a node that requests authenticated key classes, the user is expected to
 *  input the missing digits of the DSK.
 *
 *  If wrong digits are typed, then the frame is encrypted using wrong DSK and hence it cannot be decrypted.
 *  In that case, it is expected that the S2 protocol layer calls s2_inclusion_decrypt_failure(...).
 *
 *  This failure should result in a KEX Fail - TBD to be transmitted to the inclusion_peer.
 */
TEST_F(InclusionTest, test_kex_joining_node_wrong_dsk_input)
{
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_read");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_write");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    /**
     *  Test expectation setup.
     */

    // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
    helper_func_inclusion_initiated_event();

    // Expect a S2 KEX Get to be sent.
    helper_func_kex_report_frame_expect(0x02, 0x01, 0x82);

    // Before exchange of public keys, then we expect that our public key is requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    uint8_t s2_public_key_frame[3 + sizeof(public_key_b)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x00, 0x00, 0x00};  // Key exchange received from slave - public key for secure exchange of LTK. Note the first two bytes should not be transmitted on authenticated/access keys.
    memcpy(&s2_public_key_frame[5], &public_key_b[2], sizeof(public_key_b) - 2);
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_public_key_frame, sizeof(s2_public_key_frame)), sizeof(s2_public_key_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving public key A, then we expect an event to be pushed up and both our public and private keys are requested from the keystore.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
    memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // Expect Echo(KEX Report) to be sent.
    uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};  // Note: Echo flag set.
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(0)).RetiresOnSaturation();

    // When a KEX Fail decrypt is received, then inclusion should be aborted.
    zwave_event_t *p_expected_inc_fail_event                                     = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_inc_fail_event->event_type                                        = S2_NODE_INCLUSION_FAILED_EVENT;
    p_expected_inc_fail_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type = 0x05;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_fail_event))).RetiresOnSaturation();

    /**
     *  Test execution.
     */
    struct S2 s2_context;

    s2_context.inclusion_state = S2_INC_IDLE;
    // memcpy(s2_context.temp_network_key, m_temp_key, sizeof(s2_context.temp_network_key));

    s2_inclusion_init(0x02, 0x01, 0x82);
    s2_inclusion_set_event_handler(s2_event_handler);

    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    // KEX Get frame received.
    uint8_t S2_kex_get_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
    s2_context.buf             = S2_kex_get_frame;
    s2_context.length          = sizeof(S2_kex_get_frame);
    s2_con.class_id            = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // KEX report is expeted to be transmitted. Let's make the s2_send_frame a success.
    s2_inclusion_send_done(&s2_context, 1);

    // KEX Set frame received.
    // bit0: echo field not set, bit1-7: Reserved.
    // Selected schemes: scheme 0 and scheme 2.
    // Selected curve25519
    // Keys to exchange, Security2, class 2 - Security0, network key.
    uint8_t s2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x00, 0x02, 0x01, 0x82};
    s2_context.buf             = s2_kex_set_frame;
    s2_context.length          = sizeof(s2_kex_set_frame);
    s2_con.class_id            = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    uint8_t public_key_frame[3 + sizeof(m_test_public_key_a)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x01};  // Key exchange received from slave - public key for secure exchange of LTK.
    memcpy(&public_key_frame[3], m_test_public_key_a, sizeof(m_test_public_key_a));
    s2_context.buf    = public_key_frame;
    s2_context.length = sizeof(public_key_frame);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    // If wrong DSK is provided by end-user, then it is expected that a KEX Fail event is received.
    uint8_t s2_kex_fail_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_FAIL, 0x05};
    s2_context.buf              = s2_kex_fail_frame;
    s2_context.length           = sizeof(s2_kex_fail_frame);
    s2_con.class_id             = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_verify_idle_state(&s2_context, 0x02, 0x82);
}

/** This test case verifies the handling of the keystore during inclusion.
 * Keys like a DoorLockKeypad is requested and granted
 *
 *
 *  Expected behavior:
 *  - Clearing of all keys when entering learn mode.
 *  - Temporary key is calculated and updated in s2 context.
 *  - Writing of network keys to keystore when receiving them.
 *  - Restore of all keys when inclusion is complete.
 */
TEST_F(InclusionTest, test_keystore_doorlock)
{
    //  zwave_event_t  * p_expected_event;
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    static uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                                     0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t shared_secret[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x0F, 0xED, 0xCB, 0xA9, 0x87, 0x65, 0x43, 0x21, 0x10, 0x29, 0x38, 0x47, 0x56, 0x65, 0x74, 0x83, 0x38, 0x63, 0x97, 0x47, 0x82, 0x12, 0x53, 0xc4};

    uint8_t expected_auth_tag[64];

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_send_frame");
    libs2_stub("S2_send_data");
    libs2_stub("s2_event_handler");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);

    /****************************
     * Mock expectation section *
     ****************************/
    s2_inclusion_init(0x02, 0x01, 0x84);

    // When entering joining start, we expect all keys to be cleared.
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_clear(_)).WillOnce(Return(true)).RetiresOnSaturation();

    // When exchanging public keys, we expect a call to keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // After receiving public key A, we expect calls to keystore and crypto lib to extract temporary key.
    EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, crypto_scalarmult_curve25519(_, _, _)).WillOnce(SetArrayArgument<0>(shared_secret, shared_secret + 32)).RetiresOnSaturation();

    memcpy(expected_auth_tag, m_test_public_key_a, 32);
    memcpy(&expected_auth_tag[32], public_key_b, 32);

    EXPECT_CALL(*g_libs2_mocks, tempkey_extract(_, _, _)).WillOnce(SetArrayArgument<2>(m_temp_key, m_temp_key + 32)).RetiresOnSaturation();

    // S2 protocol should be updated with the temporary key.
    EXPECT_CALL(*g_libs2_mocks, S2_network_key_update(_, _, _, _, _)).WillOnce(Return(1)).RetiresOnSaturation();

    // When the network key is received we expect a write to the keystore and an update to the protocol, Security 2 / Class 0 key.
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_write(_, _)).WillOnce(Return(true)).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, S2_network_key_update(_, _, _, _, _)).WillOnce(Return(1)).RetiresOnSaturation();

    // When the network key is received we expect a write to the keystore and an update to the protocol, Security 0 Network key.
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_write(_, _)).WillOnce(Return(true)).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, S2_network_key_update(_, _, _, _, _)).WillOnce(Return(1)).RetiresOnSaturation();

    // After network key exchange has completed, we expect the node to fetch all keys available from the keystore and update the S2 context.
    // 0x80 is an S0 key and should not be passed into S2_network_key_update.
    // Keys granted and stored: 0x81, Keys rejected and thus unavailable: 0x06
    // Upon restore, all keys will be fetched from keystore, but only 0x81 are available. And only 0x01 is an S2 key.
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(0x01, _)).WillOnce(DoAll(SetArrayArgument<1>(m_test_mem_pool[0], m_test_mem_pool[0] + 16), Return(false))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(0x02, _)).WillOnce(DoAll(SetArrayArgument<1>(m_test_mem_pool[0], m_test_mem_pool[0] + 16), Return(false))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(0x04, _)).WillOnce(DoAll(SetArrayArgument<1>(m_test_network_key_s2_class_2, m_test_network_key_s2_class_2 + 16), Return(true))).RetiresOnSaturation();

#ifdef ZW_CONTROLLER
    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(0x08, _)).WillOnce(DoAll(SetArrayArgument<1>(m_test_mem_pool[0], m_test_mem_pool[0] + 16), Return(false))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_network_key_read(0x10, _)).WillOnce(DoAll(SetArrayArgument<1>(m_test_mem_pool[0], m_test_mem_pool[0] + 16), Return(false))).RetiresOnSaturation();
#endif

    EXPECT_CALL(*g_libs2_mocks, S2_network_key_update(_, _, _, _, _)).WillOnce(Return(1)).RetiresOnSaturation();

    /*******************
     * Testing section *
     *******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_kex_set_frame(&s2_context, 0x02, 0x84);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_pub_key_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    // Transmision succeeds.
    s2_inclusion_send_done(&s2_context, 1);

    helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x84);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_net_key_report_frame(&s2_context, 0x04);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_transfer_end_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_net_key_report_frame(&s2_context, 0x80);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_transfer_end_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Transmision succeeds.
    s2_inclusion_send_done(&s2_context, 1);
}

/** Verification that in case the inclusion state machine is in idle, then AUTH Failed should not
 *  be returned for an invalid encrypted frame. TO#06510
 */
TEST_F(InclusionTest, test_kex_invalid_crypt_frame_idle)
{

    libs2_clear_stubs();

    /**
     *  Test expectation setup.
     */

    /**
     *  Test execution.
     */
    struct S2 s2_context;

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_inclusion_set_event_handler(s2_event_handler);

    // S2 Transfer end frame received.
    // bit0: Key request complete set,
    // bit1: Key verified not set,
    // bit2-7: Reserved.
    uint8_t s2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x02};
    s2_context.buf                  = s2_transfer_end_frame;
    s2_context.length               = sizeof(s2_transfer_end_frame);
    s2_con.class_id                 = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);
}

/** Test case for ensuring that the inclusion state machine will not abort if an older inclusion
 *  frame is received multiple times, as example due to retransmission or routing.
 */
TEST_F(InclusionTest, test_kex_inclusion_tx_queue_full)
{
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_write");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);
    s2_inclusion_init(0x02, 0x01, 0x82);

    /******************************************************************************
     * Mock expectation section                                                   *
     * - Even though frames are received multiple times then the duplicate frames *
     *   are expected to be ignored and hence the wit is only expected that each  *
     *   frame is transmitted once from the joining node.                         *
     ******************************************************************************/
    helper_func_inclusion_initiated_event();

    // Expect a S2 KEX Report to be sent. Calls fails as TX is full.
    uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x82};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(0)).WillOnce(Return(1)).RetiresOnSaturation();

    // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When the KEX Set is received, we expect public keys to be exchanges.
    uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                    PUBLIC_KEY_REPORT,
                                    0x00,  // Including node bit not set
                                    0x00,
                                    0x00,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,  // Public key as being fetch from keystore.
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22};

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(0)).WillOnce(Return(1)).RetiresOnSaturation();

    // When the public key A is received, we expect that the event from libs2 contains the key in order to present it for the operator.
    // Therefore we copy the key minus header frame into expected data and push an event upwards.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
    memcpy((uint8_t *)p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    // When receiving public key A, then we expect that both our public and private keys are requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When public key is received we expect an echo(KEX Set) to be sent.
    uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};
    S2_echo_kex_set_frame[0]        = COMMAND_CLASS_SECURITY_2;
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(0)).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect Net Key Get to be sent.
    uint8_t S2_net_key_get_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_net_key_get_frame, sizeof(S2_net_key_get_frame)), sizeof(S2_net_key_get_frame))).WillOnce(Return(0)).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect Net Key Verify to be sent (same frame for both key classes).
    uint8_t S2_network_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_network_key_verify_frame, sizeof(S2_network_key_verify_frame)), sizeof(S2_network_key_verify_frame))).WillOnce(Return(0)).WillOnce(Return(1)).WillOnce(Return(0)).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_get_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x80};  // Keys requested, Security2, class 2 - Security0, network key.
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_0_frame, sizeof(s2_net_key_get_0_frame)), sizeof(s2_net_key_get_0_frame))).WillOnce(Return(0)).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect S2 Transfer End to be sent.
    uint8_t S2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x01};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_transfer_end_frame, sizeof(S2_transfer_end_frame)), sizeof(S2_transfer_end_frame))).WillOnce(Return(0)).WillOnce(Return(1)).RetiresOnSaturation();

    // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
    zwave_event_t *p_expected_complete_event                                          = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_complete_event->event_type                                             = S2_NODE_JOINING_COMPLETE_EVENT;
    p_expected_complete_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0x82;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_complete_event))).RetiresOnSaturation();

    /******************
     * Start the test *
     ******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, 1);

    helper_func_kex_set_frame(&s2_context, 0x02, 0x82);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, 1);

    helper_func_pub_key_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    // After the received public key is pushed upwards in the system, then it is expected to receive a challenge response from the upper level.
    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    // Echo KEX Set is a special case, dur to the ECDH calculation.
    // Thus the normal send done / fail will not trigger a re-transmit, instead the timeout will.
    s2_inclusion_send_done(&s2_context, 1);
    s2_inclusion_notify_timeout(&s2_context);

    helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x82);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, 1);

    // First round network key exchange.
    helper_func_net_key_report_frame(&s2_context, 0x02);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, 1);

    helper_func_transfer_end_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, 1);

    // Second round network key exchange.
    helper_func_net_key_report_frame(&s2_context, 0x80);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, 1);

    helper_func_transfer_end_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);
    // Network key exchange completed.

    s2_inclusion_send_done(&s2_context, 1);
    s2_inclusion_send_done(&s2_context, true);
}

/** Test case for ensuring that in the (including node misbehaving) event where a
 *  SECURITY_2_TRANSFER_END is returned but KeyVerified bit is set to 0.
 *  TO#7507
 */
TEST_F(InclusionTest, test_kex_inclusion_transfer_end_key_not_verified_TO7507)
{
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_write");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);
    s2_inclusion_init(0x02, 0x01, 0x82);

    /******************************************************************************
     * Mock expectation section                                                   *
     * - Even though frames are received multiple times then the duplicate frames *
     *   are expected to be ignored and hence the wit is only expected that each  *
     *   frame is transmitted once from the joining node.                         *
     ******************************************************************************/
    helper_func_inclusion_initiated_event();

    // Expect a S2 KEX Report to be sent.
    uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x82};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When the KEX Set is received, we expect public keys to be exchanges.
    uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                    PUBLIC_KEY_REPORT,
                                    0x00,  // Including node bit not set
                                    0x00,
                                    0x00,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,  // Public key as being fetch from keystore.
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22};

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When the public key A is received, we expect that the event from libs2 contains the key in order to present it for the operator.
    // Therefore we copy the key minus header frame into expected data and push an event upwards.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
    memcpy((uint8_t *)p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    // When receiving public key A, then we expect that both our public and private keys are requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When public key is received we expect an echo(KEX Set) to be sent.
    uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};
    S2_echo_kex_set_frame[0]        = COMMAND_CLASS_SECURITY_2;
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect Net Key Get to be sent.
    uint8_t S2_net_key_get_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_net_key_get_frame, sizeof(S2_net_key_get_frame)), sizeof(S2_net_key_get_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect Net Key Verify to be sent.
    uint8_t S2_network_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_network_key_verify_frame, sizeof(S2_network_key_verify_frame)), sizeof(S2_network_key_verify_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect KEX FAIL to be sent.
    uint8_t S2_network_kex_fail_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_FAIL, KEX_FAIL_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_network_kex_fail_frame, sizeof(S2_network_kex_fail_frame)), sizeof(S2_network_kex_fail_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
    zwave_event_t *p_expected_inc_fail_event                                     = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_inc_fail_event->event_type                                        = S2_NODE_INCLUSION_FAILED_EVENT;
    p_expected_inc_fail_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type = KEX_FAIL_KEY_VERIFY;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_fail_event))).RetiresOnSaturation();

    /******************
     * Start the test *
     ******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    /** The loop which will set up the expectation based on the round. */
    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_kex_set_frame(&s2_context, 0x02, 0x82);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_pub_key_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x82);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_net_key_report_frame(&s2_context, 0x02);
    s2_inclusion_post_event(&s2_context, &s2_con);

    // First round network key exchange. The verify bit is not set so the joining node should
    // silently abort (push error internally).
    frame_buffer[0]   = COMMAND_CLASS_SECURITY_2;
    frame_buffer[1]   = SECURITY_2_TRANSFER_END;
    frame_buffer[2]   = 0x00;  // Verify bit not set. Normally this packet will look as 0x02.
    s2_context.length = 3;
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);
}

/** Test case for ensuring that in the (including node misbehaving) event where a
 *  SECURITY_2_TRANSFER_END is returned by including node with KeyRequestComplete bit is set to 1.
 */
TEST_F(InclusionTest, test_kex_inclusion_transfer_end_key_request_complete_error)
{
    uint8_t frame_buffer[UNIT_TEST_BUF_SIZE];
    struct S2 s2_context;
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key as being fetch from the keystore.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_write");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_read");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    memset(&s2_context, 0, sizeof(s2_context));
    s2_context.inclusion_state = S2_INC_IDLE;
    s2_context.buf             = frame_buffer;
    s2_inclusion_set_event_handler(s2_event_handler);
    s2_inclusion_init(0x02, 0x01, 0x82);

    /******************************************************************************
     * Mock expectation section                                                   *
     * - Even though frames are received multiple times then the duplicate frames *
     *   are expected to be ignored and hence the wit is only expected that each  *
     *   frame is transmitted once from the joining node.                         *
     ******************************************************************************/
    helper_func_inclusion_initiated_event();

    // Expect a S2 KEX Report to be sent.
    uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x82};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When the KEX Set is received, we expect a call to the keystore in order to obtain our public key.
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When the KEX Set is received, we expect public keys to be exchanges.
    uint8_t S2_pub_key_B_frame[] = {COMMAND_CLASS_SECURITY_2,
                                    PUBLIC_KEY_REPORT,
                                    0x00,  // Including node bit not set
                                    0x00,
                                    0x00,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,  // Public key as being fetch from keystore.
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22,
                                    0xAA,
                                    0xBB,
                                    0xCC,
                                    0xDD,
                                    0xEE,
                                    0xFF,
                                    0x11,
                                    0x22};

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_pub_key_B_frame, sizeof(S2_pub_key_B_frame)), sizeof(S2_pub_key_B_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When the public key A is received, we expect that the event from libs2 contains the key in order to present it for the operator.
    // Therefore we copy the key minus header frame into expected data and push an event upwards.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
    memcpy((uint8_t *)p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    // When receiving public key A, then we expect that both our public and private keys are requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // When public key is received we expect an echo(KEX Set) to be sent.
    uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};
    S2_echo_kex_set_frame[0]        = COMMAND_CLASS_SECURITY_2;
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect Net Key Get to be sent.
    uint8_t S2_net_key_get_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_net_key_get_frame, sizeof(S2_net_key_get_frame)), sizeof(S2_net_key_get_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect Net Key Verify to be sent.
    uint8_t S2_network_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_network_key_verify_frame, sizeof(S2_network_key_verify_frame)), sizeof(S2_network_key_verify_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect KEX FAIL to be sent.
    uint8_t S2_network_kex_fail_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_FAIL, KEX_FAIL_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_network_kex_fail_frame, sizeof(S2_network_kex_fail_frame)), sizeof(S2_network_kex_fail_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
    zwave_event_t *p_expected_inc_fail_event                                     = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_inc_fail_event->event_type                                        = S2_NODE_INCLUSION_FAILED_EVENT;
    p_expected_inc_fail_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type = KEX_FAIL_KEY_VERIFY;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_fail_event))).RetiresOnSaturation();

    /******************
     * Start the test *
     ******************/

    // Node id (first step in inclusion) has been assigned.
    // Continue with secure inclusion.
    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    /** The loop which will set up the expectation based on the round. */
    helper_func_kex_get_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_kex_set_frame(&s2_context, 0x02, 0x82);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_pub_key_frame(&s2_context);
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    helper_func_echo_kex_report_frame(&s2_context, 0x02, 0x01, 0x82);
    s2_inclusion_post_event(&s2_context, &s2_con);

    helper_func_net_key_report_frame(&s2_context, 0x02);
    s2_inclusion_post_event(&s2_context, &s2_con);

    // First round network key exchange. The verify bit is not set so the joining node should
    // silently abort (push error internally).
    frame_buffer[0]   = COMMAND_CLASS_SECURITY_2;
    frame_buffer[1]   = SECURITY_2_TRANSFER_END;
    frame_buffer[2]   = 0x03;  // Verify bit not set. Normally this packet will look as 0x02.
    s2_context.length = 3;
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);
}

/** Verification that in case operator rejects the use of CSA and restricts the keys to be exchanged to S2 unauthenticated or S0.
 *
 * Expectation: The including node sends its full public key
 *              The joining node sends its full public key
 *              CSA is disabled on the S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT event.
 */
TEST_F(InclusionTest, test_kex_joining_node_state_machine_csa_rejected_S2_unauth_S0_TO7654)
{
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_read");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_write");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    /**
     *  Test expectation setup.
     * This section set up the expectations in the system when the inclusion state machine progresses:
     * 1) Starting in idle it is expected that an S2 frame KEX1 will be transmitted based on external event when a node id has been assigned.
     * 2) After S2 KEX Get has been transmitted, the FSM awaits a S2 KEX Report as response.
     * 3) After S2 KEX Report is received then S2 KEX Set shall be sent.
     * 4) After S2 KEX Set  has been transmitted, the FSM awaits a Public KeyB from joining node.
     * 5) Exchange of public keys.
     *    a) Public KeyB is received from joining node A.
     *       - Public KeyB must be pushed upwards as event to be presented to user for further verification.
     *         If node is rejected, then node is part of network insecurely. Should this be changed ? Maybe force a node exclude.
     *         Currently that use case is outside the scope of libs2.
     *       - When Public KeyB is received, the controller must send its public KeyA.
     *         Public KeyB shall be provided by the application.
     *       - Using Pub KeyB and Private KeyA then a temporary key, KeyT is derived from the x coordinate of the shared secret.
     *         This is done in security sub module (currently under development)
     *       - The security context must be re-initialized with temporary derived key.
     *    b) Public KeyA is transmitted from including node A, the state machine will await 'Echo (KEX Set)'.
     * 6) Echoing of KEX frames.
     *    a) A 'Echo(KEX Set)' is expected to be received from the joining node, and it must be
     *       verified that 'Echo(KEX Set)' is identical to the KEX Set sent at step 3).
     *    b) 'Echo(KEX Report)' with content identical to packet received in 3) shall be replied to the joining node.
     * x) Nonce exchange: When joining node wants to send 'Echo(KEX Set)' the S2 protocol layer will exchange a nonce prior to communication.
     * 7) Network key exchange:
     *    a) Joining node will request the network key, by sending a 'Network Key Get'.
     *    b) Including node shall reply with a 'Network Key Set'.
     *    c) Joining node shall confirm with a 'Network Key Verify' which is encrypted using the the key obtained by 'NetworkKey-expand'
     *       - Including node must verify that it can succesfully decrypt the message using the key.
     *    d) Including node shall reply with a 'Transfer End'.
     *    e) Joining node shall reply with a 'Transfer End' if no more keys shall be exchanged.
     *
     */

    // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
    zwave_event_t *p_expected_inc_init_event = (zwave_event_t *)m_test_mem_pool[2];
    p_expected_inc_init_event->event_type    = S2_NODE_INCLUSION_INITIATED_EVENT;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_init_event))).RetiresOnSaturation();

    // Expect a S2 KEX Get to be sent.
    uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2,
                                     KEX_REPORT,
                                     0x06,   // bit 0 is echo field, bit 1 is CSA, rest are reserved
                                     0x02,   // Supported schemes. Scheme 0 and 2.
                                     0x01,   // Supported ECDH Profiles, bit0=1 is curve 25519 value.
                                     0x87};  // Requested keys bits. Security 2 class 0, 1, 2, Security 0 network key.

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Before exchange of public keys, then we expect that our public key is requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    uint8_t s2_public_key_frame[3 + sizeof(public_key_b)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x00, 0x00, 0x00};  // Key exchange received from slave - public key for secure exchange of LTK. Note the first two bytes should not be transmitted on authenticated/access keys.
    memcpy(&s2_public_key_frame[3], &public_key_b[0], sizeof(public_key_b));

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_public_key_frame, sizeof(s2_public_key_frame)), sizeof(s2_public_key_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving public key A, then we expect an event to be pushed up and both our public and private keys are requested from the keystore.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x81;  // Only S2 unauthenticated and S0 keys were granted
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;     // CSA was disabled by including node.
    memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // Expect Echo(KEX Report) to be sent.
    uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x81};  // Note: Echo flag set.

    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_get_2_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x01};  // Keys requested, Security2, class 2 - Security0, network key.
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_2_frame, sizeof(s2_net_key_get_2_frame)), sizeof(s2_net_key_get_2_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_frame, sizeof(s2_net_key_verify_frame)), sizeof(s2_net_key_verify_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_get_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x80};  // Keys requested, Security2, class 2 - Security0, network key.
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_0_frame, sizeof(s2_net_key_get_0_frame)), sizeof(s2_net_key_get_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_verify_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_0_frame, sizeof(s2_net_key_verify_0_frame)), sizeof(s2_net_key_verify_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Expect S2 Transfer End to be sent.
    uint8_t S2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x01};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_transfer_end_frame, sizeof(S2_transfer_end_frame)), sizeof(S2_transfer_end_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
    zwave_event_t *p_expected_complete_event                                          = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_complete_event->event_type                                             = S2_NODE_JOINING_COMPLETE_EVENT;
    p_expected_complete_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0x81;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_complete_event))).RetiresOnSaturation();

    /**
     *  Test execution.
     */
    struct S2 s2_context;

    s2_context.inclusion_state = S2_INC_IDLE;
    // memcpy(s2_context.temp_network_key, m_temp_key, sizeof(s2_context.temp_network_key));

    s2_inclusion_init(0x02, 0x01, 0x87);
    s2_inclusion_set_event_handler(s2_event_handler);

    s2_inclusion_joining_start(&s2_context, &s2_con, 0x01);

    // KEX Get frame received.
    uint8_t S2_kex_get_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
    s2_context.buf             = S2_kex_get_frame;
    s2_context.length          = sizeof(S2_kex_get_frame);
    s2_con.class_id            = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // KEX report is expected to be transmitted. Let's make the s2_send_frame a success.
    s2_inclusion_send_done(&s2_context, 1);

    // KEX Set frame received.
    // bit0: echo field not set, bit1-7: Reserved.
    // Selected schemes: scheme 0.
    // Selected curve25519
    // Keys to exchange, Security2, class 1 - Security0, network key.
    uint8_t s2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x00, 0x02, 0x01, 0x81};

    s2_context.buf    = s2_kex_set_frame;
    s2_context.length = sizeof(s2_kex_set_frame);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // As CSA was disabled, we expected that full public key was received.
    uint8_t public_key_frame[3 + sizeof(m_test_public_key_a)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x01};  // Key exchange received from slave - public key for secure exchange of LTK.
    memcpy(&public_key_frame[3], m_test_public_key_a, sizeof(m_test_public_key_a));
    s2_context.buf    = public_key_frame;
    s2_context.length = sizeof(public_key_frame);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // After the received public key is pushed upwards in the system, then it is expected to receive a challenge response from the upper level.
    // The challenge response should result in correct pub key being set on the context.
    s2_inclusion_challenge_response(&s2_context, 1, NULL, 0);
    EXPECT_EQ(0, memcmp(s2_context.public_key, m_test_public_key_a, sizeof(m_test_public_key_a)));

    // After receiving public key from joining node, the upper layer must specify our public key to be send. (This can also be done on initialization, but must happen no later than the received event).

    // Echo(KEX Report) frame received.
    // bit0: echo field set, bit1-7: Reserved.
    // Selected schemes: scheme 0 and scheme 2.
    // Selected curve25519
    // Keys to exchange, Security2, class 2 - Security0, network key.
    uint8_t s2_echo_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x07, 0x02, 0x01, 0x87};

    s2_context.buf    = s2_echo_kex_report_frame;
    s2_context.length = sizeof(s2_echo_kex_report_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Network Key Report frame received.
    uint8_t s2_net_key_report_frame[3 + sizeof(m_test_network_key_s2_class_1)] = {
      COMMAND_CLASS_SECURITY_2,
      SECURITY_2_NETWORK_KEY_REPORT,
      0x01,
    };  // Keys requested, Security2, class 1 - Security0, network key.
    memcpy(&s2_net_key_report_frame[3], m_test_network_key_s2_class_1, sizeof(m_test_network_key_s2_class_1));
    s2_context.buf    = s2_net_key_report_frame;
    s2_context.length = sizeof(s2_net_key_report_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // S2 Transfer end frame received.
    // bit0: Key request complete not set,
    // bit1: Key verified set,
    // bit2-7: Reserved.
    uint8_t s2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x02};
    s2_context.buf                  = s2_transfer_end_frame;
    s2_context.length               = sizeof(s2_transfer_end_frame);
    s2_con.class_id                 = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Network Key Report frame received.
    uint8_t s2_net_key_report_0_frame[3 + sizeof(m_test_network_key_s0)] = {
      COMMAND_CLASS_SECURITY_2,
      SECURITY_2_NETWORK_KEY_REPORT,
      0x80,
    };  // Keys requested, Security2, class 2 - Security0, network key.
    memcpy(&s2_net_key_report_0_frame[3], m_test_network_key_s0, sizeof(m_test_network_key_s0));
    s2_context.buf    = s2_net_key_report_0_frame;
    s2_context.length = sizeof(s2_net_key_report_0_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // S2 Transfer end frame received.
    // bit0: Key request complete not set,
    // bit1: Key verified set,
    // bit2-7: Reserved.
    s2_context.buf    = s2_transfer_end_frame;
    s2_context.length = sizeof(s2_transfer_end_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, true);
}

/** This test case is identical to @ref test_kex_joining_node_state_machine_unauthenticated except that a send failed
 *  is received by the S2 layer which should result in a retransmission of the frame.
 *
 */
TEST_F(InclusionTest, test_kex_joining_node_state_machine_retry_all_states)
{
    uint32_t send_frame_tries;
    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x02, 0x01, 0x01};
    uint8_t s2_public_key_frame[3 + sizeof(public_key_b)];
    uint8_t S2_echo_kex_set_frame[]   = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x01};
    uint8_t s2_net_key_get_2_frame[]  = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x01};
    uint8_t s2_net_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    uint8_t S2_transfer_end_frame[]   = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x01};

    libs2_clear_stubs();

    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_read");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_write");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    /**
     *  Test expectation setup.
     * This section set up the expectations in the system when the inclusion state machine progresses:
     * 1) Starting in idle it is expected that an S2 frame KEX1 will be transmitted based on external event when a node id has been assigned.
     * 2) After S2 KEX Get has been transmitted, the FSM awaits a S2 KEX Report as response.
     * 3) After S2 KEX Report is received then S2 KEX Set shall be sent.
     * 4) After S2 KEX Set  has been transmitted, the FSM awaits a Public KeyB from joining node.
     * 5) Exchange of public keys.
     *    a) Public KeyB is received from joining node A.
     *       - Public KeyB must be pushed upwards as event to be presented to user for further verification.
     *         If node is rejected, then node is part of network insecurely. Should this be changed ? Maybe force a node exclude.
     *         Currently that use case is outside the scope of libs2.
     *       - When Public KeyB is received, the controller must send its public KeyA.
     *         Public KeyB shall be provided by the application.
     *       - Using Pub KeyB and Private KeyA then a temporary key, KeyT is derived from the x coordinate of the shared secret.
     *         This is done in security sub module (currently under development)
     *       - The security context must be re-initialized with temporary derived key.
     *    b) Public KeyA is transmitted from including node A, the state machine will await 'Echo (KEX Set)'.
     * 6) Echoing of KEX frames.
     *    a) A 'Echo(KEX Set)' is expected to be received from the joining node, and it must be
     *       verified that 'Echo(KEX Set)' is identical to the KEX Set sent at step 3).
     *    b) 'Echo(KEX Report)' with content identical to packet received in 3) shall be replied to the joining node.
     * x) Nonce exchange: When joining node wants to send 'Echo(KEX Set)' the S2 protocol layer will exchange a nonce prior to communication.
     * 7) Network key exchange:
     *    a) Joining node will request the network key, by sending a 'Network Key Get'.
     *    b) Including node shall reply with a 'Network Key Set'.
     *    c) Joining node shall confirm with a 'Network Key Verify' which is encrypted using the the key obtained by 'NetworkKey-expand'
     *       - Including node must verify that it can succesfully decrypt the message using the key.
     *    d) Including node shall reply with a 'Transfer End'.
     *    e) Joining node shall reply with a 'Transfer End' if no more keys shall be exchanged.
     *
     */

    // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
    zwave_event_t *p_expected_inc_init_event = (zwave_event_t *)m_test_mem_pool[2];
    p_expected_inc_init_event->event_type    = S2_NODE_INCLUSION_INITIATED_EVENT;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_init_event))).RetiresOnSaturation();

    for (send_frame_tries = 0; send_frame_tries < 2; send_frame_tries++) {
        // Expect a S2 KEX Get to be sent.
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();
    }

    // Before exchange of public keys, then we expect that our public key is requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    s2_public_key_frame[0] = COMMAND_CLASS_SECURITY_2;
    s2_public_key_frame[1] = PUBLIC_KEY_REPORT;
    s2_public_key_frame[2] = 0x00;
    memcpy(&s2_public_key_frame[3], public_key_b, sizeof(public_key_b));
    for (send_frame_tries = 0; send_frame_tries < 2; send_frame_tries++) {
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_public_key_frame, sizeof(s2_public_key_frame)), sizeof(s2_public_key_frame))).WillOnce(Return(1)).RetiresOnSaturation();
    }

    // When receiving public key A, then we expect an event to be pushed up and both our public and private keys are requested from the keystore.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x01;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
    memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_dynamic_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // Expect Echo(KEX Set) to be sent.
    for (send_frame_tries = 0; send_frame_tries < 3; send_frame_tries++) {
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();
    }

    for (send_frame_tries = 0; send_frame_tries < 3; send_frame_tries++) {
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_2_frame, sizeof(s2_net_key_get_2_frame)), sizeof(s2_net_key_get_2_frame))).WillOnce(Return(1)).RetiresOnSaturation();
    }
    //  // After receiving the network key, we expect to get a call to update the context to use the new key when transmitting Net key verify.
    //  mock_call_expect(TO_STR(S2_network_key_update), &p_mock);
    //  p_mock->compare_rule_arg[0]   = COMPARE_ANY;  // For the outline, we just expect any/null pointers now.
    //  p_mock->expect_arg[1].pointer = m_test_network_key_s2_class_1;

    for (send_frame_tries = 0; send_frame_tries < 2; send_frame_tries++) {
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_frame, sizeof(s2_net_key_verify_frame)), sizeof(s2_net_key_verify_frame))).WillOnce(Return(1)).RetiresOnSaturation();
    }

    // Expect S2 Transfer End to be sent.
    for (send_frame_tries = 0; send_frame_tries < 2; send_frame_tries++) {
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_transfer_end_frame, sizeof(S2_transfer_end_frame)), sizeof(S2_transfer_end_frame))).WillOnce(Return(1)).RetiresOnSaturation();
    }

    // When S2 Transfer End is received, we expect a corresponding Node inclusion complete event from libs2.
    zwave_event_t *p_expected_complete_event                                          = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_complete_event->event_type                                             = S2_NODE_JOINING_COMPLETE_EVENT;
    p_expected_complete_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0x01;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_complete_event))).RetiresOnSaturation();

    /**
     *  Test execution.
     */
    struct S2 s2_context;

    s2_context.inclusion_state = S2_INC_IDLE;
    // memcpy(s2_context.temp_network_key, m_temp_key, sizeof(s2_context.temp_network_key));

    s2_inclusion_init(0x02, 0x01, 0x01);
    s2_inclusion_set_event_handler(s2_event_handler);

    s2_inclusion_joining_start(&s2_context, &s2_con, 0);

    // KEX Get frame received.
    uint8_t S2_kex_get_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
    s2_context.buf             = S2_kex_get_frame;
    s2_context.length          = sizeof(S2_kex_get_frame);
    s2_con.class_id            = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // KEX report is expected to be transmitted. Let's make the the first frame fail to verify a resent is transmitted.
    s2_inclusion_send_done(&s2_context, 0);

    // KEX report is expeted to be transmitted. Let's make the s2_send_frame a success.
    s2_inclusion_send_done(&s2_context, 1);

    // KEX Set frame received.
    // bit0: echo field not set, bit1-7: Reserved.
    // Selected schemes: scheme 0 and scheme 2.
    // Selected curve25519
    // Keys to exchange, Security2, class 2 - Security0, network key.
    uint8_t s2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x00, 0x02, 0x01, 0x01};
    s2_context.buf             = s2_kex_set_frame;
    s2_context.length          = sizeof(s2_kex_set_frame);
    s2_con.class_id            = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // KEX report is expected to be transmitted. Let's make the the first frame fail to verify a resent is transmitted.
    s2_inclusion_send_done(&s2_context, 0);

    uint8_t public_key_frame[3 + sizeof(m_test_public_key_a)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x01};  // Key exchange received from slave - public key for secure exchange of LTK.
    memcpy(&public_key_frame[3], m_test_public_key_a, sizeof(m_test_public_key_a));
    s2_context.buf    = public_key_frame;
    s2_context.length = sizeof(public_key_frame);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);

    // After receiving public key from joining node, the upper layer must specify our public key to be send. (This can also be done on initialization, but must happen no later than the received event).
    //  KEX set echo is expeted to be transmitted. Let's make the the first frame fail to verify a resent is transmitted.
    // After exchanging the keys, the other side might wait for user acceptance.
    // Therefore frames are retried per timeout basis instead send frame basis.
    // So a send done shoud not do any thing, only the timeout shoud result in retry.
    s2_inclusion_send_done(&s2_context, 0);  // Should do nothing
    s2_inclusion_notify_timeout(&s2_context);

    // Successful transmission, however, retries are expected until ECHO KEX Report is received (or 'n' retries due to timeouts).
    s2_inclusion_send_done(&s2_context, 1);
    s2_inclusion_notify_timeout(&s2_context);

    // Successful transmission, however, retries are expected until ECHO KEX Report is received (or 'n' retries due to timeouts).
    s2_inclusion_send_done(&s2_context, 1);

    // Echo(KEX Report) frame received.
    // bit0: echo field set, bit1-7: Reserved.
    // Selected schemes: scheme 0 and scheme 2.
    // Selected curve25519
    // Keys to exchange, Security2, class 2 - Security0, network key.
    uint8_t s2_echo_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x05, 0x02, 0x01, 0x01};
    s2_context.buf                     = s2_echo_kex_report_frame;
    s2_context.length                  = sizeof(s2_echo_kex_report_frame);
    s2_con.class_id                    = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // KEY get is expected to be transmitted. Let's make the the first two frames fail to verify that resents are transmitted.
    s2_inclusion_send_done(&s2_context, 0);
    s2_inclusion_send_done(&s2_context, 0);
    s2_inclusion_send_done(&s2_context, 1);

    // Network Key Report frame received.
    uint8_t s2_net_key_report_frame[3 + sizeof(m_test_network_key_s2_class_1)] = {
      COMMAND_CLASS_SECURITY_2,
      SECURITY_2_NETWORK_KEY_REPORT,
      0x01,
    };  // Keys requested, Security2, class 2 - Security0, network key.
    memcpy(&s2_net_key_report_frame[3], m_test_network_key_s2_class_1, sizeof(m_test_network_key_s2_class_0));
    s2_context.buf    = s2_net_key_report_frame;
    s2_context.length = sizeof(s2_net_key_report_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // KEY verify is expected to be transmitted. Let's make the first frame fail to verify that a resent is transmitted.
    s2_inclusion_send_done(&s2_context, 0);
    s2_inclusion_send_done(&s2_context, 1);

    // S2 Transfer end frame received.
    // bit0: Key request complete not set,
    // bit1: Key verified set,
    // bit2-7: Reserved.
    uint8_t s2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x02};
    s2_context.buf                  = s2_transfer_end_frame;
    s2_context.length               = sizeof(s2_transfer_end_frame);
    s2_con.class_id                 = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Transfer end complete is expected to be transmitted. Let's make the first frame fail to verify that a resent is transmitted.
    s2_inclusion_send_done(&s2_context, 0);
    s2_inclusion_send_done(&s2_context, 1);

    s2_inclusion_send_done(&s2_context, true);
}

/** Test that S2_inclusion_init() rejects LR key bits as it should. */
TEST_F(InclusionTest, test_setting_LR_key_bits_in_init)
{
    uint8_t retval = s2_inclusion_init(0x02, 0x01, 0x9F);
    EXPECT_EQ(1, retval);  // KEX_FAIL_KEX_KEY = 1

    retval = s2_inclusion_init(0x02, 0x01, 0x0F);
    EXPECT_EQ(1, retval);  // KEX_FAIL_KEX_KEY = 1

    retval = s2_inclusion_init(0x02, 0x01, 0x097);
    EXPECT_EQ(1, retval);  // KEX_FAIL_KEX_KEY = 1

    retval = s2_inclusion_init(0x02, 0x01, 0x0F);
    EXPECT_EQ(1, retval);  // KEX_FAIL_KEX_KEY = 1

    /* Clean up global m_keys and friends so subsequent test cases that don't call s2_inclusion_init continue to work */
    s2_inclusion_init(0x02, 0x01, 0x87);
}

///** Helper function sections - Start */
//
/** This helper function set up an expected KEX get frame to be send (through the mock). */
void helper_func_kex_report_frame_expect(uint8_t scheme, uint8_t curve, uint8_t keys)
{
    // Static to keep in scope for complete test.
    static uint8_t s2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x00, 0x00, 0x00};
    s2_kex_report_frame[3]               = scheme;
    s2_kex_report_frame[4]               = curve;
    s2_kex_report_frame[5]               = keys;

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_kex_report_frame, sizeof(s2_kex_report_frame)), sizeof(s2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();
}

/** This helper function constructs a KEX Get frame and update p_context */
void helper_func_kex_get_frame(struct S2 *p_context)
{
    uint8_t s2_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
    memcpy((uint8_t *)p_context->buf, s2_frame, sizeof(s2_frame));
    p_context->length = sizeof(s2_frame);
    s2_con.class_id   = 0xFF;
}

/** This helper function constructs a KEX Set frame and update p_context */
void helper_func_kex_set_frame(struct S2 *p_context, uint8_t scheme, uint8_t keys)
{
    uint8_t *p_tmp = (uint8_t *)p_context->buf;
    p_tmp[0]       = COMMAND_CLASS_SECURITY_2;
    p_tmp[1]       = KEX_SET;
    p_tmp[2]       = 0x00;
    p_tmp[3]       = scheme;
    p_tmp[4]       = 0x01;
    p_tmp[5]       = keys;

    p_context->length = 6;
    s2_con.class_id   = 0xFF;
}

void helper_func_pub_key_frame(struct S2 *p_context)
{
    uint8_t *p_tmp = (uint8_t *)p_context->buf;
    p_tmp[0]       = COMMAND_CLASS_SECURITY_2;
    p_tmp[1]       = PUBLIC_KEY_REPORT;
    p_tmp[2]       = 0x01;
    memcpy(&p_tmp[3], m_test_public_key_a, sizeof(m_test_public_key_a));
    p_context->length = 3 + sizeof(m_test_public_key_a);
    s2_con.class_id   = 0xFF;
}

void helper_func_echo_kex_report_frame(struct S2 *p_context, uint8_t scheme, uint8_t curve, uint8_t keys)
{
    uint8_t s2_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x05, 0x00, 0x00, 0x00};  // This data must be compared to earlier sent data. As long as frames are not defined, this data is unknown.
    s2_frame[3]        = scheme;
    s2_frame[4]        = curve;
    s2_frame[5]        = keys;

    memcpy((uint8_t *)p_context->buf, s2_frame, sizeof(s2_frame));
    p_context->length = sizeof(s2_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
}

void helper_func_net_key_report_frame(struct S2 *p_context, uint8_t key)
{
    //  uint8_t s2_frame[3];
    uint8_t *p_tmp = (uint8_t *)p_context->buf;
    p_tmp[0]       = COMMAND_CLASS_SECURITY_2;
    p_tmp[1]       = SECURITY_2_NETWORK_KEY_REPORT;
    p_tmp[2]       = key;
    if (key & 0x01) {
        memcpy(&p_tmp[3], m_test_network_key_s2_class_0, sizeof(m_test_network_key_s2_class_0));
        p_context->length = 3 + sizeof(m_test_network_key_s2_class_0);
    } else if (key & 0x02) {
        memcpy(&p_tmp[3], m_test_network_key_s2_class_1, sizeof(m_test_network_key_s2_class_1));
        p_context->length = 3 + sizeof(m_test_network_key_s2_class_1);
    } else if (key & 0x04) {
        memcpy(&p_tmp[3], m_test_network_key_s2_class_2, sizeof(m_test_network_key_s2_class_2));
        p_context->length = 3 + sizeof(m_test_network_key_s2_class_2);
    } else if (key & 0x80) {
        memcpy(&p_tmp[3], m_test_network_key_s0, sizeof(m_test_network_key_s0));
        p_context->length = 3 + sizeof(m_test_network_key_s0);
    } else {
        memcpy(&p_tmp[3], m_test_network_key_s0, sizeof(m_test_network_key_s0));
        p_context->length = 3 + sizeof(m_test_network_key_s0);
    }
    s2_con.class_id = UNIT_TEST_TEMP_KEY_SECURE;
}

void helper_func_transfer_end_frame(struct S2 *p_context)
{
    uint8_t s2_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x02};
    memcpy((uint8_t *)p_context->buf, s2_frame, sizeof(s2_frame));
    p_context->length = sizeof(s2_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
}

void helper_func_idle_state(struct S2 *p_context)
{

    // This test will trigger a timeout to the inclusion module after which we expect to receive a
    // callback event with a S2_NODE_INCLUSION_FAILED_EVENT.
    zwave_event_t *p_expected_event = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_event->event_type    = S2_NODE_INCLUSION_FAILED_EVENT;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_event))).RetiresOnSaturation();

    // Trigger a timeout to get back in idle for next loop.
    s2_inclusion_notify_timeout(p_context);
}

void helper_func_verify_idle_state(struct S2 *p_context, uint8_t scheme, uint8_t keys)
{

    if (!libs2_is_stubbed("s2_event_handler", "s2_event_handler_mock.c")) {
        // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
        zwave_event_t *p_expected_inc_init_event = (zwave_event_t *)m_test_mem_pool[3];
        p_expected_inc_init_event->event_type    = S2_NODE_INCLUSION_INITIATED_EVENT;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_init_event))).RetiresOnSaturation();
    }

    if (!libs2_is_stubbed("S2_send_frame", "s2_extern_mock.c")) {
        static uint8_t S2_kex_report_frame_retry[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x04, 0x00, 0x01, 0x00};
        S2_kex_report_frame_retry[3]               = scheme;
        S2_kex_report_frame_retry[5]               = keys;
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame_retry, sizeof(S2_kex_report_frame_retry)), sizeof(S2_kex_report_frame_retry))).WillOnce(Return(0)).RetiresOnSaturation();
    }

    if (!libs2_is_stubbed("s2_inclusion_set_timeout", "s2_inclusion_extern_mock.c")) {
        EXPECT_CALL(*g_libs2_mocks, s2_inclusion_set_timeout(_, 1000)).WillOnce(Return(1)).RetiresOnSaturation();
    }

    // At final stage retry node inclusion to ensure we are in idle stage.
    s2_inclusion_joining_start(p_context, &s2_con, 0);

    helper_func_kex_get_frame(p_context);
    s2_inclusion_post_event(p_context, &s2_con);
}

void helper_func_inclusion_initiated_event(void)
{

    // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
    zwave_event_t *p_expected_inc_init_event = (zwave_event_t *)m_test_mem_pool[3];
    p_expected_inc_init_event->event_type    = S2_NODE_INCLUSION_INITIATED_EVENT;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_init_event))).RetiresOnSaturation();
}

static void helper_func_init_s2_conn(void)
{
    s2_con.l_node     = 2;
    s2_con.r_node     = 1;
    s2_con.class_id   = 0xFF;
    s2_con.rx_options = 0x00;
}
static void helper_func_init_s2_conn_lr(void)
{
    s2_con.l_node     = 2;
    s2_con.r_node     = 0x101;
    s2_con.class_id   = 0xFF;
    s2_con.rx_options = 0x00;
}

/* The node(libs2 here) will be initialized s2_inclusion_init(0x87) will all the keys
 * but as the node id is LR, libs2 will send only 0x06 (LR keys) in kex report */
TEST_F(InclusionTest, test_kex_joining_node_state_machine_lr_requeesting_classic_keys)
{
    int test_flavour;
    for (test_flavour = 0; test_flavour < 2; test_flavour++) {
        uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key.
                                  0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

        uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                                   0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

        libs2_clear_stubs();
        libs2_stub("s2_inclusion_extern_mock.c");
        libs2_stub("S2_network_key_update");
        libs2_stub("keystore_network_key_read");
        libs2_stub("keystore_network_key_clear");
        libs2_stub("keystore_network_key_write");
        libs2_stub("crypto_scalarmult_curve25519");
        libs2_stub("tempkey_extract");

        /**
         *  Test expectation setup.
         * This section set up the expectations in the system when the inclusion state machine progresses:
         * 1) Starting in idle it is expected that an S2 frame KEX1 will be transmitted based on external event when a node id has been assigned.
         * 2) After S2 KEX Get has been transmitted, the FSM awaits a S2 KEX Report as response.
         * 3) After S2 KEX Report is received then S2 KEX Set shall be sent.
         * 4) After S2 KEX Set  has been transmitted, the FSM awaits a Public KeyB from joining node.
         * 5) Exchange of public keys.
         *    a) Public KeyB is received from joining node A.
         *       - Public KeyB must be pushed upwards as event to be presented to user for further verification.
         *         If node is rejected, then node is part of network insecurely. Should this be changed ? Maybe force a node exclude.
         *         Currently that use case is outside the scope of libs2.
         *       - When Public KeyB is received, the controller must send its public KeyA.
         *         Public KeyB shall be provided by the application.
         *       - Using Pub KeyB and Private KeyA then a temporary key, KeyT is derived from the x coordinate of the shared secret.
         *         This is done in security sub module (currently under development)
         *       - The security context must be re-initialized with temporary derived key.
         *    b) Public KeyA is transmitted from including node A, the state machine will await 'Echo (KEX Set)'.
         * 6) Echoing of KEX frames.
         *    a) A 'Echo(KEX Set)' is expected to be received from the joining node, and it must be
         *       verified that 'Echo(KEX Set)' is identical to the KEX Set sent at step 3).
         *    b) 'Echo(KEX Report)' with content identical to packet received in 3) shall be replied to the joining node.
         * x) Nonce exchange: When joining node wants to send 'Echo(KEX Set)' the S2 protocol layer will exchange a nonce prior to communication.
         * 7) Network key exchange:
         *    a) Joining node will request the network key, by sending a 'Network Key Get'.
         *    b) Including node shall reply with a 'Network Key Set'.
         *    c) Joining node shall confirm with a 'Network Key Verify' which is encrypted using the the key obtained by 'NetworkKey-expand'
         *       - Including node must verify that it can succesfully decrypt the message using the key.
         *    d) Including node shall reply with a 'Transfer End'.
         *    e) Joining node shall reply with a 'Transfer End' if no more keys shall be exchanged.
         *
         */
        struct S2 s2_context;

        s2_context.inclusion_state = S2_INC_IDLE;
        // memcpy(s2_context.temp_network_key, m_temp_key, sizeof(s2_context.temp_network_key));

        s2_inclusion_init(0x02, 0x01, 0x87);  // libs2 is joining node and its initialized with all keys
        s2_inclusion_set_event_handler(s2_event_handler);
        s2_con.l_node   = 0x101;  // libs2 node id
        s2_con.r_node   = 1;      // unit test node id
        s2_con.class_id = 0xFF;

        // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
        zwave_event_t *p_expected_inc_init_event = (zwave_event_t *)m_test_mem_pool[2];
        p_expected_inc_init_event->event_type    = S2_NODE_INCLUSION_INITIATED_EVENT;
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_init_event))).RetiresOnSaturation();

        // Expect a S2 KEX report frame (libs2 will send it)
        uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2,
                                         KEX_REPORT,
                                         0x04,   // reserved | NLS support | Request CSA | Echo
                                         0x02,   // Supported schemes. Scheme 0 and 2.
                                         0x01,   // Supported ECDH Profiles, bit0=1 is curve 25519 value.
                                         0x06};  // Requested keys bits. Authenticated and Acess keys
        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        s2_inclusion_joining_start(&s2_context, &s2_con, 0x00);
        // Inject KEX Get frame
        uint8_t S2_kex_get_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
        s2_context.buf             = S2_kex_get_frame;
        s2_context.length          = sizeof(S2_kex_get_frame);
        s2_con.class_id            = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // Before exchange of public keys, then we expect that our public key is requested from the keystore.
        EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        uint8_t s2_public_key_frame[3 + sizeof(public_key_b)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x00, 0x00, 0x00};  // Key exchange received from slave - public key for secure exchange of LTK. Note the first two bytes should not be transmitted on authenticated/access keys.
        memcpy(&s2_public_key_frame[5], &public_key_b[2], sizeof(public_key_b) - 2);

        EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_public_key_frame, sizeof(s2_public_key_frame)), sizeof(s2_public_key_frame))).WillOnce(Return(1)).RetiresOnSaturation();
        // KEX report is expeted to be transmitted. Let's make the s2_send_frame a success.
        s2_inclusion_send_done(&s2_context, 1);

        // Inject KEX Set frame
        // bit0: echo field not set, bit1-7: Reserved.
        // Selected schemes: scheme 0 and scheme 2.
        // Selected curve25519
        // Keys to exchange, Security2, class 2 - Security0, network key.
        uint8_t s2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x00, 0x02, 0x01, 0x06};

        if (test_flavour == 1) {
            /* Test the special case where a reserved bit is set in the KEX_SET */
            s2_kex_set_frame[2] |= 0x10;
        }
        s2_context.buf    = s2_kex_set_frame;
        s2_context.length = sizeof(s2_kex_set_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // When receiving public key A, then we expect an event to be pushed up and both our public and private keys are requested from the keystore.
        zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
        p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x06;
        p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
        memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));
        EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

        EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
        EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

        // Inject Public key report frame
        uint8_t public_key_frame[3 + sizeof(m_test_public_key_a)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x01};  // Key exchange received from slave - public key for secure exchange of LTK.
        memcpy(&public_key_frame[3], m_test_public_key_a, sizeof(m_test_public_key_a));
        s2_context.buf    = public_key_frame;
        s2_context.length = sizeof(public_key_frame);
        s2_con.class_id   = 0xFF;
        s2_inclusion_post_event(&s2_context, &s2_con);

        // Expect Echo(KEX Report) (libs2 will send it)
        uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x06};  // Note: Echo flag set.
        if (test_flavour == 1) {
            /* Test the special case where a reserved bit is set in the KEX_SET */

            /* Expect the reserved bit 0x10 to be set in the KEX_SET_ECHO */
            S2_echo_kex_set_frame[2] |= 0x10;
        }
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // Expect Network Key get (libs2 will send it)
        uint8_t s2_net_key_get_2_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};  // Keys requested, Security2, class 2 - Security0, network key.
        EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_2_frame, sizeof(s2_net_key_get_2_frame)), sizeof(s2_net_key_get_2_frame))).WillOnce(Return(1)).RetiresOnSaturation();

        // After the received public key is pushed upwards in the system, then it is expected to receive a challenge response from the upper level.
        // The challenge response should result in correct pub key being set on the context.
        s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);
        EXPECT_EQ(0, memcmp(s2_context.public_key, m_test_public_key_a, sizeof(m_test_public_key_a)));

        // After receiving public key from joining node, the upper layer must specify our public key to be send. (This can also be done on initialization, but must happen no later than the received event).

        // Echo(KEX Report) frame received.
        // bit0: echo field set, bit1-7: Reserved.
        // Selected schemes: scheme 0 and scheme 2.
        // Selected curve25519
        // Keys to exchange, Security2, class 2 - Security0, network key.
        uint8_t s2_echo_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x05, 0x02, 0x01, 0x06};  // this is being sent from unit test to libs2

        s2_context.buf    = s2_echo_kex_report_frame;
        s2_context.length = sizeof(s2_echo_kex_report_frame);
        s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
        s2_inclusion_post_event(&s2_context, &s2_con);

    } /* for(test_flavour) */
}

/**
 * This test is the same as test_kex_joining_node_state_machine_ssa(),
 * except that in this test, last S2 Transfer End received by joining node,
 * has Verify Key flag set to 0, which should stop the inclusion
 */
TEST_F(InclusionTest, test_kex_joining_node_error_transfer_end)
{

    uint8_t public_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Public key.
                              0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    uint8_t private_key_b[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,  // Private key.
                               0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22};

    libs2_clear_stubs();
    libs2_stub("s2_inclusion_extern_mock.c");
    libs2_stub("S2_network_key_update");
    libs2_stub("keystore_network_key_read");
    libs2_stub("keystore_network_key_clear");
    libs2_stub("keystore_network_key_write");
    libs2_stub("crypto_scalarmult_curve25519");
    libs2_stub("tempkey_extract");

    /**
     *  Test expectation setup.
     * This section set up the expectations in the system when the inclusion state machine progresses:
     * 1) Starting in idle it is expected that an S2 frame KEX1 will be transmitted based on external event when a node id has been assigned.
     * 2) After S2 KEX Get has been transmitted, the FSM awaits a S2 KEX Report as response.
     * 3) After S2 KEX Report is received then S2 KEX Set shall be sent.
     * 4) After S2 KEX Set  has been transmitted, the FSM awaits a Public KeyB from joining node.
     * 5) Exchange of public keys.
     *    a) Public KeyB is received from joining node A.
     *       - Public KeyB must be pushed upwards as event to be presented to user for further verification.
     *         If node is rejected, then node is part of network insecurely. Should this be changed ? Maybe force a node exclude.
     *         Currently that use case is outside the scope of libs2.
     *       - When Public KeyB is received, the controller must send its public KeyA.
     *         Public KeyB shall be provided by the application.
     *       - Using Pub KeyB and Private KeyA then a temporary key, KeyT is derived from the x coordinate of the shared secret.
     *         This is done in security sub module (currently under development)
     *       - The security context must be re-initialized with temporary derived key.
     *    b) Public KeyA is transmitted from including node A, the state machine will await 'Echo (KEX Set)'.
     * 6) Echoing of KEX frames.
     *    a) A 'Echo(KEX Set)' is expected to be received from the joining node, and it must be
     *       verified that 'Echo(KEX Set)' is identical to the KEX Set sent at step 3).
     *    b) 'Echo(KEX Report)' with content identical to packet received in 3) shall be replied to the joining node.
     * x) Nonce exchange: When joining node wants to send 'Echo(KEX Set)' the S2 protocol layer will exchange a nonce prior to communication.
     * 7) Network key exchange:
     *    a) Joining node will request the network key, by sending a 'Network Key Get'.
     *    b) Including node shall reply with a 'Network Key Set'.
     *    c) Joining node shall confirm with a 'Network Key Verify' which is encrypted using the the key obtained by 'NetworkKey-expand'
     *       - Including node must verify that it can successfully decrypt the message using the key.
     *    d) Including node shall reply with a 'Transfer End' having Key Verify flag set to 0
     *    e) Joining node aborts inclusion process.
     *
     */

    // When secure learn mode is started then the following event is expected when the joining node receives a KEX Get from including node.
    zwave_event_t *p_expected_inc_init_event = (zwave_event_t *)m_test_mem_pool[2];
    p_expected_inc_init_event->event_type    = S2_NODE_INCLUSION_INITIATED_EVENT;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_init_event))).RetiresOnSaturation();

    // Expect a S2 KEX Get to be sent.
    uint8_t S2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2,
                                     KEX_REPORT,
                                     0x04,   // reserved | NLS support | Request CSA | Echo
                                     0x02,   // Supported schemes. Scheme 0 and 2.
                                     0x01,   // Supported ECDH Profiles, bit0=1 is curve 25519 value.
                                     0x82};  // Requested keys bits. Security 2 class 1, Security 0 network key.

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_kex_report_frame, sizeof(S2_kex_report_frame)), sizeof(S2_kex_report_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Before exchange of public keys, then we expect that our public key is requested from the keystore.
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    uint8_t s2_public_key_frame[3 + sizeof(public_key_b)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x00, 0x00, 0x00};  // Key exchange received from slave - public key for secure exchange of LTK. Note the first two bytes should not be transmitted on authenticated/access keys.
    memcpy(&s2_public_key_frame[5], &public_key_b[2], sizeof(public_key_b) - 2);

    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(s2_public_key_frame, sizeof(s2_public_key_frame)), sizeof(s2_public_key_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When receiving public key A, then we expect an event to be pushed up and both our public and private keys are requested from the keystore.
    zwave_event_t *p_expected_inc_req_event                                   = (zwave_event_t *)m_test_mem_pool[0];
    p_expected_inc_req_event->event_type                                      = S2_NODE_INCLUSION_PUBLIC_KEY_CHALLENGE_EVENT;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.length       = sizeof(m_test_public_key_a);
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.granted_keys = 0x82;
    p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.dsk_length   = 0;
    memcpy(p_expected_inc_req_event->evt.s2_event.s2_data.challenge_req.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_inc_req_event))).RetiresOnSaturation();

    EXPECT_CALL(*g_libs2_mocks, keystore_private_key_read(_)).WillOnce(SetArrayArgument<0>(private_key_b, private_key_b + 32)).RetiresOnSaturation();
    EXPECT_CALL(*g_libs2_mocks, keystore_public_key_read(_)).WillOnce(SetArrayArgument<0>(public_key_b, public_key_b + 32)).RetiresOnSaturation();

    // Expect Echo(KEX Report) to be sent.
    uint8_t S2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x01, 0x02, 0x01, 0x82};  // Note: Echo flag set.

    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(S2_echo_kex_set_frame, sizeof(S2_echo_kex_set_frame)), sizeof(S2_echo_kex_set_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_get_2_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x02};  // Keys requested, Security2, class 2 - Security0, network key.
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_2_frame, sizeof(s2_net_key_get_2_frame)), sizeof(s2_net_key_get_2_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_verify_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_frame, sizeof(s2_net_key_verify_frame)), sizeof(s2_net_key_verify_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_get_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_GET, 0x80};  // Keys requested, Security2, class 2 - Security0, network key.
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_get_0_frame, sizeof(s2_net_key_get_0_frame)), sizeof(s2_net_key_get_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    uint8_t s2_net_key_verify_0_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_data(_, NotNull(), BufferEq(s2_net_key_verify_0_frame, sizeof(s2_net_key_verify_0_frame)), sizeof(s2_net_key_verify_0_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // When S2 Transfer End with Key Verify set to 0 is received, we expect a joining node to abort inclusion
    zwave_event_t *p_expected_complete_event                                          = (zwave_event_t *)m_test_mem_pool[1];
    p_expected_complete_event->event_type                                             = S2_NODE_INCLUSION_FAILED_EVENT;
    p_expected_complete_event->evt.s2_event.s2_data.inclusion_complete.exchanged_keys = 0x0;
    p_expected_complete_event->evt.s2_event.s2_data.inclusion_fail.kex_fail_type      = KEX_FAIL_KEY_VERIFY;
    EXPECT_CALL(*g_libs2_mocks, s2_event_handler(EventMatches(p_expected_complete_event))).RetiresOnSaturation();

    /**
     *  Test execution.
     */
    struct S2 s2_context;

    s2_context.inclusion_state = S2_INC_IDLE;
    s2_inclusion_init(0x02, 0x01, 0x82);
    s2_inclusion_set_event_handler(s2_event_handler);
    helper_func_init_s2_conn();

    s2_inclusion_joining_start(&s2_context, &s2_con, 0x00);

    // KEX Get frame received.
    uint8_t S2_kex_get_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
    s2_context.buf             = S2_kex_get_frame;
    s2_context.length          = sizeof(S2_kex_get_frame);
    s2_con.class_id            = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // KEX report is expeted to be transmitted. Let's make the s2_send_frame a success.
    s2_inclusion_send_done(&s2_context, 1);

    // KEX Set frame received.
    // bit0: echo field not set, bit1-7: Reserved.
    // Selected schemes: scheme 0 and scheme 2.
    // Selected curve25519
    // Keys to exchange, Security2, class 2 - Security0, network key.
    uint8_t s2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x00, 0x02, 0x01, 0x82};

    s2_context.buf    = s2_kex_set_frame;
    s2_context.length = sizeof(s2_kex_set_frame);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    uint8_t public_key_frame[3 + sizeof(m_test_public_key_a)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x01};  // Key exchange received from slave - public key for secure exchange of LTK.
    memcpy(&public_key_frame[3], m_test_public_key_a, sizeof(m_test_public_key_a));
    s2_context.buf    = public_key_frame;
    s2_context.length = sizeof(public_key_frame);
    s2_con.class_id   = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // After the received public key is pushed upwards in the system, then it is expected to receive a challenge response from the upper level.
    // The challenge response should result in correct pub key being set on the context.
    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, 0);
    EXPECT_EQ(0, memcmp(s2_context.public_key, m_test_public_key_a, sizeof(m_test_public_key_a)));

    // After receiving public key from joining node, the upper layer must specify our public key to be send. (This can also be done on initialization, but must happen no later than the received event).

    // Echo(KEX Report) frame received.
    // bit0: echo field set, bit1-7: Reserved.
    // Selected schemes: scheme 0 and scheme 2.
    // Selected curve25519
    // Keys to exchange, Security2, class 2 - Security0, network key.
    uint8_t s2_echo_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x05, 0x02, 0x01, 0x82};

    s2_context.buf    = s2_echo_kex_report_frame;
    s2_context.length = sizeof(s2_echo_kex_report_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Network Key Report frame received.
    uint8_t s2_net_key_report_frame[3 + sizeof(m_test_network_key_s2_class_1)] = {
      COMMAND_CLASS_SECURITY_2,
      SECURITY_2_NETWORK_KEY_REPORT,
      0x02,
    };  // Keys requested, Security2, class 2 - Security0, network key.
    memcpy(&s2_net_key_report_frame[3], m_test_network_key_s2_class_1, sizeof(m_test_network_key_s2_class_1));
    s2_context.buf    = s2_net_key_report_frame;
    s2_context.length = sizeof(s2_net_key_report_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // S2 Transfer end frame received.
    // bit0: Key request complete not set,
    // bit1: Key verified set,
    // bit2-7: Reserved.
    uint8_t s2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x02};
    s2_context.buf                  = s2_transfer_end_frame;
    s2_context.length               = sizeof(s2_transfer_end_frame);
    s2_con.class_id                 = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Network Key Report frame received.
    uint8_t s2_net_key_report_0_frame[3 + sizeof(m_test_network_key_s0)] = {
      COMMAND_CLASS_SECURITY_2,
      SECURITY_2_NETWORK_KEY_REPORT,
      0x80,
    };  // Keys requested, Security2, class 2 - Security0, network key.
    memcpy(&s2_net_key_report_0_frame[3], m_test_network_key_s0, sizeof(m_test_network_key_s0));
    s2_context.buf    = s2_net_key_report_0_frame;
    s2_context.length = sizeof(s2_net_key_report_0_frame);
    s2_con.class_id   = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Expect KEX FAIL to be sent.
    uint8_t S2_network_kex_fail_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_FAIL, KEX_FAIL_KEY_VERIFY};
    EXPECT_CALL(*g_libs2_mocks, S2_send_frame(_, NotNull(), BufferEq(S2_network_kex_fail_frame, sizeof(S2_network_kex_fail_frame)), sizeof(S2_network_kex_fail_frame))).WillOnce(Return(1)).RetiresOnSaturation();

    // Set Key Verify to 0 and verify that inclusion gets aborted after it.

    // S2 Transfer end frame received.
    // bit0: Key request complete not set,
    // bit1: Key verified not set,
    // bit2-7: Reserved.
    s2_transfer_end_frame[2] = 0x00;  // {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x00}
    s2_context.buf           = s2_transfer_end_frame;
    s2_context.length        = sizeof(s2_transfer_end_frame);
    s2_con.class_id          = UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // S2 inclusion is aborted now
}
