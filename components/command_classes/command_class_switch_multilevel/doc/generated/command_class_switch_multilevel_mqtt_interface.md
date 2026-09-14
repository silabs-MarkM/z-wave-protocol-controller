## COMMAND_CLASS_SWITCH_MULTILEVEL MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [SWITCH_MULTILEVEL_GET](#switch_multilevel_get)
- [SWITCH_MULTILEVEL_REPORT](#switch_multilevel_report)
- [SWITCH_MULTILEVEL_SET](#switch_multilevel_set)
- [SWITCH_MULTILEVEL_START_LEVEL_CHANGE](#switch_multilevel_start_level_change)
- [SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE](#switch_multilevel_stop_level_change)
- [SWITCH_MULTILEVEL_SUPPORTED_GET](#switch_multilevel_supported_get)
- [SWITCH_MULTILEVEL_SUPPORTED_REPORT](#switch_multilevel_supported_report)

### SWITCH_MULTILEVEL_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### SWITCH_MULTILEVEL_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Report/SwitchMultilevelReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "current_value": "0x01",
  "target_value": "0x01",
  "duration": "0x01"
}
```

### SWITCH_MULTILEVEL_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "value": "0x01",
  "duration": "0x01"
}
```

### SWITCH_MULTILEVEL_START_LEVEL_CHANGE

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelStartLevelChange`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "inc_dec": "0x01",
    "ignore_start_level": "0x01",
    "up_down": "0x01"
  },
  "start_level": "0x01",
  "dimming_duration": "0x01",
  "step_size": "0x01"
}
```

### SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelStopLevelChange`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### SWITCH_MULTILEVEL_SUPPORTED_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelSupportedGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### SWITCH_MULTILEVEL_SUPPORTED_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Report/SwitchMultilevelSupportedReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "primary_switch_type": "0x01",
    "reserved1": "0x01"
  },
  "properties2": {
    "secondary_switch_type": "0x01",
    "reserved2": "0x01"
  }
}
```
