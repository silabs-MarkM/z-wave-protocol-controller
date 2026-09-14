## COMMAND_CLASS_POWERLEVEL MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | false |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [POWERLEVEL_REPORT](#powerlevel_report)
- [POWERLEVEL_TEST_NODE_REPORT](#powerlevel_test_node_report)

### POWERLEVEL_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Powerlevel/Report/PowerlevelReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "power_level": "0x01",
  "timeout": "0x01"
}
```

### POWERLEVEL_TEST_NODE_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Powerlevel/Report/PowerlevelTestNodeReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "test_nodeid": "0x01",
  "status_of_operation": "0x01",
  "test_frame_count": "0x01"
}
```
