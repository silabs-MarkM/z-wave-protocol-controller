/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <gtest/gtest.h>

#include "attribute.hpp"
#include "attribute_store.h"
#include "attribute_store_fixt.h"
#include "command_class_schedule.hpp"
#include "command_class_schedule_attribute_store.hpp"
#include "command_class_schedule_mqtt.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zwave_frame_generator.hpp"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_schedule_types;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class ScheduleCommandClassTest : public ::testing::Test
{
  protected:
    static command_class_schedule &component()
    {
        static command_class_schedule instance;
        return instance;
    }

    void SetUp() override
    {
        ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
        auto root = attribute_store::attribute::root();
        auto home = root.emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>{0x12, 0x34, 0x56, 0x78});
        auto node = home.emplace_node(ATTRIBUTE_NODE_ID, uint16_t{2});
        endpoint = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, uint8_t{0});
        endpoint_node_id = attribute_store_get_node_child_by_type(
          attribute_store_get_node_child_by_type(attribute_store_get_node_child_by_type(attribute_store_get_root(), ATTRIBUTE_HOME_ID, 0), ATTRIBUTE_NODE_ID, 0),
          ATTRIBUTE_ENDPOINT_ID,
          0);
        (void)component();
    }

    void TearDown() override
    {
        attribute_store_teardown();
    }

    template<typename AttributeEnum, typename Value> void desired(attribute_store::attribute group, AttributeEnum type, Value value)
    {
        group.emplace_node(static_cast<attribute_store_type_t>(type)).set_desired(value);
    }

    attribute_store::attribute endpoint;
    attribute_store_node_t endpoint_node_id = ATTRIBUTE_STORE_INVALID_NODE;
};

// CC:0053.02.02.11.001, CC:0053.02.02.11.002
// Verifies supported capabilities are stored by schedule block, including packed supported-CC flags.
TEST_F(ScheduleCommandClassTest, StoresSupportedReportFieldsPerBlock)
{
    // Store one supported-CC entry to verify its wire bytes are flattened without losing the flags.
    schedule_supported_report_vg1_t supported_ccs(1);
    supported_ccs[0].supported_cc = 0x20;
    supported_ccs[0].properties2.value = 0x03;
    command_class_schedule_attribute_map_t report{
      {"schedule_id_block", uint8_t{2}},
      {"number_of_supported_schedule_id", uint8_t{5}},
      {"start_time_support", uint8_t{0x15}},
      {"fallback_support", uint8_t{1}},
      {"support_enable_disable", uint8_t{1}},
      {"number_of_supported_cc", uint8_t{1}},
      {"vg1", supported_ccs},
      {"supported_override_types", uint8_t{0x12}},
      {"override_support", uint8_t{1}},
      {"number_of_supported_schedule_blocks", uint8_t{3}},
    };

    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_report_received_store(endpoint, report));
    auto group = command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, 2);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(5, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_id)).reported<uint8_t>());
    EXPECT_EQ(0x15, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::start_time_support)).reported<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t>{0x20, 0x03}), group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::vg1)).reported<std::vector<uint8_t>>());
    EXPECT_EQ(0x12, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::supported_override_types)).reported<uint8_t>());
    EXPECT_EQ(3, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_report_group_attributes_t::number_of_supported_schedule_blocks)).reported<uint8_t>());
    EXPECT_TRUE(group == command_class_schedule_attribute_store::find_or_create_supported_report_group_by_schedule_id_block(endpoint, 2));
}

// CC:0053.04.0B.11.001
// Verifies v4 supported-command entries are stored as class, byte count, and command bytes.
TEST_F(ScheduleCommandClassTest, StoresSupportedCommandsReportAndUsesDefaultBlock)
{
    // Omitting the optional block selects block zero, and nested command lists retain their lengths.
    schedule_supported_commands_report_vg1_t commands{{0x20, 2, {0x01, 0x02}}, {0x25, 1, {0x03}}};
    command_class_schedule_attribute_map_t report{{"command_class_list_length", uint8_t{2}}, {"vg1", commands}};

    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_commands_report_received_store(endpoint, report));
    auto group = command_class_schedule_attribute_store::find_supported_commands_report_group_by_schedule_id_block(endpoint, 0);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(2, group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::command_class_list_length)).reported<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t>{0x20, 2, 0x01, 0x02, 0x25, 1, 0x03}), group.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_report_group_attributes_t::vg1)).reported<std::vector<uint8_t>>());
    EXPECT_TRUE(group == command_class_schedule_attribute_store::find_or_create_supported_commands_report_group_by_schedule_id_block(endpoint, 0));
}

