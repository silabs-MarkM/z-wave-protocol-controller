## COMMAND_CLASS_THERMOSTAT_SETPOINT MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [THERMOSTAT_SETPOINT_GET](#thermostat_setpoint_get)
- [THERMOSTAT_SETPOINT_REPORT](#thermostat_setpoint_report)
- [THERMOSTAT_SETPOINT_SET](#thermostat_setpoint_set)
- [THERMOSTAT_SETPOINT_SUPPORTED_GET](#thermostat_setpoint_supported_get)
- [THERMOSTAT_SETPOINT_SUPPORTED_REPORT](#thermostat_setpoint_supported_report)
- [THERMOSTAT_SETPOINT_CAPABILITIES_GET](#thermostat_setpoint_capabilities_get)
- [THERMOSTAT_SETPOINT_CAPABILITIES_REPORT](#thermostat_setpoint_capabilities_report)

### THERMOSTAT_SETPOINT_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Command/ThermostatSetpointGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "level": {
    "setpoint_type": "0x01"
  }
}
```

### THERMOSTAT_SETPOINT_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Report/ThermostatSetpointReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "level": {
    "setpoint_type": "0x01"
  },
  "level2": {
    "size": "0x01",
    "scale": "0x01",
    "precision": "0x01"
  },
  "value": [
    "0x01"
  ]
}
```

### THERMOSTAT_SETPOINT_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Command/ThermostatSetpointSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "level": {
    "setpoint_type": "0x01"
  },
  "level2": {
    "size": "0x01",
    "scale": "0x01",
    "precision": "0x01"
  },
  "value": [
    "0x01"
  ]
}
```

### THERMOSTAT_SETPOINT_SUPPORTED_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Command/ThermostatSetpointSupportedGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### THERMOSTAT_SETPOINT_SUPPORTED_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Report/ThermostatSetpointSupportedReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "bit_mask": "0x01"
}
```

### THERMOSTAT_SETPOINT_CAPABILITIES_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Command/ThermostatSetpointCapabilitiesGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "setpoint_type": "0x01"
  }
}
```

### THERMOSTAT_SETPOINT_CAPABILITIES_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Report/ThermostatSetpointCapabilitiesReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "setpoint_type": "0x01"
  },
  "properties2": {
    "size1": "0x01",
    "scale1": "0x01",
    "precision1": "0x01"
  },
  "min_value": [
    "0x01"
  ],
  "properties3": {
    "size2": "0x01",
    "scale2": "0x01",
    "precision2": "0x01"
  },
  "maxvalue": [
    "0x01"
  ]
}
```
