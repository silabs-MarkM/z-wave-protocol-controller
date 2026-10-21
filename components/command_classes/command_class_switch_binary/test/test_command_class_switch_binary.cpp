/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <gtest/gtest.h>

#include "attribute.hpp"
#include "attribute_store.h"
#include "attribute_store_fixt.h"
#include "command_class_switch_binary.hpp"
#include "command_class_switch_binary_attribute_store.hpp"
#include "command_class_switch_binary_mqtt.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zwave_frame_generator.hpp"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_switch_binary_types;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class SwitchBinaryReportStoreTest : public ::testing::Test
{
  protected:
    static command_class_switch_binary &component()
    {
        static command_class_switch_binary instance;
        return instance;
    }

    void SetUp() override
    {
        ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
        auto root = attribute_store::attribute::root();
        auto home_id = root.emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>{0x12, 0x34, 0x56, 0x78});
        auto node_id = home_id.emplace_node(ATTRIBUTE_NODE_ID, uint16_t{2});
        endpoint = node_id.emplace_node(ATTRIBUTE_ENDPOINT_ID, uint8_t{0});
        endpoint_node_id = attribute_store_get_node_child_by_type(
          attribute_store_get_node_child_by_type(attribute_store_get_node_child_by_type(attribute_store_get_root(), ATTRIBUTE_HOME_ID, 0), ATTRIBUTE_NODE_ID, 0),
          ATTRIBUTE_ENDPOINT_ID,
          0);
    }

    void TearDown() override
    {
        attribute_store_teardown();
    }

    attribute_store::attribute endpoint;
    attribute_store_node_t endpoint_node_id = ATTRIBUTE_STORE_INVALID_NODE;
};

// CC:0025.01.03.11.001, CC:0025.02.03.11.001
// Verifies reports store the supported Off, Unknown, and On current values.
TEST_F(SwitchBinaryReportStoreTest, StoresTheReportedCurrentOnOffValuesForVersions1And2)
{
    // Exercise the three wire values: Off, Unknown, and On.
    for (const uint8_t value : {uint8_t{0}, uint8_t{254}, uint8_t{255}}) {
        command_class_switch_binary_attribute_map_t report {
            {"current_value", value},
        };

        ASSERT_EQ(SL_STATUS_OK, component().on_switch_binary_report_received_store(endpoint, report));
        auto report_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));
        const auto current_value = report_group.emplace_node(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::current_value));

        ASSERT_TRUE(current_value.reported_exists());
        EXPECT_EQ(value, current_value.reported<switch_binary_report_current_value_t>());
    }
}

// CC:0025.02.03.11.003, CC:0025.02.03.12.001
// Verifies version 2 reports preserve their target value and transition duration.
TEST_F(SwitchBinaryReportStoreTest, StoresVersion2TargetAndDuration)
{
    // Version 2 reports include both the requested target and remaining transition time.
    command_class_switch_binary_attribute_map_t report {
        {"current_value", uint8_t{0}},
        {"target_value", uint8_t{255}},
        {"duration", uint8_t{17}},
    };

    ASSERT_EQ(SL_STATUS_OK, component().on_switch_binary_report_received_store(endpoint, report));
    auto report_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));
    auto target_value = report_group.emplace_node(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::target_value));
    auto duration = report_group.emplace_node(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::duration));

    ASSERT_TRUE(target_value.reported_exists());
    ASSERT_TRUE(duration.reported_exists());
    EXPECT_EQ(255, target_value.reported<switch_binary_report_target_value_t>());
    EXPECT_EQ(17, duration.reported<switch_binary_report_duration_t>());
}

