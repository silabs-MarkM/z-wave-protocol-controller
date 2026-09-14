# Network Management MQTT API

<!-- Generated from AsyncAPI; do not edit -->

Add/remove nodes, accept DSK, grant keys, list devices, factory reset, and
Network Layer Security (NLS) operations.

Breaking change: node inventory topics use `Network/Node/List` and
`Network/Node/Properties` (not Device-prefixed names).

## Table of Contents
- [NETWORK_NODE_ADD](#network_node_add)
- [NETWORK_NODE_ADD_REPORT](#network_node_add_report)
- [NETWORK_NODE_ADD_ABORT](#network_node_add_abort)
- [NETWORK_NODE_REMOVE](#network_node_remove)
- [NETWORK_NODE_REMOVE_REPORT](#network_node_remove_report)
- [NETWORK_NODE_REMOVE_FAILED](#network_node_remove_failed)
- [NETWORK_NODE_REMOVE_FAILED_REPORT](#network_node_remove_failed_report)
- [NETWORK_NODE_REMOVE_ABORT](#network_node_remove_abort)
- [NETWORK_DSK_ACCEPT](#network_dsk_accept)
- [NETWORK_REQUESTED_KEYS_REPORT](#network_requested_keys_report)
- [NETWORK_GRANT_KEYS](#network_grant_keys)
- [NETWORK_REQUESTED_DSK_REPORT](#network_requested_dsk_report)
- [NETWORK_NODE_LIST](#network_node_list)
- [NETWORK_NODE_LIST_REPORT](#network_node_list_report)
- [NETWORK_NODE_PROPERTIES](#network_node_properties)
- [NETWORK_NODE_PROPERTIES_REPORT](#network_node_properties_report)
- [NETWORK_NODE_INTERVIEW](#network_node_interview)
- [NETWORK_FACTORY_RESET](#network_factory_reset)
- [NETWORK_FACTORY_RESET_REPORT](#network_factory_reset_report)
- [NETWORK_NLS_ENABLE](#network_nls_enable)
- [NETWORK_NLS_ENABLE_REPORT](#network_nls_enable_report)
- [NETWORK_NLS_STATE](#network_nls_state)
- [NETWORK_NLS_STATE_REPORT](#network_nls_state_report)

### NETWORK_NODE_ADD

**Topic:** `zpc/<home_id>/Network/Node/Add`

**Direction:** Command (client → ZPC)

Start inclusion.

**Payload:**

```json
{}
```

### NETWORK_NODE_ADD_REPORT

**Topic:** `zpc/<home_id>/Network/Node/Add/Report`

**Direction:** Report (ZPC → client)

Success includes `node_id` and optional `dsk`. Fail may be immediate rejection
(`reason` 6401 busy / 6402 reset) or security fail (`reason` 6404).
Status is `"success"` or `"fail"`.

**Payload:**

```json
{
  "node_id": 0,
  "dsk": "0x01",
  "status": "0x01",
  "reason": {},
  "activity": "0x01"
}
```

### NETWORK_NODE_ADD_ABORT

**Topic:** `zpc/<home_id>/Network/Node/Add/Abort`

**Direction:** Command (client → ZPC)

Abort ongoing inclusion. No report is published.

**Payload:**

```json
{}
```

### NETWORK_NODE_REMOVE

**Topic:** `zpc/<home_id>/Network/Node/Remove`

**Direction:** Command (client → ZPC)

Start exclusion.

**Payload:**

```json
{}
```

### NETWORK_NODE_REMOVE_REPORT

**Topic:** `zpc/<home_id>/Network/Node/Remove/Report`

**Direction:** Report (ZPC → client)

Also fired on successful Remove Failed. `dsk` omitted if unavailable.

**Payload:**

```json
{
  "node_id": 0,
  "dsk": "0x01",
  "status": "0x01",
  "reason": "0x01",
  "activity": "0x01"
}
```

### NETWORK_NODE_REMOVE_FAILED

**Topic:** `zpc/<home_id>/Network/Node/RemoveFailed`

**Direction:** Command (client → ZPC)

Remove a node believed failed. Controller NOP-probes first.

**Payload:**

```json
{
  "node_id": 2
}
```

### NETWORK_NODE_REMOVE_FAILED_REPORT

**Topic:** `zpc/<home_id>/Network/Node/RemoveFailed/Report`

**Direction:** Report (ZPC → client)

`ok` / `fail` with `reason` (`operation_successful`, `node_online`, …).
On success, Remove/Report is also published.

**Payload:**

```json
{
  "node_id": 0,
  "status": "0x01",
  "reason": "0x01"
}
```

### NETWORK_NODE_REMOVE_ABORT

**Topic:** `zpc/<home_id>/Network/Node/Remove/Abort`

**Direction:** Command (client → ZPC)

Abort ongoing exclusion. No report is published.

**Payload:**

```json
{}
```

### NETWORK_DSK_ACCEPT

**Topic:** `zpc/<home_id>/Network/DSK/Accept`

**Direction:** Command (client → ZPC)

Accept DSK during classic S2 inclusion. Not used for SmartStart.

**Payload:**

```json
{
  "dsk": "0x01"
}
```

### NETWORK_REQUESTED_KEYS_REPORT

**Topic:** `zpc/<home_id>/Network/RequestedKeys/Report`

**Direction:** Report (ZPC → client)

Keys requested during classic S2 inclusion. Not published for SmartStart.

**Payload:**

```json
{
  "Keys": "0x7",
  "CSA": false
}
```

### NETWORK_GRANT_KEYS

**Topic:** `zpc/<home_id>/Network/GrantKeys`

**Direction:** Command (client → ZPC)

Grant keys during classic S2 inclusion. Not used for SmartStart.

**Payload:**

```json
{
  "Accept": true,
  "Keys": 7,
  "CSA": false
}
```

### NETWORK_REQUESTED_DSK_REPORT

**Topic:** `zpc/<home_id>/Network/RequestedDSK/Report`

**Direction:** Report (ZPC → client)

DSK verification needed during classic S2 inclusion.

**Payload:**

```json
{
  "DSK": "0x01"
}
```

### NETWORK_NODE_LIST

**Topic:** `zpc/<home_id>/Network/Node/List`

**Direction:** Command (client → ZPC)

Request node list.

**Payload:**

```json
{}
```

### NETWORK_NODE_LIST_REPORT

**Topic:** `zpc/<home_id>/Network/Node/List/Report`

**Direction:** Report (ZPC → client)

Array of nodes with `node_information` and `version_report`.

**Payload:**

```json
[
  {}
]
```

### NETWORK_NODE_PROPERTIES

**Topic:** `zpc/<home_id>/Network/Node/Properties`

**Direction:** Command (client → ZPC)

Request node properties.

**Payload:**

```json
{
  "node_id": 2
}
```

### NETWORK_NODE_PROPERTIES_REPORT

**Topic:** `zpc/<home_id>/Network/Node/Properties/Report`

**Direction:** Report (ZPC → client)

Node properties; missing attribute-store values are `null`.

**Payload:**

```json
{}
```

### NETWORK_NODE_INTERVIEW

**Topic:** `zpc/<home_id>/Network/Node/Interview`

**Direction:** Command (client → ZPC)

Request a (re-)interview for `node_id`. Completion is published on
`Interview/Report`, not under this topic.

**Payload:**

```json
{
  "node_id": 2
}
```

### NETWORK_FACTORY_RESET

**Topic:** `zpc/<home_id>/Network/FactoryReset`

**Direction:** Command (client → ZPC)

Factory reset the controller.

**Payload:**

```json
{}
```

### NETWORK_FACTORY_RESET_REPORT

**Topic:** `zpc/Network/FactoryReset/Report`

**Direction:** Report (ZPC → client)

Global topic. Published once when the controller is ready on the new network.
Not retained.

**Payload:**

```json
{
  "status": "ready",
  "home_id": "AABBCCDD"
}
```

### NETWORK_NLS_ENABLE

**Topic:** `zpc/<home_id>/Network/NLS/Enable`

**Direction:** Command (client → ZPC)

Enable NLS for a node.

**Payload:**

```json
{
  "node_id": 0
}
```

### NETWORK_NLS_ENABLE_REPORT

**Topic:** `zpc/<home_id>/Network/NLS/Enable/Report`

**Direction:** Report (ZPC → client)

NLS enable result.

**Payload:**

```json
{
  "node_id": 0,
  "status": "0x01"
}
```

### NETWORK_NLS_STATE

**Topic:** `zpc/<home_id>/Network/NLS/State`

**Direction:** Command (client → ZPC)

Request NLS state.

**Payload:**

```json
{
  "node_id": 0
}
```

### NETWORK_NLS_STATE_REPORT

**Topic:** `zpc/<home_id>/Network/NLS/State/Report`

**Direction:** Report (ZPC → client)

NLS state report.

**Payload:**

```json
{
  "node_id": 0,
  "nls_support": false,
  "nls_state": false,
  "status": "0x01"
}
```
