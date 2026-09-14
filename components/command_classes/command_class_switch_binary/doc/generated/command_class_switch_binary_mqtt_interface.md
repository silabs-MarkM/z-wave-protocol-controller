## COMMAND_CLASS_SWITCH_BINARY MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [SWITCH_BINARY_GET](#switch_binary_get)
- [SWITCH_BINARY_REPORT](#switch_binary_report)
- [SWITCH_BINARY_SET](#switch_binary_set)

### SWITCH_BINARY_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchBinary/Command/SwitchBinaryGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### SWITCH_BINARY_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchBinary/Report/SwitchBinaryReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "current_value": "0x01",
  "target_value": "0x01",
  "duration": "0x01"
}
```

### SWITCH_BINARY_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchBinary/Command/SwitchBinarySet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "target_value": "0x01",
  "duration": "0x01"
}
```