// CC:0025.01.01.11.001, CC:0025.02.01.11.001, CC:0025.02.01.12.001
// Verifies version 1 Set frames contain only the target and version 2 frames also contain duration.
TEST_F(SwitchBinaryReportStoreTest, SetFrameUsesTheFieldsDefinedByTheSupportedVersion)
{
    // A Set without its required target must remain unready.
    auto &cc = component();
    auto set_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::SWITCH_BINARY_SET_GROUP));
    auto target_value = set_group.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::target_value));
    target_value.set_desired(uint8_t{255});

    uint8_t data[4] = {};
    uint16_t length = 0;
    zwave_frame_generator frame_generator{0x25};
    frame_generator.initialize_frame(0x01, data, &length);
    command_class_switch_binary_core::set_requested_args args{set_group, &frame_generator};

    auto empty_set_group = endpoint.add_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::SWITCH_BINARY_SET_GROUP));
    command_class_switch_binary_core::set_requested_args empty_args{empty_set_group, &frame_generator};
    frame_generator.initialize_frame(0x01, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, cc.on_switch_binary_set_requested(empty_args, data, &length));

    auto version = endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_switch_binary_attributes_t::supported_version));
    version.set_reported(uint8_t{1});
    // Version 1 sends the target only, even when no duration is available.
    ASSERT_EQ(SL_STATUS_OK, cc.on_switch_binary_set_requested(args, data, &length));
    EXPECT_EQ(3, length);
    EXPECT_EQ(0x25, data[0]);
    EXPECT_EQ(0x01, data[1]);
    EXPECT_EQ(255, data[2]);

    version.set_reported(uint8_t{2});
    // Version 2 also requires duration before the frame can be generated.
    frame_generator.initialize_frame(0x01, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, cc.on_switch_binary_set_requested(args, data, &length));
    auto duration = set_group.emplace_node(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::duration));
    duration.set_desired(uint8_t{17});
    frame_generator.initialize_frame(0x01, data, &length);
    ASSERT_EQ(SL_STATUS_OK, cc.on_switch_binary_set_requested(args, data, &length));
    EXPECT_EQ(4, length);
    EXPECT_EQ(17, data[3]);
}

// CC:0025.02.03.11.001
// Verifies interview initializes the report requirement and starts a Get resolution for a supported endpoint.
TEST_F(SwitchBinaryReportStoreTest, InterviewCreatesReportAndGetGroups)
{
    // Mark Switch Binary v2 as supported, then deliver the normal interview-complete event.
    (void)component();
    endpoint.emplace_node(ATTRIBUTE_ZWAVE_NIF, std::vector<uint8_t>{0x25});
    auto version = endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_switch_binary_attributes_t::supported_version));
    version.set_reported(uint8_t{2});

    component_connector connector;
    const component_connector_interview_done_payload_t payload{endpoint_node_id, SL_STATUS_OK};
    auto status = connector.fire_event_async(
      static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), payload);
    connector.run();
    ASSERT_EQ(SL_STATUS_OK, status.get());
    // Interview registers the report value as required and queues a Get resolution.
    const auto report_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::SWITCH_BINARY_REPORT_GROUP));
    ASSERT_TRUE(report_group.is_valid());
    EXPECT_TRUE(report_group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_report_group_attributes_t::current_value)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_get_group_attributes_t::SWITCH_BINARY_GET_GROUP)).is_valid());
}

// CC:0025.01.02.11.001, CC:0025.02.00.21.002
// Verifies MQTT Set rejects a missing duration and stores both required version 2 fields.
TEST_F(SwitchBinaryReportStoreTest, MqttSetParsesRequiredFieldsAndRejectsMissingFields)
{
    (void)component();
    // A v2 Set needs both target_value and duration.
    EXPECT_NE(SL_STATUS_OK,
              command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":255})"));

    EXPECT_EQ(SL_STATUS_OK,
              command_class_switch_binary_mqtt::mqtt_on_switch_binary_set_command(endpoint, R"({"target_value":255,"duration":17})"));
    const auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::SWITCH_BINARY_SET_GROUP));
    ASSERT_TRUE(group.is_valid());
    const auto target_value = group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::target_value));
    const auto duration = group.child_by_type(static_cast<attribute_store_type_t>(switch_binary_set_group_attributes_t::duration));
    ASSERT_TRUE(target_value.desired_exists());
    ASSERT_TRUE(duration.desired_exists());
    EXPECT_EQ(255, target_value.desired<switch_binary_set_target_value_t>());
    EXPECT_EQ(17, duration.desired<switch_binary_set_duration_t>());
}

// CC:0025.01.02.11.001, CC:0025.02.00.21.002
// Verifies MQTT Get creates a Switch Binary Get resolution group.
TEST_F(SwitchBinaryReportStoreTest, MqttGetCreatesAGetResolutionGroup)
{
    (void)component();
    // An empty MQTT Get command requests the current Switch Binary value.
    EXPECT_EQ(SL_STATUS_OK, command_class_switch_binary_mqtt::mqtt_on_switch_binary_get_command(endpoint, "{}"));
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(switch_binary_get_group_attributes_t::SWITCH_BINARY_GET_GROUP)).is_valid());
}

