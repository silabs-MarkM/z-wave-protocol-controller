## COMMAND_CLASS_THERMOSTAT_FAN_MODE MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [THERMOSTAT_FAN_MODE_GET](#thermostat_fan_mode_get)
- [THERMOSTAT_FAN_MODE_REPORT](#thermostat_fan_mode_report)
- [THERMOSTAT_FAN_MODE_SET](#thermostat_fan_mode_set)
- [THERMOSTAT_FAN_MODE_SUPPORTED_GET](#thermostat_fan_mode_supported_get)
- [THERMOSTAT_FAN_MODE_SUPPORTED_REPORT](#thermostat_fan_mode_supported_report)

### THERMOSTAT_FAN_MODE_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanMode/Command/ThermostatFanModeGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### THERMOSTAT_FAN_MODE_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanMode/Report/ThermostatFanModeReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "fan_mode": "0x01",
    "off": "0x01"
  }
}
```

### THERMOSTAT_FAN_MODE_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanMode/Command/ThermostatFanModeSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "fan_mode": "0x01",
    "off": "0x01"
  }
}
```

### THERMOSTAT_FAN_MODE_SUPPORTED_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanMode/Command/ThermostatFanModeSupportedGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### THERMOSTAT_FAN_MODE_SUPPORTED_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanMode/Report/ThermostatFanModeSupportedReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "bit_mask": "0x01"
}
```
