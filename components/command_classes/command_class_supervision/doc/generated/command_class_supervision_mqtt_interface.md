## COMMAND_CLASS_SUPERVISION MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | true |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [SUPERVISION_REPORT](#supervision_report)

### SUPERVISION_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Supervision/Report/SupervisionReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "session_id": "0x01",
    "wake_up_request": "0x01",
    "more_status_updates": "0x01"
  },
  "status": "0x01",
  "duration": "0x01"
}
```
