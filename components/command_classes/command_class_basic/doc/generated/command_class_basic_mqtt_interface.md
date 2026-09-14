## COMMAND_CLASS_BASIC MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [BASIC_GET](#basic_get)
- [BASIC_REPORT](#basic_report)
- [BASIC_SET](#basic_set)

### BASIC_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Basic/Command/BasicGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### BASIC_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Basic/Report/BasicReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "current_value": "0x01",
  "target_value": "0x01",
  "duration": "0x01"
}
```

### BASIC_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Basic/Command/BasicSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "value": "0x01"
}
```
