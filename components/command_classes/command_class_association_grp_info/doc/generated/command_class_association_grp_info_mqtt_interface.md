## COMMAND_CLASS_ASSOCIATION_GRP_INFO MQTT API

<!-- Generated from AsyncAPI; do not edit -->

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [ASSOCIATION_GROUP_NAME_GET](#association_group_name_get)
- [ASSOCIATION_GROUP_NAME_REPORT](#association_group_name_report)
- [ASSOCIATION_GROUP_INFO_GET](#association_group_info_get)
- [ASSOCIATION_GROUP_INFO_REPORT](#association_group_info_report)
- [ASSOCIATION_GROUP_COMMAND_LIST_GET](#association_group_command_list_get)
- [ASSOCIATION_GROUP_COMMAND_LIST_REPORT](#association_group_command_list_report)

### ASSOCIATION_GROUP_NAME_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Command/AssociationGroupNameGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "grouping_identifier": "0x01"
}
```

### ASSOCIATION_GROUP_NAME_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Report/AssociationGroupNameReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "grouping_identifier": "0x01",
  "length_of_name": "0x01",
  "name": [
    "0x01"
  ]
}
```

### ASSOCIATION_GROUP_INFO_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Command/AssociationGroupInfoGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "list_mode": "0x01",
    "refresh_cache": "0x01"
  },
  "grouping_identifier": "0x01"
}
```

### ASSOCIATION_GROUP_INFO_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Report/AssociationGroupInfoReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "properties1": {
    "group_count": "0x01",
    "dynamic_info": "0x01",
    "list_mode": "0x01"
  },
  "vg1": [
    {
      "grouping_identifier": "0x01",
      "mode": "0x01",
      "profile1": "0x01",
      "profile2": "0x01",
      "event_code": "0x01"
    }
  ]
}
```

### ASSOCIATION_GROUP_COMMAND_LIST_GET

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Command/AssociationGroupCommandListGet`

**Direction:** Command (client → ZPC)

**Payload:**

```json
{
  "properties1": {
    "allow_cache": "0x01"
  },
  "grouping_identifier": "0x01"
}
```

### ASSOCIATION_GROUP_COMMAND_LIST_REPORT

**Topic:** `zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Report/AssociationGroupCommandListReport`

**Direction:** Report (ZPC → client)

**Payload:**

```json
{
  "grouping_identifier": "0x01",
  "list_length": "0x01",
  "command": [
    "0x01"
  ]
}
```
