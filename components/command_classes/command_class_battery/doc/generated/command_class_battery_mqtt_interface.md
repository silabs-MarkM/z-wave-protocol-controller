## COMMAND_CLASS_BATTERY MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [BATTERY_GET](#battery_get)
- [BATTERY_REPORT](#battery_report)
- [BATTERY_HEALTH_GET](#battery_health_get)
- [BATTERY_HEALTH_REPORT](#battery_health_report)

### BATTERY_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Battery/Command/BatteryGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### BATTERY_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Battery/Report/BatteryReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "battery_level": "0x01",
  "properties1": {
    "replace_recharge": "0x01",
    "low_fluid": "0x01",
    "overheating": "0x01",
    "backup_battery": "0x01",
    "rechargeable": "0x01",
    "charging_status": "0x01"
  },
  "properties2": {
    "disconnected": "0x01",
    "low_temperature_status": "0x01",
    "reserved1": "0x01"
  }
}
```

### BATTERY_HEALTH_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Battery/Command/BatteryHealthGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### BATTERY_HEALTH_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Battery/Report/BatteryHealthReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "maximum_capacity": "0x01",
  "properties1": {
    "size": "0x01",
    "scale": "0x01",
    "precision": "0x01"
  },
  "battery_temperature": [
    "0x01"
  ]
}
```
