# MQTT API Index

<!-- Generated from AsyncAPI; do not edit -->

This page is generated from the assembled AsyncAPI document. Edit per-component `asyncapi/*.yaml` or `zwave.xml`, not this file.

## Table of Contents

- [Discovery](#discovery)
- [Network Management](#network-management)
- [SmartStart](#smartstart)
- [Device Interview](#device-interview)
- [Network Status](#network-status)
- [OTA (Firmware Update)](#ota-firmware-update)
- [Security Keys Dump](#security-keys-dump)
- [Command Classes](#command-classes)
- [Sequences](#sequences)

## Discovery

The Discovery API allows clients to obtain the Z-Wave Home ID of the controller.

| Topic | Direction | Description |
|-------|-----------|-------------|
| `zpc/Discovery` | Command (client → ZPC) | Publish any message (e.g. empty JSON `{}`) to request the current Home ID. |
| `zpc/Discovery/Report` | Report (ZPC → client) | The `home_id` is the 8-character hexadecimal representation of the Z-Wave Home ID. Use this value to construct networ... |

**Full reference:** [Discovery MQTT API](discovery_mqtt_api.md)

## Network Management

Add/remove nodes, accept DSK, grant keys, list devices, factory reset, and

| Topic | Direction | Description |
|-------|-----------|-------------|
| `zpc/<home_id>/Network/Node/Add` | Command (client → ZPC) | Start inclusion. |
| `zpc/<home_id>/Network/Node/Add/Report` | Report (ZPC → client) | Success includes `node_id` and optional `dsk`. Fail may be immediate rejection (`reason` 6401 busy / 6402 reset) or s... |
| `zpc/<home_id>/Network/Node/Add/Abort` | Command (client → ZPC) | Abort ongoing inclusion. No report is published. |
| `zpc/<home_id>/Network/Node/Remove` | Command (client → ZPC) | Start exclusion. |
| `zpc/<home_id>/Network/Node/Remove/Report` | Report (ZPC → client) | Also fired on successful Remove Failed. `dsk` omitted if unavailable. |
| `zpc/<home_id>/Network/Node/RemoveFailed` | Command (client → ZPC) | Remove a node believed failed. Controller NOP-probes first. |
| `zpc/<home_id>/Network/Node/RemoveFailed/Report` | Report (ZPC → client) | `ok` / `fail` with `reason` (`operation_successful`, `node_online`, …). On success, Remove/Report is also published. |
| `zpc/<home_id>/Network/Node/Remove/Abort` | Command (client → ZPC) | Abort ongoing exclusion. No report is published. |
| `zpc/<home_id>/Network/DSK/Accept` | Command (client → ZPC) | Accept DSK during classic S2 inclusion. Not used for SmartStart. |
| `zpc/<home_id>/Network/RequestedKeys/Report` | Report (ZPC → client) | Keys requested during classic S2 inclusion. Not published for SmartStart. |
| `zpc/<home_id>/Network/GrantKeys` | Command (client → ZPC) | Grant keys during classic S2 inclusion. Not used for SmartStart. |
| `zpc/<home_id>/Network/RequestedDSK/Report` | Report (ZPC → client) | DSK verification needed during classic S2 inclusion. |
| `zpc/<home_id>/Network/Node/List` | Command (client → ZPC) | Request node list. |
| `zpc/<home_id>/Network/Node/List/Report` | Report (ZPC → client) | Array of nodes with `node_information` and `version_report`. |
| `zpc/<home_id>/Network/Node/Properties` | Command (client → ZPC) | Request node properties. |
| `zpc/<home_id>/Network/Node/Properties/Report` | Report (ZPC → client) | Node properties; missing attribute-store values are `null`. |
| `zpc/<home_id>/Network/Node/Interview` | Command (client → ZPC) | Request a (re-)interview for `node_id`. Completion is published on `Interview/Report`, not under this topic. |
| `zpc/<home_id>/Network/FactoryReset` | Command (client → ZPC) | Factory reset the controller. |
| `zpc/Network/FactoryReset/Report` | Report (ZPC → client) | Global topic. Published once when the controller is ready on the new network. Not retained. |
| `zpc/<home_id>/Network/NLS/Enable` | Command (client → ZPC) | Enable NLS for a node. |
| `zpc/<home_id>/Network/NLS/Enable/Report` | Report (ZPC → client) | NLS enable result. |
| `zpc/<home_id>/Network/NLS/State` | Command (client → ZPC) | Request NLS state. |
| `zpc/<home_id>/Network/NLS/State/Report` | Report (ZPC → client) | NLS state report. |

**Full reference:** [Network Management MQTT API](../../network_manager/doc/network_management_mqtt_api.md)

## SmartStart

SmartStart allows pre-provisioned devices to join the network automatically when they

| Topic | Direction | Description |
|-------|-----------|-------------|
| `zpc/<home_id>/Network/SmartStart/Update` | Command (client → ZPC) | Replaces ZPC's entire SmartStart provisioning list with the entries in the payload. Not incremental. For incremental ... |
| `zpc/<home_id>/Network/SmartStart/Add` | Command (client → ZPC) | Appends entries. Existing DSKs are skipped. No dedicated report topic; observe the list via List. |
| `zpc/<home_id>/Network/SmartStart/Remove` | Command (client → ZPC) | Removes entries identified by DSK. Only the DSK field is read. Unknown DSKs are skipped. No dedicated report topic. |
| `zpc/<home_id>/Network/SmartStart/Clear` | Command (client → ZPC) | Purges the entire list. Payload ignored. |
| `zpc/<home_id>/Network/SmartStart/List` | Command (client → ZPC) | Request the current SmartStart list. Payload ignored. |
| `zpc/<home_id>/Network/SmartStart/List/Report` | Report (ZPC → client) | Current provisioning list (`value` array, same shape as Update). |

**Full reference:** [SmartStart MQTT API](smartstart_mqtt_api.md)

## Device Interview

Clients request an on-demand interview via Network Management MQTT

| Topic | Direction | Description |
|-------|-----------|-------------|
| `zpc/<home_id>/Interview/Report` | Report (ZPC → client) | Published when an interview completes for an endpoint—either successfully or after cancellation/failure. One report i... |

**Full reference:** [Device Interview MQTT API](../../device_interviewer/docs/interview_mqtt_api.md)

## Network Status

Unsolicited reports reflecting the availability of Z-Wave nodes

| Topic | Direction | Description |
|-------|-----------|-------------|
| `zpc/<home_id>/Network/Status/Report` | Report (ZPC → client) | Published whenever Network Monitor detects a node availability transition. See the Network Status behavior document f... |

**Full reference:** [Network Status MQTT](../../network_monitor/doc/network_status_mqtt_api.md) and [Network Status](../../network_monitor/doc/network_status.md)

## OTA (Firmware Update)

Firmware update over the air: image management, start/abort, progress, and activation.

| Topic | Direction | Description |
|-------|-----------|-------------|
| `zpc/<home_id>/OTA/UploadImage` | Command (client → ZPC) | Store a `.gbl` image in the cache (`image_name`, `data` array). |
| `zpc/<home_id>/OTA/UploadImage/Report` | Report (ZPC → client) | Store result (`ok` or `error`). |
| `zpc/<home_id>/OTA/ListImages` | Command (client → ZPC) | List cached `.gbl` files. Payload ignored. |
| `zpc/<home_id>/OTA/ListImages/Report` | Report (ZPC → client) | Array of cached images (`images`). No `status` field. |
| `zpc/<home_id>/OTA/RemoveImage` | Command (client → ZPC) | Remove a cached image by name. |
| `zpc/<home_id>/OTA/RemoveImage/Report` | Report (ZPC → client) | Remove result (`ok` or `error`). |
| `zpc/<home_id>/OTA/StartFirmwareUpload` | Command (client → ZPC) | Start OTA for a node (`node_id`, `image_name`, `wait_for_activation`). |
| `zpc/<home_id>/OTA/StartFirmwareUpload/Report` | Report (ZPC → client) | Accept, reject, error, or abort. |
| `zpc/<home_id>/OTA/Progress` | Command (client → ZPC) | Request a one-shot progress snapshot. Payload ignored. |
| `zpc/<home_id>/OTA/Progress/Report` | Report (ZPC → client) | Progress snapshot and final completion or failure status. |
| `zpc/<home_id>/OTA/Abort` | Command (client → ZPC) | Abort the current transfer. `node_id` is required. |
| `zpc/<home_id>/OTA/Activate` | Command (client → ZPC) | Apply firmware stored in waiting-for-activation (`node_id`). |
| `zpc/<home_id>/OTA/Activate/Report` | Report (ZPC → client) | Parse errors on the activation command. Device status travels on OTA/Progress/Report. |

**Full reference:** [OTA MQTT API](../../ota/docs/ota_mqtt_api.md) and [OTA Firmware Manager](../../ota/docs/ota.md)

## Security Keys Dump

On request, ZPC seals its currently-assigned S2 (and S0, if any) keys with a given

| Topic | Direction | Description |
|-------|-----------|-------------|
| `zpc/<home_id>/Network/DumpSecurityKeys` | Command (client → ZPC) | Request a sealed dump of assigned security keys. The request body is empty. |
| `zpc/<home_id>/Network/DumpSecurityKeys/Report` | Report (ZPC → client) | Status only. The report never contains key material, fingerprints, or path information. |

**Full reference:** [Security Keys Dump topics](../../security/doc/security_keys_dump_mqtt_topics.md) and [setup](../../security/doc/security_keys_dump_mqtt_api.md)

## Command Classes

Per–command-class MQTT topics:

`zpc/<home_id>/<node_id>/ep<endpoint_id>/<CommandClass>/Command/<Command>` and `.../Report/<Report>`.

**Full reference:** [Command Classes MQTT Interface](../../command_classes/doc/generated/mqtt_interface.md)

## Sequences

- **[Inclusion flow](../../../docs/sequences/inclusion_flow.md)**
- **[Exclusion flow](../../../docs/sequences/exclusion_flow.md)**

## See also

- [MQTT API Overview](mqtt_api_overview.md)
- [MQTT API Interface](mqtt_api_interface.md)
