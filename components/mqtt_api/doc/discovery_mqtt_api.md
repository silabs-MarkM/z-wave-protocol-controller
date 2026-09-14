# Discovery MQTT API

<!-- Generated from AsyncAPI; do not edit -->

The Discovery API allows clients to obtain the Z-Wave Home ID of the controller.
This is a global topic (not scoped by home ID) because the Home ID is unknown until after discovery.

## Table of Contents
- [DISCOVERY](#discovery)
- [DISCOVERY_REPORT](#discovery_report)

### DISCOVERY

**Topic:** `zpc/Discovery`

**Direction:** Command (client → ZPC)

Publish any message (e.g. empty JSON `{}`) to request the current Home ID.

**Payload:**

```json
{}
```

### DISCOVERY_REPORT

**Topic:** `zpc/Discovery/Report`

**Direction:** Report (ZPC → client)

The `home_id` is the 8-character hexadecimal representation of the Z-Wave Home ID.
Use this value to construct network-scoped topics `zpc/<home_id>/...`.

**Payload:**

```json
{
  "home_id": "CAFECAFE"
}
```

## Example

```bash
# Request Home ID
mosquitto_pub -t "zpc/Discovery" -m '{}'

# Subscribe to receive the report
mosquitto_sub -t "zpc/Discovery/Report" -v
```