// CC:0053.02.01.11.001, CC:0053.04.0A.11.001
// Verifies interview queries block zero in versions 1–4 and adds the supported-commands query only in v4.
TEST_F(ScheduleCommandClassTest, InterviewRequestsCapabilitiesForSupportedVersions)
{
    // The v4 command query is absent in v1–v3 and appears when interview reaches version 4.
    for (const uint8_t version : {uint8_t{1}, uint8_t{2}, uint8_t{3}, uint8_t{4}}) {
        endpoint.emplace_node(ATTRIBUTE_ZWAVE_NIF, std::vector<uint8_t>{0x53});
        endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_schedule_attributes_t::supported_version)).set_reported(version);
        component_connector connector;
        const component_connector_interview_done_payload_t payload{endpoint_node_id, SL_STATUS_OK};
        auto status = connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), payload);
        connector.run();
        ASSERT_EQ(SL_STATUS_OK, status.get());
        EXPECT_EQ(version >= 4,
                  endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP)).is_valid());
    }

    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP)).is_valid());
    EXPECT_TRUE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, 0).is_valid());
    EXPECT_TRUE(command_class_schedule_attribute_store::find_supported_commands_report_group_by_schedule_id_block(endpoint, 0).is_valid());
}

// CC:0053.02.02.11.003
// Verifies a Supported Report starts the next block query only while another block remains.
TEST_F(ScheduleCommandClassTest, SupportedReportAdvancesBlockInterviewUntilLastBlock)
{
    // A report advertising three blocks advances from block zero to one, then stops at block two.
    const auto receive_supported_report = [this](uint8_t block, uint8_t number_of_blocks) {
        const uint8_t frame[] = {0x53, 0x02, 0, 0, 0, 0, block, number_of_blocks};
        zwave_frame_parser parser{frame, sizeof(frame)};
        command_class_schedule_core::report_received_args args{endpoint, parser, nullptr};
        return component().on_schedule_supported_report_received(args);
    };
    ASSERT_EQ(SL_STATUS_OK, receive_supported_report(0, 3));
    ASSERT_EQ(SL_STATUS_OK, receive_supported_report(1, 3));
    ASSERT_EQ(SL_STATUS_OK, receive_supported_report(2, 3));
    EXPECT_TRUE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, 1).is_valid());
    EXPECT_TRUE(command_class_schedule_attribute_store::find_supported_report_group_by_schedule_id_block(endpoint, 2).is_valid());
    EXPECT_EQ(SL_STATUS_OK, receive_supported_report(0, 0));
}

// CC:0053.02.01.11.001
// Verifies Supported Get encodes its requested schedule block and rejects a missing block value.
TEST_F(ScheduleCommandClassTest, SupportedGetBuildsBlockFrameAndRejectsMissingBlock)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP));
    uint8_t data[8]{};
    uint16_t length = 0;
    zwave_frame_generator generator{0x53};
    generator.initialize_frame(0x01, data, &length);
    command_class_schedule_core::get_requested_args args{group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_supported_get_requested(args, data, &length));

    desired(group, schedule_supported_get_group_attributes_t::schedule_id_block, uint8_t{2});
    generator.initialize_frame(0x01, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_get_requested(args, data, &length));
    EXPECT_EQ(3, length);
    EXPECT_EQ(2, data[2]);
}

