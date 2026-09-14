# Network Status MQTT API

<!-- Generated from AsyncAPI; do not edit -->

Unsolicited reports reflecting the availability of Z-Wave nodes
(online/offline/unknown transitions for Always-Listening, FLiRS, and Non-Listening devices).
No request is needed from the client.

## Table of Contents
- [NETWORK_STATUS_REPORT](#network_status_report)

### NETWORK_STATUS_REPORT

**Topic:** `zpc/<home_id>/Network/Status/Report`

**Direction:** Report (ZPC → client)

Published whenever Network Monitor detects a node availability transition.
See the Network Status behavior document for AL/FLiRS/NL semantics.

**Payload:**

```json
{
  "node_id": 2,
  "status": "online"
}
```
