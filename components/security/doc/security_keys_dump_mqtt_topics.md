# Security Keys Dump MQTT Topics

<!-- Generated from AsyncAPI; do not edit -->

On request, ZPC seals its currently-assigned S2 (and S0, if any) keys with a given
X25519 recipient public key and writes the envelope to a configured path.
The report carries only a status code; no key material is exposed to the broker.
The feature is disabled by default. See the Security Keys Dump setup documentation.

## Table of Contents
- [NETWORK_DUMP_SECURITY_KEYS](#network_dump_security_keys)
- [NETWORK_DUMP_SECURITY_KEYS_REPORT](#network_dump_security_keys_report)

### NETWORK_DUMP_SECURITY_KEYS

**Topic:** `zpc/<home_id>/Network/DumpSecurityKeys`

**Direction:** Command (client → ZPC)

Request a sealed dump of assigned security keys. The request body is empty.

**Payload:**

```json
{}
```

### NETWORK_DUMP_SECURITY_KEYS_REPORT

**Topic:** `zpc/<home_id>/Network/DumpSecurityKeys/Report`

**Direction:** Report (ZPC → client)

Status only. The report never contains key material, fingerprints, or path information.

**Payload:**

```json
{
  "status": 0
}
```
