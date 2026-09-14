## COMMAND_CLASS_ASSOCIATION MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [ASSOCIATION_GET](#association_get)
- [ASSOCIATION_GROUPINGS_GET](#association_groupings_get)
- [ASSOCIATION_GROUPINGS_REPORT](#association_groupings_report)
- [ASSOCIATION_REMOVE](#association_remove)
- [ASSOCIATION_REPORT](#association_report)
- [ASSOCIATION_SET](#association_set)
- [ASSOCIATION_SPECIFIC_GROUP_GET](#association_specific_group_get)
- [ASSOCIATION_SPECIFIC_GROUP_REPORT](#association_specific_group_report)

### ASSOCIATION_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Command/AssociationGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "grouping_identifier": "0x01"
}
```

### ASSOCIATION_GROUPINGS_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Command/AssociationGroupingsGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### ASSOCIATION_GROUPINGS_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Report/AssociationGroupingsReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "supported_groupings": "0x01"
}
```

### ASSOCIATION_REMOVE

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Command/AssociationRemove`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "grouping_identifier": "0x01",
  "node_id": [
    "0x01"
  ]
}
```

### ASSOCIATION_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Report/AssociationReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "grouping_identifier": "0x01",
  "max_nodes_supported": "0x01",
  "reports_to_follow": "0x01",
  "nodeid": [
    "0x01"
  ]
}
```

### ASSOCIATION_SET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Command/AssociationSet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "grouping_identifier": "0x01",
  "node_id": [
    "0x01"
  ]
}
```

### ASSOCIATION_SPECIFIC_GROUP_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Command/AssociationSpecificGroupGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{}
```

### ASSOCIATION_SPECIFIC_GROUP_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Report/AssociationSpecificGroupReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "group": "0x01"
}
```
