## COMMAND_CLASS_THERMOSTAT_MODE MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [THERMOSTAT_MODE_GET](#thermostat_mode_get)
- [THERMOSTAT_MODE_REPORT](#thermostat_mode_report)
- [THERMOSTAT_MODE_SET](#thermostat_mode_set)
- [THERMOSTAT_MODE_SUPPORTED_GET](#thermostat_mode_supported_get)
- [THERMOSTAT_MODE_SUPPORTED_REPORT](#thermostat_mode_supported_report)

### THERMOSTAT_MODE_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatMode/Command/ThermostatModeGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### THERMOSTAT_MODE_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatMode/Report/ThermostatModeReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "level": {
    "mode": "0x01",
    "no_of_manufacturer_data_fields": "0x01"
  },
  "manufacturer_data": [
    "0x01"
  ]
}
```

### THERMOSTAT_MODE_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatMode/Command/ThermostatModeSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "level": {
    "mode": "0x01",
    "no_of_manufacturer_data_fields": "0x01"
  },
  "manufacturer_data": [
    "0x01"
  ]
}
```

### THERMOSTAT_MODE_SUPPORTED_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatMode/Command/ThermostatModeSupportedGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### THERMOSTAT_MODE_SUPPORTED_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatMode/Report/ThermostatModeSupportedReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "bit_mask": "0x01"
}
```
