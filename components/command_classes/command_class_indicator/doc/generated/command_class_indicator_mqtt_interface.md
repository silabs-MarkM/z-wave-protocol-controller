## COMMAND_CLASS_INDICATOR MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [INDICATOR_GET](#indicator_get)
- [INDICATOR_REPORT](#indicator_report)
- [INDICATOR_SET](#indicator_set)
- [INDICATOR_SUPPORTED_GET](#indicator_supported_get)
- [INDICATOR_SUPPORTED_REPORT](#indicator_supported_report)
- [INDICATOR_DESCRIPTION_GET](#indicator_description_get)
- [INDICATOR_DESCRIPTION_REPORT](#indicator_description_report)

### INDICATOR_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Command/IndicatorGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "indicator_id": "0x01"
}
```

### INDICATOR_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Report/IndicatorReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "indicator_0_value": "0x01",
  "properties1": {
    "indicator_object_count": "0x01"
  },
  "vg1": [
    {
      "indicator_id": "0x01",
      "property_id": "0x01",
      "value": "0x01"
    }
  ]
}
```

### INDICATOR_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Command/IndicatorSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "indicator_0_value": "0x01",
  "properties1": {
    "indicator_object_count": "0x01"
  },
  "vg1": [
    {
      "indicator_id": "0x01",
      "property_id": "0x01",
      "value": "0x01"
    }
  ]
}
```

### INDICATOR_SUPPORTED_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Command/IndicatorSupportedGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "indicator_id": "0x01"
}
```

### INDICATOR_SUPPORTED_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Report/IndicatorSupportedReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "indicator_id": "0x01",
  "next_indicator_id": "0x01",
  "properties1": {
    "property_supported_bit_mask_length": "0x01"
  },
  "property_supported_bit_mask": "0x01"
}
```

### INDICATOR_DESCRIPTION_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Command/IndicatorDescriptionGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "indicator_id": "0x01"
}
```

### INDICATOR_DESCRIPTION_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Report/IndicatorDescriptionReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "indicator_id": "0x01",
  "description_length": "0x01",
  "description": [
    "0x01"
  ]
}
```
