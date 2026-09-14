# Device Interview MQTT API

<!-- Generated from AsyncAPI; do not edit -->

Clients request an on-demand interview via Network Management MQTT
(`zpc/{homeId}/Network/Node/Interview`). Completion is published here per endpoint.

## Table of Contents
- [INTERVIEW_REPORT](#interview_report)

### INTERVIEW_REPORT

**Topic:** `zpc/<home_id>/Interview/Report`

**Direction:** Report (ZPC → client)

Published when an interview completes for an endpoint—either successfully or after
cancellation/failure. One report is sent per endpoint (including endpoint 0).
For successful interviews, the report is delayed until every command class
`on_interview`-triggered resolution has settled, so receiving this message means
the device is actually ready.

**Payload:**

```json
{
  "node_id": 2,
  "endpoint_id": 0,
  "status": 0
}
```
