## COMMAND_CLASS_VERSION MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [VERSION_COMMAND_CLASS_GET](#version_command_class_get)
- [VERSION_COMMAND_CLASS_REPORT](#version_command_class_report)
- [VERSION_GET](#version_get)
- [VERSION_REPORT](#version_report)
- [VERSION_CAPABILITIES_GET](#version_capabilities_get)
- [VERSION_CAPABILITIES_REPORT](#version_capabilities_report)
- [VERSION_ZWAVE_SOFTWARE_GET](#version_zwave_software_get)
- [VERSION_ZWAVE_SOFTWARE_REPORT](#version_zwave_software_report)

### VERSION_COMMAND_CLASS_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Command/VersionCommandClassGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "requested_command_class": "0x01"
}
```

### VERSION_COMMAND_CLASS_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Report/VersionCommandClassReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "requested_command_class": "0x01",
  "command_class_version": "0x01"
}
```

### VERSION_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Command/VersionGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### VERSION_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Report/VersionReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "z_wave_library_type": "0x01",
  "z_wave_protocol_version": "0x01",
  "z_wave_protocol_sub_version": "0x01",
  "firmware_0_version": "0x01",
  "firmware_0_sub_version": "0x01",
  "hardware_version": "0x01",
  "number_of_firmware_targets": "0x01",
  "vg": [
    {
      "firmware_version": "0x01",
      "firmware_sub_version": "0x01"
    }
  ]
}
```

### VERSION_CAPABILITIES_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Command/VersionCapabilitiesGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### VERSION_CAPABILITIES_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Report/VersionCapabilitiesReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "version": "0x01",
    "command_class": "0x01",
    "z_wave_software": "0x01",
    "reserved1": "0x01"
  }
}
```

### VERSION_ZWAVE_SOFTWARE_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Command/VersionZwaveSoftwareGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### VERSION_ZWAVE_SOFTWARE_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Report/VersionZwaveSoftwareReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "sdk_version": "0x01",
  "application_framework_api_version": "0x01",
  "application_framework_build_number": "0x01",
  "host_interface_version": "0x01",
  "host_interface_build_number": "0x01",
  "z_wave_protocol_version": "0x01",
  "z_wave_protocol_build_number": "0x01",
  "application_version": "0x01",
  "application_build_number": "0x01"
}
```
