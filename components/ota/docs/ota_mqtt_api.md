# OTA MQTT API

<!-- Generated from AsyncAPI; do not edit -->

Firmware update over the air: image management, start/abort, progress, and activation.
Command and report topics are relative to `zpc/{homeId}/`.
Per-node Firmware Update MD topics follow the command-class pattern.

## Table of Contents
- [OTA_UPLOAD_IMAGE](#ota_upload_image)
- [OTA_UPLOAD_IMAGE_REPORT](#ota_upload_image_report)
- [OTA_LIST_IMAGES](#ota_list_images)
- [OTA_LIST_IMAGES_REPORT](#ota_list_images_report)
- [OTA_REMOVE_IMAGE](#ota_remove_image)
- [OTA_REMOVE_IMAGE_REPORT](#ota_remove_image_report)
- [OTA_START_FIRMWARE_UPLOAD](#ota_start_firmware_upload)
- [OTA_START_FIRMWARE_UPLOAD_REPORT](#ota_start_firmware_upload_report)
- [OTA_PROGRESS](#ota_progress)
- [OTA_PROGRESS_REPORT](#ota_progress_report)
- [OTA_ABORT](#ota_abort)
- [OTA_ACTIVATE](#ota_activate)
- [OTA_ACTIVATE_REPORT](#ota_activate_report)

### OTA_UPLOAD_IMAGE

**Topic:** `zpc/<home_id>/OTA/UploadImage`

**Direction:** Command (client → ZPC)

Store a `.gbl` image in the cache (`image_name`, `data` array).

**Payload:**

```json
{
  "image_name": "0x01",
  "data": [
    0
  ]
}
```

### OTA_UPLOAD_IMAGE_REPORT

**Topic:** `zpc/<home_id>/OTA/UploadImage/Report`

**Direction:** Report (ZPC → client)

Store result (`ok` or `error`).

**Payload:**

```json
{
  "status": "0x01",
  "reason": "0x01"
}
```

### OTA_LIST_IMAGES

**Topic:** `zpc/<home_id>/OTA/ListImages`

**Direction:** Command (client → ZPC)

List cached `.gbl` files. Payload ignored.

**Payload:**

```json
{}
```

### OTA_LIST_IMAGES_REPORT

**Topic:** `zpc/<home_id>/OTA/ListImages/Report`

**Direction:** Report (ZPC → client)

Array of cached images (`images`). No `status` field.

**Payload:**

```json
{
  "images": [
    "0x01"
  ]
}
```

### OTA_REMOVE_IMAGE

**Topic:** `zpc/<home_id>/OTA/RemoveImage`

**Direction:** Command (client → ZPC)

Remove a cached image by name.

**Payload:**

```json
{
  "image_name": "0x01"
}
```

### OTA_REMOVE_IMAGE_REPORT

**Topic:** `zpc/<home_id>/OTA/RemoveImage/Report`

**Direction:** Report (ZPC → client)

Remove result (`ok` or `error`).

**Payload:**

```json
{
  "status": "0x01",
  "reason": "0x01"
}
```

### OTA_START_FIRMWARE_UPLOAD

**Topic:** `zpc/<home_id>/OTA/StartFirmwareUpload`

**Direction:** Command (client → ZPC)

Start OTA for a node (`node_id`, `image_name`, `wait_for_activation`).

**Payload:**

```json
{
  "node_id": 0,
  "image_name": "0x01",
  "wait_for_activation": false
}
```

### OTA_START_FIRMWARE_UPLOAD_REPORT

**Topic:** `zpc/<home_id>/OTA/StartFirmwareUpload/Report`

**Direction:** Report (ZPC → client)

Accept, reject, error, or abort.

**Payload:**

```json
{
  "status": "0x01",
  "reason": "0x01"
}
```

### OTA_PROGRESS

**Topic:** `zpc/<home_id>/OTA/Progress`

**Direction:** Command (client → ZPC)

Request a one-shot progress snapshot. Payload ignored.

**Payload:**

```json
{}
```

### OTA_PROGRESS_REPORT

**Topic:** `zpc/<home_id>/OTA/Progress/Report`

**Direction:** Report (ZPC → client)

Progress snapshot and final completion or failure status.

**Payload:**

```json
{
  "node_id": 2,
  "image_size": 65536,
  "current_sent": 32768,
  "percentage": 50
}
```

### OTA_ABORT

**Topic:** `zpc/<home_id>/OTA/Abort`

**Direction:** Command (client → ZPC)

Abort the current transfer. `node_id` is required.

**Payload:**

```json
{
  "node_id": 0
}
```

### OTA_ACTIVATE

**Topic:** `zpc/<home_id>/OTA/Activate`

**Direction:** Command (client → ZPC)

Apply firmware stored in waiting-for-activation (`node_id`).

**Payload:**

```json
{
  "node_id": 0
}
```

### OTA_ACTIVATE_REPORT

**Topic:** `zpc/<home_id>/OTA/Activate/Report`

**Direction:** Report (ZPC → client)

Parse errors on the activation command. Device status travels on OTA/Progress/Report.

**Payload:**

```json
{
  "status": "0x01",
  "reason": "0x01"
}
```

Progress is a request/response pair: publish any payload to `OTA/Progress` (ignored)
and subscribe to `OTA/Progress/Report`. The same report topic is used for completion
and failure. See the OTA Firmware Manager document for the state machine.
