## COMMAND_CLASS_MANUFACTURER_SPECIFIC MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [MANUFACTURER_SPECIFIC_GET](#manufacturer_specific_get)
- [MANUFACTURER_SPECIFIC_REPORT](#manufacturer_specific_report)
- [DEVICE_SPECIFIC_GET](#device_specific_get)
- [DEVICE_SPECIFIC_REPORT](#device_specific_report)

### MANUFACTURER_SPECIFIC_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ManufacturerSpecific/Command/ManufacturerSpecificGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### MANUFACTURER_SPECIFIC_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ManufacturerSpecific/Report/ManufacturerSpecificReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "manufacturer_id": "0x01",
  "product_type_id": "0x01",
  "product_id": "0x01"
}
```

### DEVICE_SPECIFIC_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ManufacturerSpecific/Command/DeviceSpecificGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "device_id_type": "0x01"
  }
}
```

### DEVICE_SPECIFIC_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/ManufacturerSpecific/Report/DeviceSpecificReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "device_id_type": "0x01"
  },
  "properties2": {
    "device_id_data_length_indicator": "0x01",
    "device_id_data_format": "0x01"
  },
  "device_id_data": [
    "0x01"
  ]
}
```