// CC:0053.02.03.11.001, CC:0053.03.03.11.001
// Verifies Set packs schedule date/time flags and appends a length-prefixed nested command.
TEST_F(ScheduleCommandClassTest, CommandScheduleSetPacksFieldsAndRejectsIncompleteInput)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::COMMAND_SCHEDULE_SET_GROUP));
    uint8_t data[32]{};
    uint16_t length = 0;
    zwave_frame_generator generator{0x53};
    generator.initialize_frame(0x03, data, &length);
    command_class_schedule_core::set_requested_args args{group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_command_schedule_set_requested(args, data, &length));

    desired(group, command_schedule_set_group_attributes_t::schedule_id, uint8_t{3});
    desired(group, command_schedule_set_group_attributes_t::schedule_id_block, uint8_t{1});
    desired(group, command_schedule_set_group_attributes_t::start_year, uint8_t{26});
    desired(group, command_schedule_set_group_attributes_t::start_month, uint8_t{4});
    desired(group, command_schedule_set_group_attributes_t::recurrence_offset, uint8_t{2});
    desired(group, command_schedule_set_group_attributes_t::start_day_of_month, uint8_t{9});
    desired(group, command_schedule_set_group_attributes_t::recurrence_mode, uint8_t{1});
    desired(group, command_schedule_set_group_attributes_t::start_weekday, uint8_t{5});
    desired(group, command_schedule_set_group_attributes_t::start_hour, uint8_t{10});
    desired(group, command_schedule_set_group_attributes_t::duration_type, uint8_t{2});
    desired(group, command_schedule_set_group_attributes_t::start_minute, uint8_t{12});
    desired(group, command_schedule_set_group_attributes_t::relative, uint8_t{1});
    desired(group, command_schedule_set_group_attributes_t::duration_byte, uint16_t{90});
    desired(group, command_schedule_set_group_attributes_t::reports_to_follow, uint8_t{0});
    desired(group, command_schedule_set_group_attributes_t::number_of_cmd_to_follow, uint8_t{1});
    desired(group, command_schedule_set_group_attributes_t::vg1, std::vector<uint8_t>{2, 0x20, 0x01});

    generator.initialize_frame(0x03, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_command_schedule_set_requested(args, data, &length));
    EXPECT_EQ(17, length);
    EXPECT_EQ(3, data[2]);
    EXPECT_EQ(0x24, data[5]);
    EXPECT_EQ(0x29, data[6]);
    EXPECT_EQ(0x4A, data[8]);
    EXPECT_EQ(0x4C, data[9]);
    EXPECT_EQ(0, data[12]);
    EXPECT_EQ(90, data[11]);
    EXPECT_EQ((std::vector<uint8_t>{2, 0x20, 0x01}), std::vector<uint8_t>(data + 14, data + length));
}

// CC:0053.02.04.11.001
// Verifies Schedule Get packs the override control bit, while Remove and State commands preserve byte order.
TEST_F(ScheduleCommandClassTest, GetRemoveAndStateCommandsEncodeTheirRequestedFields)
{
    uint8_t data[8]{};
    uint16_t length = 0;
    zwave_frame_generator generator{0x53};

    auto get = endpoint.emplace_node(static_cast<attribute_store_type_t>(command_schedule_get_group_attributes_t::COMMAND_SCHEDULE_GET_GROUP));
    command_class_schedule_core::get_requested_args get_args{get, &generator};
    generator.initialize_frame(0x04, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_command_schedule_get_requested(get_args, data, &length));
    desired(get, command_schedule_get_group_attributes_t::schedule_id, uint8_t{7});
    desired(get, command_schedule_get_group_attributes_t::schedule_id_block, uint8_t{2});
    desired(get, command_schedule_get_group_attributes_t::aid_ro_ctl, uint8_t{1});
    generator.initialize_frame(0x04, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_command_schedule_get_requested(get_args, data, &length));
    EXPECT_EQ((std::vector<uint8_t>{0x53, 0x04, 7, 2, 0x80}), std::vector<uint8_t>(data, data + length));

    auto remove = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_remove_group_attributes_t::SCHEDULE_REMOVE_GROUP));
    command_class_schedule_core::set_requested_args remove_args{remove, &generator};
    generator.initialize_frame(0x06, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_remove_requested(remove_args, data, &length));
    desired(remove, schedule_remove_group_attributes_t::schedule_id, uint8_t{7});
    desired(remove, schedule_remove_group_attributes_t::schedule_id_block, uint8_t{2});
    generator.initialize_frame(0x06, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_remove_requested(remove_args, data, &length));
    EXPECT_EQ((std::vector<uint8_t>{0x53, 0x06, 7, 2}), std::vector<uint8_t>(data, data + length));

    auto state_set = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::SCHEDULE_STATE_SET_GROUP));
    command_class_schedule_core::set_requested_args state_set_args{state_set, &generator};
    generator.initialize_frame(0x07, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_state_set_requested(state_set_args, data, &length));
    desired(state_set, schedule_state_set_group_attributes_t::schedule_id, uint8_t{7});
    desired(state_set, schedule_state_set_group_attributes_t::schedule_state, uint8_t{1});
    desired(state_set, schedule_state_set_group_attributes_t::schedule_id_block, uint8_t{2});
    generator.initialize_frame(0x07, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_state_set_requested(state_set_args, data, &length));
    EXPECT_EQ((std::vector<uint8_t>{0x53, 0x07, 7, 1, 2}), std::vector<uint8_t>(data, data + length));
}

