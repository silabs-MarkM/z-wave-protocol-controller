## COMMAND_CLASS_DEVICE_RESET_LOCALLY MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | true |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [DEVICE_RESET_LOCALLY_NOTIFICATION](#device_reset_locally_notification)

### DEVICE_RESET_LOCALLY_NOTIFICATION

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/DeviceResetLocally/Report/DeviceResetLocallyNotification`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{}
```
