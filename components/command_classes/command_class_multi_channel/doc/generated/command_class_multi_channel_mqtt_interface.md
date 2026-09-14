## COMMAND_CLASS_MULTI_CHANNEL MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [MULTI_CHANNEL_CAPABILITY_GET](#multi_channel_capability_get)
- [MULTI_CHANNEL_CAPABILITY_REPORT](#multi_channel_capability_report)
- [MULTI_CHANNEL_CMD_ENCAP](#multi_channel_cmd_encap)
- [MULTI_CHANNEL_END_POINT_FIND](#multi_channel_end_point_find)
- [MULTI_CHANNEL_END_POINT_FIND_REPORT](#multi_channel_end_point_find_report)
- [MULTI_CHANNEL_END_POINT_GET](#multi_channel_end_point_get)
- [MULTI_CHANNEL_END_POINT_REPORT](#multi_channel_end_point_report)
- [MULTI_INSTANCE_CMD_ENCAP](#multi_instance_cmd_encap)
- [MULTI_INSTANCE_GET](#multi_instance_get)
- [MULTI_INSTANCE_REPORT](#multi_instance_report)
- [MULTI_CHANNEL_AGGREGATED_MEMBERS_GET](#multi_channel_aggregated_members_get)
- [MULTI_CHANNEL_AGGREGATED_MEMBERS_REPORT](#multi_channel_aggregated_members_report)

### MULTI_CHANNEL_CAPABILITY_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiChannelCapabilityGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "end_point": "0x01"
  }
}
```

### MULTI_CHANNEL_CAPABILITY_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Report/MultiChannelCapabilityReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "end_point": "0x01",
    "dynamic": "0x01"
  },
  "generic_device_class": "0x01",
  "specific_device_class": "0x01",
  "command_class": [
    "0x01"
  ]
}
```

### MULTI_CHANNEL_CMD_ENCAP

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiChannelCmdEncap`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "source_end_point": "0x01"
  },
  "properties2": {
    "destination_end_point": "0x01",
    "bit_address": "0x01"
  },
  "command_class": "0x01",
  "command": "0x01",
  "parameter": [
    "0x01"
  ]
}
```

### MULTI_CHANNEL_END_POINT_FIND

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiChannelEndPointFind`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "generic_device_class": "0x01",
  "specific_device_class": "0x01"
}
```

### MULTI_CHANNEL_END_POINT_FIND_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Report/MultiChannelEndPointFindReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "reports_to_follow": "0x01",
  "generic_device_class": "0x01",
  "specific_device_class": "0x01",
  "vg": [
    {
      "properties1": {
        "end_point": "0x01"
      }
    }
  ]
}
```

### MULTI_CHANNEL_END_POINT_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiChannelEndPointGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### MULTI_CHANNEL_END_POINT_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Report/MultiChannelEndPointReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "identical": "0x01",
    "dynamic": "0x01"
  },
  "properties2": {
    "individual_end_points": "0x01"
  },
  "properties3": {
    "aggregated_end_points": "0x01"
  }
}
```

### MULTI_INSTANCE_CMD_ENCAP

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiInstanceCmdEncap`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "instance": "0x01"
  },
  "command_class": "0x01",
  "command": "0x01",
  "parameter": [
    "0x01"
  ]
}
```

### MULTI_INSTANCE_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiInstanceGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "command_class": "0x01"
}
```

### MULTI_INSTANCE_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Report/MultiInstanceReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "command_class": "0x01",
  "properties1": {
    "instances": "0x01"
  }
}
```

### MULTI_CHANNEL_AGGREGATED_MEMBERS_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiChannelAggregatedMembersGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "aggregated_end_point": "0x01"
  }
}
```

### MULTI_CHANNEL_AGGREGATED_MEMBERS_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Report/MultiChannelAggregatedMembersReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "aggregated_end_point": "0x01"
  },
  "number_of_bit_masks": "0x01",
  "aggregated_members_bit_mask": "0x01"
}
```
