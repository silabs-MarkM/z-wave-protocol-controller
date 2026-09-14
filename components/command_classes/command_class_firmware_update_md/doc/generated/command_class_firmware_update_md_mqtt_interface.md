## COMMAND_CLASS_FIRMWARE_UPDATE_MD MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | true |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [FIRMWARE_MD_REPORT](#firmware_md_report)
- [FIRMWARE_UPDATE_MD_REQUEST_REPORT](#firmware_update_md_request_report)
- [FIRMWARE_UPDATE_MD_STATUS_REPORT](#firmware_update_md_status_report)
- [FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT](#firmware_update_activation_status_report)
- [FIRMWARE_UPDATE_MD_PREPARE_REPORT](#firmware_update_md_prepare_report)

### FIRMWARE_MD_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/FirmwareUpdateMd/Report/FirmwareMdReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "manufacturer_id": "0x01",
  "firmware_0_id": "0x01",
  "firmware_0_checksum": "0x01",
  "firmware_upgradable": "0x01",
  "number_of_firmware_targets": "0x01",
  "max_fragment_size": "0x01",
  "vg1": [
    {
      "firmware_id": "0x01"
    }
  ],
  "hardware_version": "0x01"
}
```

### FIRMWARE_UPDATE_MD_REQUEST_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/FirmwareUpdateMd/Report/FirmwareUpdateMdRequestReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "status": "0x01"
}
```

### FIRMWARE_UPDATE_MD_STATUS_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/FirmwareUpdateMd/Report/FirmwareUpdateMdStatusReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "status": "0x01",
  "waittime": "0x01"
}
```

### FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/FirmwareUpdateMd/Report/FirmwareUpdateActivationStatusReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "manufacturer_id": "0x01",
  "firmware_id": "0x01",
  "checksum": "0x01",
  "firmware_target": "0x01",
  "firmware_update_status": "0x01",
  "hardware_version": "0x01"
}
```

### FIRMWARE_UPDATE_MD_PREPARE_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/FirmwareUpdateMd/Report/FirmwareUpdateMdPrepareReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "status": "0x01",
  "firmware_checksum": "0x01"
}
```