// CC:0053.02.08.11.001, CC:0053.04.0A.11.001
// Verifies the v1 State Get and v4 Supported Commands Get each carry their block selector.
TEST_F(ScheduleCommandClassTest, StateAndSupportedCommandsGetEncodeBlock)
{
    uint8_t data[8]{};
    uint16_t length = 0;
    zwave_frame_generator generator{0x53};

    auto state = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_state_get_group_attributes_t::SCHEDULE_STATE_GET_GROUP));
    command_class_schedule_core::get_requested_args state_args{state, &generator};
    generator.initialize_frame(0x08, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_state_get_requested(state_args, data, &length));
    desired(state, schedule_state_get_group_attributes_t::schedule_id_block, uint8_t{3});
    generator.initialize_frame(0x08, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_state_get_requested(state_args, data, &length));
    EXPECT_EQ((std::vector<uint8_t>{0x53, 0x08, 3}), std::vector<uint8_t>(data, data + length));

    auto supported = endpoint.emplace_node(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP));
    command_class_schedule_core::get_requested_args supported_args{supported, &generator};
    generator.initialize_frame(0x0A, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_schedule_supported_commands_get_requested(supported_args, data, &length));
    desired(supported, schedule_supported_commands_get_group_attributes_t::schedule_id_block, uint8_t{3});
    generator.initialize_frame(0x0A, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_schedule_supported_commands_get_requested(supported_args, data, &length));
    EXPECT_EQ((std::vector<uint8_t>{0x53, 0x0A, 3}), std::vector<uint8_t>(data, data + length));
}

// CC:0053.02.01.11.001, CC:0053.02.03.11.001, CC:0053.02.04.11.001
// Verifies MQTT parses required Schedule fields and stores desired values before starting resolutions.
TEST_F(ScheduleCommandClassTest, MqttCommandsCreateGroupsAndRejectMalformedPayloads)
{
    // Commands with missing required properties fail without creating a valid request group.
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_supported_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_remove_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_state_set_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_state_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_supported_commands_get_command(endpoint, "{}"));

    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_supported_get_command(endpoint, R"({"schedule_id_block":2})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_get_command(endpoint, R"({"schedule_id":7,"schedule_id_block":2,"properties1":{"aid_ro_ctl":1}})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_remove_command(endpoint, R"({"schedule_id":7,"schedule_id_block":2})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_state_set_command(endpoint, R"({"schedule_id":7,"schedule_state":1,"schedule_id_block":2})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_state_get_command(endpoint, R"({"schedule_id_block":2})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_schedule_supported_commands_get_command(endpoint, R"({"schedule_id_block":2})"));

    EXPECT_EQ(2, endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::SCHEDULE_SUPPORTED_GET_GROUP)).child_by_type(static_cast<attribute_store_type_t>(schedule_supported_get_group_attributes_t::schedule_id_block)).desired<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::SCHEDULE_STATE_SET_GROUP)).child_by_type(static_cast<attribute_store_type_t>(schedule_state_set_group_attributes_t::schedule_state)).desired<uint8_t>());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(schedule_supported_commands_get_group_attributes_t::SCHEDULE_SUPPORTED_COMMANDS_GET_GROUP)).is_valid());
}

// CC:0053.02.03.11.001, CC:0053.03.03.11.001
// Verifies MQTT Schedule Set parses nested properties and length-prefixed nested command arrays.
TEST_F(ScheduleCommandClassTest, MqttCommandScheduleSetStoresPackedNestedCommand)
{
    // Provide all required fields and one nested command using the documented MQTT names.
    const std::string payload = R"({"schedule_id":3,"schedule_id_block":1,"start_year":26,"duration_byte":90,"reports_to_follow":0,"properties1":{"start_month":4,"recurrence_offset":2},"properties2":{"start_day_of_month":9,"recurrence_mode":1},"properties3":{"start_weekday":5},"properties4":{"start_hour":10,"duration_type":2},"properties5":{"start_minute":12,"relative":1},"vg1":[{"cmd_byte":[32,1]}]})";
    ASSERT_EQ(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_set_command(endpoint, payload));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::COMMAND_SCHEDULE_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(3, group.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::schedule_id)).desired<uint8_t>());
    EXPECT_EQ(4, group.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::start_month)).desired<uint8_t>());
    EXPECT_EQ(1, group.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::number_of_cmd_to_follow)).desired<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t>{2, 32, 1}), group.child_by_type(static_cast<attribute_store_type_t>(command_schedule_set_group_attributes_t::vg1)).desired<std::vector<uint8_t>>());
    EXPECT_NE(SL_STATUS_OK, command_class_schedule_mqtt::mqtt_on_command_schedule_set_command(endpoint, "{}"));
}
