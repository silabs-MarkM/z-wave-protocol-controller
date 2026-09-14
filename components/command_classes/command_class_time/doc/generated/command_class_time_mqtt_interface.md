## COMMAND_CLASS_TIME MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | false |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [DATE_REPORT](#date_report)
- [TIME_OFFSET_REPORT](#time_offset_report)
- [TIME_REPORT](#time_report)

### DATE_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Report/DateReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "year": "0x01",
  "month": "0x01",
  "day": "0x01"
}
```

### TIME_OFFSET_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Report/TimeOffsetReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "level": {
    "hour_tzo": "0x01",
    "sign_tzo": "0x01"
  },
  "minute_tzo": "0x01",
  "level2": {
    "minute_offset_dst": "0x01",
    "sign_offset_dst": "0x01"
  },
  "month_start_dst": "0x01",
  "day_start_dst": "0x01",
  "hour_start_dst": "0x01",
  "month_end_dst": "0x01",
  "day_end_dst": "0x01",
  "hour_end_dst": "0x01"
}
```

### TIME_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Report/TimeReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "hour_local_time": "0x01",
    "time_source": "0x01",
    "rtc_failure": "0x01"
  },
  "minute_local_time": "0x01",
  "second_local_time": "0x01"
}
```
