## COMMAND_CLASS_DOOR_LOCK MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [DOOR_LOCK_CONFIGURATION_GET](#door_lock_configuration_get)
- [DOOR_LOCK_CONFIGURATION_REPORT](#door_lock_configuration_report)
- [DOOR_LOCK_CONFIGURATION_SET](#door_lock_configuration_set)
- [DOOR_LOCK_OPERATION_GET](#door_lock_operation_get)
- [DOOR_LOCK_OPERATION_REPORT](#door_lock_operation_report)
- [DOOR_LOCK_OPERATION_SET](#door_lock_operation_set)
- [DOOR_LOCK_CAPABILITIES_GET](#door_lock_capabilities_get)
- [DOOR_LOCK_CAPABILITIES_REPORT](#door_lock_capabilities_report)

### DOOR_LOCK_CONFIGURATION_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Command/DoorLockConfigurationGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### DOOR_LOCK_CONFIGURATION_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Report/DoorLockConfigurationReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "operation_type": "0x01",
  "properties1": {
    "inside_door_handles_enabled": "0x01",
    "outside_door_handles_enabled": "0x01"
  },
  "lock_timeout_minutes": "0x01",
  "lock_timeout_seconds": "0x01",
  "auto_relock_time": "0x01",
  "hold_and_release_time": "0x01",
  "properties2": {
    "ta": "0x01",
    "btb": "0x01",
    "reserved1": "0x01"
  }
}
```

### DOOR_LOCK_CONFIGURATION_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Command/DoorLockConfigurationSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "operation_type": "0x01",
  "properties1": {
    "inside_door_handles_enabled": "0x01",
    "outside_door_handles_enabled": "0x01"
  },
  "lock_timeout_minutes": "0x01",
  "lock_timeout_seconds": "0x01",
  "auto_relock_time": "0x01",
  "hold_and_release_time": "0x01",
  "properties2": {
    "ta": "0x01",
    "btb": "0x01",
    "reserved1": "0x01"
  }
}
```

### DOOR_LOCK_OPERATION_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Command/DoorLockOperationGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### DOOR_LOCK_OPERATION_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Report/DoorLockOperationReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "current_door_lock_mode": "0x01",
  "properties1": {
    "inside_door_handles_mode": "0x01",
    "outside_door_handles_mode": "0x01"
  },
  "door_condition": "0x01",
  "remaining_lock_time_minutes": "0x01",
  "remaining_lock_time_seconds": "0x01",
  "target_door_lock_mode": "0x01",
  "duration": "0x01"
}
```

### DOOR_LOCK_OPERATION_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Command/DoorLockOperationSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "door_lock_mode": "0x01"
}
```

### DOOR_LOCK_CAPABILITIES_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Command/DoorLockCapabilitiesGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### DOOR_LOCK_CAPABILITIES_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Report/DoorLockCapabilitiesReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "supported_operation_type_bit_mask_length": "0x01"
  },
  "supported_operation_type_bit_mask": [
    "0x01"
  ],
  "supported_door_lock_mode_list_length": "0x01",
  "supported_door_lock_mode": [
    "0x01"
  ],
  "properties2": {
    "supported_inside_handle_modes_bitmask": "0x01",
    "supported_outside_handle_modes_bitmask": "0x01"
  },
  "supported_door_components": "0x01",
  "properties3": {
    "btbs": "0x01",
    "tas": "0x01",
    "hrs": "0x01",
    "ars": "0x01"
  }
}
```
