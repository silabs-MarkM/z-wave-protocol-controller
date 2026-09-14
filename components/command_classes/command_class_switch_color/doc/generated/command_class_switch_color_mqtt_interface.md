## COMMAND_CLASS_SWITCH_COLOR MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [SWITCH_COLOR_SUPPORTED_GET](#switch_color_supported_get)
- [SWITCH_COLOR_SUPPORTED_REPORT](#switch_color_supported_report)
- [SWITCH_COLOR_GET](#switch_color_get)
- [SWITCH_COLOR_REPORT](#switch_color_report)
- [SWITCH_COLOR_SET](#switch_color_set)
- [SWITCH_COLOR_START_LEVEL_CHANGE](#switch_color_start_level_change)
- [SWITCH_COLOR_STOP_LEVEL_CHANGE](#switch_color_stop_level_change)

### SWITCH_COLOR_SUPPORTED_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Command/SwitchColorSupportedGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### SWITCH_COLOR_SUPPORTED_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Report/SwitchColorSupportedReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "color_component_mask": "0x01"
}
```

### SWITCH_COLOR_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Command/SwitchColorGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "color_component_id": "0x01"
}
```

### SWITCH_COLOR_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Report/SwitchColorReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "color_component_id": "0x01",
  "current_value": "0x01",
  "target_value": "0x01",
  "duration": "0x01"
}
```

### SWITCH_COLOR_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Command/SwitchColorSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "color_component_count": "0x01"
  },
  "vg1": [
    {
      "color_component_id": "0x01",
      "value": "0x01"
    }
  ],
  "duration": "0x01"
}
```

### SWITCH_COLOR_START_LEVEL_CHANGE

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Command/SwitchColorStartLevelChange`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "ignore_start_state": "0x01",
    "up_down": "0x01"
  },
  "color_component_id": "0x01",
  "start_level": "0x01",
  "duration": "0x01"
}
```

### SWITCH_COLOR_STOP_LEVEL_CHANGE

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Command/SwitchColorStopLevelChange`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "color_component_id": "0x01"
}
```
