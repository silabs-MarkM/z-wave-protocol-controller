# SmartStart MQTT API

<!-- Generated from AsyncAPI; do not edit -->

SmartStart allows pre-provisioned devices to join the network automatically when they
are powered and in range. The client maintains a SmartStart provisioning list and
synchronizes it with ZPC via Update, Add, Remove, and Clear. Read the list back via List.

## Table of Contents
- [NETWORK_SMARTSTART_UPDATE](#network_smartstart_update)
- [NETWORK_SMARTSTART_ADD](#network_smartstart_add)
- [NETWORK_SMARTSTART_REMOVE](#network_smartstart_remove)
- [NETWORK_SMARTSTART_CLEAR](#network_smartstart_clear)
- [NETWORK_SMARTSTART_LIST](#network_smartstart_list)
- [NETWORK_SMARTSTART_LIST_REPORT](#network_smartstart_list_report)

### NETWORK_SMARTSTART_UPDATE

**Topic:** `zpc/<home_id>/Network/SmartStart/Update`

**Direction:** Command (client → ZPC)

Replaces ZPC's entire SmartStart provisioning list with the entries in the payload.
Not incremental. For incremental changes use Add, Remove, or Clear.

**Payload:**

```json
{
  "value": [
    {
      "DSK": "06743-56104-10648-10659-64918-00784-01021-46920",
      "PreferredProtocols": [
        "Z-Wave"
      ]
    }
  ]
}
```

### NETWORK_SMARTSTART_ADD

**Topic:** `zpc/<home_id>/Network/SmartStart/Add`

**Direction:** Command (client → ZPC)

Appends entries. Existing DSKs are skipped. No dedicated report topic;
observe the list via List.

**Payload:**

```json
{
  "value": [
    {
      "DSK": "06743-56104-10648-10659-64918-00784-01021-46920",
      "PreferredProtocols": [
        "Z-Wave"
      ]
    }
  ]
}
```

### NETWORK_SMARTSTART_REMOVE

**Topic:** `zpc/<home_id>/Network/SmartStart/Remove`

**Direction:** Command (client → ZPC)

Removes entries identified by DSK. Only the DSK field is read.
Unknown DSKs are skipped. No dedicated report topic.

**Payload:**

```json
{
  "value": [
    {
      "DSK": "06743-56104-10648-10659-64918-00784-01021-46920",
      "PreferredProtocols": [
        "Z-Wave"
      ]
    }
  ]
}
```

### NETWORK_SMARTSTART_CLEAR

**Topic:** `zpc/<home_id>/Network/SmartStart/Clear`

**Direction:** Command (client → ZPC)

Purges the entire list. Payload ignored.

**Payload:**

```json
{}
```

### NETWORK_SMARTSTART_LIST

**Topic:** `zpc/<home_id>/Network/SmartStart/List`

**Direction:** Command (client → ZPC)

Request the current SmartStart list. Payload ignored.

**Payload:**

```json
{}
```

### NETWORK_SMARTSTART_LIST_REPORT

**Topic:** `zpc/<home_id>/Network/SmartStart/List/Report`

**Direction:** Report (ZPC → client)

Current provisioning list (`value` array, same shape as Update).

**Payload:**

```json
{
  "value": [
    {
      "DSK": "06743-56104-10648-10659-64918-00784-01021-46920",
      "PreferredProtocols": [
        "Z-Wave"
      ]
    }
  ]
}
```

## Entry lifecycle and persistence

The SmartStart list tracks **pending** entries. ZPC removes an entry when inclusion
completes (success or failure) or when the DSK is already in the current network.
The list is in-memory only and is empty after ZPC restarts.

S2 during SmartStart is handled internally. `Network/RequestedKeys/Report`,
`Network/GrantKeys`, `Network/RequestedDSK/Report`, and `Network/DSK/Accept` apply
only to standard inclusion via `Network/Node/Add`.

See [Inclusion flow](../../../docs/sequences/inclusion_flow.md).
