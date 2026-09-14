## COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [MULTI_CHANNEL_ASSOCIATION_GET](#multi_channel_association_get)
- [MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET](#multi_channel_association_groupings_get)
- [MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT](#multi_channel_association_groupings_report)
- [MULTI_CHANNEL_ASSOCIATION_REMOVE](#multi_channel_association_remove)
- [MULTI_CHANNEL_ASSOCIATION_REPORT](#multi_channel_association_report)
- [MULTI_CHANNEL_ASSOCIATION_SET](#multi_channel_association_set)

### MULTI_CHANNEL_ASSOCIATION_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Command/MultiChannelAssociationGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "grouping_identifier": "0x01"
}
```

### MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Command/MultiChannelAssociationGroupingsGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Report/MultiChannelAssociationGroupingsReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "supported_groupings": "0x01"
}
```

### MULTI_CHANNEL_ASSOCIATION_REMOVE

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Command/MultiChannelAssociationRemove`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "grouping_identifier": "0x01",
  "node_id": [
    "0x01"
  ],
  "marker": "0x01",
  "vg": [
    {
      "multi_channel_node_id": "0x01",
      "properties1": {
        "end_point": "0x01",
        "bit_address": "0x01"
      }
    }
  ]
}
```

### MULTI_CHANNEL_ASSOCIATION_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Report/MultiChannelAssociationReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "grouping_identifier": "0x01",
  "max_nodes_supported": "0x01",
  "reports_to_follow": "0x01",
  "node_id": [
    "0x01"
  ],
  "marker": "0x01",
  "vg": [
    {
      "multi_channel_node_id": "0x01",
      "properties1": {
        "end_point": "0x01",
        "bit_address": "0x01"
      }
    }
  ]
}
```

### MULTI_CHANNEL_ASSOCIATION_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Command/MultiChannelAssociationSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "grouping_identifier": "0x01",
  "node_id": [
    "0x01"
  ],
  "marker": "0x01",
  "vg": [
    {
      "multi_channel_node_id": "0x01",
      "properties1": {
        "end_point": "0x01",
        "bit_address": "0x01"
      }
    }
  ]
}
```
