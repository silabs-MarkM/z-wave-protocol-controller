## COMMAND_CLASS_WAKE_UP MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [WAKE_UP_INTERVAL_CAPABILITIES_GET](#wake_up_interval_capabilities_get)
- [WAKE_UP_INTERVAL_CAPABILITIES_REPORT](#wake_up_interval_capabilities_report)
- [WAKE_UP_INTERVAL_GET](#wake_up_interval_get)
- [WAKE_UP_INTERVAL_REPORT](#wake_up_interval_report)
- [WAKE_UP_INTERVAL_SET](#wake_up_interval_set)
- [WAKE_UP_NO_MORE_INFORMATION](#wake_up_no_more_information)
- [WAKE_UP_NOTIFICATION](#wake_up_notification)

### WAKE_UP_INTERVAL_CAPABILITIES_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Command/WakeUpIntervalCapabilitiesGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### WAKE_UP_INTERVAL_CAPABILITIES_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Report/WakeUpIntervalCapabilitiesReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "minimum_wake_up_interval_seconds": "0x01",
  "maximum_wake_up_interval_seconds": "0x01",
  "default_wake_up_interval_seconds": "0x01",
  "wake_up_interval_step_seconds": "0x01",
  "properties1": {
    "wake_up_on_demand": "0x01"
  }
}
```

### WAKE_UP_INTERVAL_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Command/WakeUpIntervalGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### WAKE_UP_INTERVAL_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Report/WakeUpIntervalReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "seconds": "0x01",
  "nodeid": "0x01"
}
```

### WAKE_UP_INTERVAL_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Command/WakeUpIntervalSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "seconds": "0x01",
  "nodeid": "0x01"
}
```

### WAKE_UP_NO_MORE_INFORMATION

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Command/WakeUpNoMoreInformation`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### WAKE_UP_NOTIFICATION

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Report/WakeUpNotification`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{}
```
