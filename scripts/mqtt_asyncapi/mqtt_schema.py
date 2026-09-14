"""JSON Schema for MQTT payloads derived from Z-Wave XML command params."""

from __future__ import annotations

RESERVED = frozenset({"reserved", "res", "res1", "res2", "res3"})


def mqtt_param_name(raw: str | None) -> str:
    if not raw:
        return "unnamed"
    return (
        raw.replace(" ", "_")
        .replace("/", "_")
        .replace("-", "_")
        .replace("+", "_plus")
        .lower()
    )


def mqtt_topic_token(raw: str) -> str:
    return "".join(part.capitalize() for part in raw.split("_") if part)


def command_class_topic_token(cc_name: str) -> str:
    return mqtt_topic_token(cc_name.replace("COMMAND_CLASS_", ""))


def _is_reserved(name: str) -> bool:
    return name in RESERVED


def _param_schema(attribute) -> dict:
    type_name = (getattr(attribute, "type", None) or "").lower()
    if type_name == "struct_byte":
        inner = {}
        for field in getattr(attribute, "fields", []):
            field_name = mqtt_param_name(getattr(field, "name", None))
            if _is_reserved(field_name):
                continue
            inner[field_name] = {
                "type": "string",
                "description": "Hex-encoded byte (e.g. 0x05)",
            }
        return {
            "type": "object",
            "properties": inner,
            "additionalProperties": True,
        }
    if type_name == "variant":
        return {
            "type": "array",
            "items": {"type": "string"},
            "description": "Hex-encoded bytes",
        }
    return {
        "type": "string",
        "description": "Hex-encoded value as published on MQTT",
    }


def command_payload_schema(command) -> dict:
    properties = {}
    for attribute in getattr(command, "params", []):
        type_name = (getattr(attribute, "type", None) or "").lower()
        name = mqtt_param_name(getattr(attribute, "name", None))
        nested = getattr(attribute, "params", None)
        if type_name == "variant_group" or nested is not None:
            item_properties = {}
            for param in nested or []:
                param_name = mqtt_param_name(getattr(param, "name", None))
                if _is_reserved(param_name):
                    continue
                item_properties[param_name] = _param_schema(param)
            properties[name] = {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": item_properties,
                    "additionalProperties": True,
                },
            }
            continue
        if _is_reserved(name):
            continue
        properties[name] = _param_schema(attribute)
    return {
        "type": "object",
        "additionalProperties": True,
        "properties": properties,
    }


def example_from_schema(schema: dict | None):
    if not schema or not isinstance(schema, dict):
        return {}
    type_name = schema.get("type")
    if "example" in schema:
        return schema["example"]
    if type_name == "object" or "properties" in schema:
        return {
            key: example_from_schema(value)
            for key, value in schema.get("properties", {}).items()
        }
    if type_name == "array":
        return [example_from_schema(schema.get("items") or {"type": "string"})]
    if type_name == "boolean":
        return False
    if type_name in ("integer", "number"):
        return 0
    if type_name == "string":
        return "0x01"
    return {}
