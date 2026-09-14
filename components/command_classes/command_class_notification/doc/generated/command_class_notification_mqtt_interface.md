## COMMAND_CLASS_NOTIFICATION MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [NOTIFICATION_GET](#notification_get)
- [NOTIFICATION_REPORT](#notification_report)
- [NOTIFICATION_SET](#notification_set)
- [NOTIFICATION_SUPPORTED_GET](#notification_supported_get)
- [NOTIFICATION_SUPPORTED_REPORT](#notification_supported_report)
- [EVENT_SUPPORTED_GET](#event_supported_get)
- [EVENT_SUPPORTED_REPORT](#event_supported_report)

### NOTIFICATION_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Command/NotificationGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "v1_alarm_type": "0x01",
  "notification_type": "0x01",
  "event": "0x01"
}
```

### NOTIFICATION_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Report/NotificationReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "v1_alarm_type": "0x01",
  "v1_alarm_level": "0x01",
  "notification_status": "0x01",
  "notification_type": "0x01",
  "event": "0x01",
  "properties1": {
    "event_parameters_length": "0x01",
    "reserved2": "0x01",
    "sequence": "0x01"
  },
  "event_parameter": [
    "0x01"
  ],
  "sequence_number": "0x01"
}
```

### NOTIFICATION_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Command/NotificationSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "notification_type": "0x01",
  "notification_status": "0x01"
}
```

### NOTIFICATION_SUPPORTED_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Command/NotificationSupportedGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### NOTIFICATION_SUPPORTED_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Report/NotificationSupportedReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "number_of_bit_masks": "0x01",
    "v1_alarm": "0x01"
  },
  "bit_mask": "0x01"
}
```

### EVENT_SUPPORTED_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Command/EventSupportedGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "notification_type": "0x01"
}
```

### EVENT_SUPPORTED_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Report/EventSupportedReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "notification_type": "0x01",
  "properties1": {
    "number_of_bit_masks": "0x01"
  },
  "bit_mask": "0x01"
}
```
