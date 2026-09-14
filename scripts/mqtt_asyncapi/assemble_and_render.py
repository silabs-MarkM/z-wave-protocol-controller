#!/usr/bin/env python3
"""Assemble AsyncAPI fragments and render MQTT reference markdown."""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

import yaml

_SCRIPTS_DIR = Path(__file__).resolve().parent.parent
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from mqtt_asyncapi.mqtt_schema import example_from_schema

REPO_ROOT = Path(__file__).resolve().parents[2]
GENERATED_BANNER = "<!-- Generated from AsyncAPI; do not edit -->"

DOC_PAGES = {
    "discovery": Path("components/mqtt_api/doc/discovery_mqtt_api.md"),
    "network": Path("components/network_manager/doc/network_management_mqtt_api.md"),
    "smartstart": Path("components/mqtt_api/doc/smartstart_mqtt_api.md"),
    "interview": Path("components/device_interviewer/docs/interview_mqtt_api.md"),
    "network_status": Path("components/network_monitor/doc/network_status_mqtt_api.md"),
    "ota": Path("components/ota/docs/ota_mqtt_api.md"),
    "security_keys_dump": Path("components/security/doc/security_keys_dump_mqtt_topics.md"),
}

INDEX_PATH = Path("components/mqtt_api/doc/mqtt_api_index.md")
CC_INDEX_PATH = Path("doc/generated/mqtt_interface.md")


def deep_merge(base: dict, overlay: dict) -> dict:
    merged = dict(base)
    for key, value in overlay.items():
        if key in merged and isinstance(merged[key], dict) and isinstance(value, dict):
            merged[key] = deep_merge(merged[key], value)
        else:
            merged[key] = value
    return merged


def load_yaml(path: Path) -> dict:
    with path.open(encoding="utf-8") as handle:
        data = yaml.safe_load(handle)
    return data if isinstance(data, dict) else {}


def load_component_fragments(root_dir: Path) -> list[dict]:
    fragments = []
    skip = {"asyncapi.yaml", "asyncapi.assembled.yaml"}
    for path in sorted((root_dir / "components").glob("*/asyncapi/*.yaml")):
        if path.name in skip:
            continue
        fragments.append(load_yaml(path))
    return fragments



def fragment_doc_meta(fragment: dict) -> dict:
    group = fragment.get("x-doc-group")
    if not group:
        return {}
    return {
        "group": group,
        "title": fragment.get("x-doc-title") or group,
        "preamble": fragment.get("x-doc-preamble") or "",
        "appendix": fragment.get("x-doc-appendix") or "",
    }


def strip_extension_keys(document: dict) -> dict:
    return {key: value for key, value in document.items() if not str(key).startswith("x-doc-")}


def assemble(root: dict, fragments: list[dict], cc_doc: dict) -> tuple[dict, dict]:
    assembled = dict(root)
    assembled.setdefault("channels", {})
    assembled.setdefault("operations", {})
    assembled.setdefault("components", {})
    meta_by_group = {}
    for fragment in fragments:
        meta = fragment_doc_meta(fragment)
        if meta:
            meta_by_group[meta["group"]] = meta
        assembled = deep_merge(assembled, strip_extension_keys(fragment))
    assembled = deep_merge(assembled, strip_extension_keys(cc_doc))
    return assembled, meta_by_group


def _resolve_ref(document: dict, ref: str):
    if not ref.startswith("#/"):
        return None
    node = document
    for part in ref[2:].split("/"):
        if not isinstance(node, dict) or part not in node:
            return None
        node = node[part]
    return node


def channel_payload_schema(document: dict, channel: dict) -> dict | None:
    messages = channel.get("messages") or {}
    if not messages:
        return None
    first = next(iter(messages.values()))
    if isinstance(first, dict) and "$ref" in first:
        first = _resolve_ref(document, first["$ref"]) or {}
    payload = (first or {}).get("payload") or {}
    schema = payload.get("schema") if isinstance(payload, dict) else None
    if isinstance(schema, dict) and "$ref" in schema:
        schema = _resolve_ref(document, schema["$ref"])
    return schema if isinstance(schema, dict) else None


def payload_example(document: dict, channel: dict):
    schema = channel_payload_schema(document, channel)
    return example_from_schema(schema)


def human_topic(address: str) -> str:
    return (
        address.replace("{homeId}", "<home_id>")
        .replace("{nodeId}", "<node_id>")
        .replace("{endpointId}", "<endpoint_id>")
    )


def slug(text: str) -> str:
    text = text.strip().lower().replace(" ", "-")
    return re.sub(r"[^a-z0-9_-]+", "", text)


def operations_by_channel(document: dict) -> dict[str, list[dict]]:
    mapping = defaultdict(list)
    for op_id, operation in (document.get("operations") or {}).items():
        channel = operation.get("channel") or {}
        ref = channel.get("$ref") if isinstance(channel, dict) else None
        if not ref:
            continue
        channel_id = ref.rsplit("/", 1)[-1]
        mapping[channel_id].append({"id": op_id, **operation})
    return mapping


def heading_for_channel(channel_id: str, channel: dict, operations: list[dict]) -> str:
    if operations and operations[0].get("title"):
        return operations[0]["title"]
    return channel.get("x-command-name") or channel_id


def direction_label(operations: list[dict]) -> str:
    actions = {op.get("action") for op in operations}
    if actions == {"receive"}:
        return "Command (client → ZPC)"
    if actions == {"send"}:
        return "Report (ZPC → client)"
    return "Command / Report"


def render_channel_section(
    document: dict, channel_id: str, channel: dict, operations: list[dict]
) -> str:
    title = heading_for_channel(channel_id, channel, operations)
    address = channel.get("address") or ""
    description = (operations[0].get("description") if operations else None) or channel.get(
        "description"
    ) or ""
    example = payload_example(document, channel)
    example_json = json.dumps(example, indent=2)
    lines = [
        f"### {title}",
        "",
        f"**Topic:** `{human_topic(address)}`",
        "",
        f"**Direction:** {direction_label(operations)}",
        "",
    ]
    if description.strip():
        lines.extend([description.strip(), ""])
    lines.extend(["**Payload:**", "", "```json", example_json, "```", ""])
    return "\n".join(lines)


def group_channels(document: dict) -> dict[str, list[tuple[str, dict]]]:
    grouped = defaultdict(list)
    for channel_id, channel in (document.get("channels") or {}).items():
        group = channel.get("x-doc-group") or "other"
        grouped[group].append((channel_id, channel))
    return grouped


def render_group_markdown(
    document: dict,
    group: str,
    channels: list[tuple[str, dict]],
    ops_map: dict[str, list[dict]],
    meta: dict,
) -> str:
    title = meta.get("title") or group
    preamble = meta.get("preamble") or ""
    appendix = meta.get("appendix") or ""
    parts = [f"# {title}", "", GENERATED_BANNER, ""]
    if preamble.strip():
        parts.extend([preamble.strip(), ""])
    parts.append("## Table of Contents")
    for channel_id, channel in channels:
        heading = heading_for_channel(channel_id, channel, ops_map.get(channel_id, []))
        parts.append(f"- [{heading}](#{slug(heading)})")
    parts.append("")
    for channel_id, channel in channels:
        parts.append(
            render_channel_section(document, channel_id, channel, ops_map.get(channel_id, []))
        )
    if appendix.strip():
        parts.extend([appendix.strip(), ""])
    return "\n".join(parts).rstrip() + "\n"


def render_cc_page(
    document: dict,
    cc_name: str,
    channels: list[tuple[str, dict]],
    ops_map: dict[str, list[dict]],
) -> str:
    first = channels[0][1] if channels else {}
    mqtt_support = str(first.get("x-mqtt-support", "true")).lower()
    support = str(first.get("x-support", "false")).lower()
    control = str(first.get("x-control", "false")).lower()
    parts = [
        f"## {cc_name} MQTT API",
        "",
        GENERATED_BANNER,
        "",
        "| MQTT Support | Support | Control |",
        "|--------------|---------|---------|",
        f"| {mqtt_support} | {support} | {control} |",
        "",
    ]
    if str(mqtt_support).lower() != "true":
        parts.extend(
            [
                "> When MQTT Support is `false`, only incoming (TX) commands are listed below. "
                "ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.",
                "",
            ]
        )
    parts.append("### Table of Contents")
    for channel_id, channel in channels:
        heading = heading_for_channel(channel_id, channel, ops_map.get(channel_id, []))
        parts.append(f"- [{heading}](#{slug(heading)})")
    parts.append("")
    for channel_id, channel in channels:
        parts.append(
            render_channel_section(document, channel_id, channel, ops_map.get(channel_id, []))
        )
    return "\n".join(parts).rstrip() + "\n"


def render_cc_index(cc_pages: list[tuple[str, str, str]]) -> str:
    parts = [
        "## MQTT Interface Documentation",
        "",
        GENERATED_BANNER,
        "",
        "### Endpoint addressing for multi-endpoint devices",
        "",
        "For devices that support multiple endpoints (e.g. multi-channel devices), "
        "include the endpoint identifier in the MQTT topic using the `ep{endpoint_id}` segment:",
        "",
        "```",
        "zpc/{home_id}/{node_id}/ep{endpoint_id}/{command_class}/Command/{command_name}",
        "```",
        "",
        "- **ep0** — root device (endpoint 0)",
        "- **ep1**, **ep2**, **ep3**, … — endpoint 1, 2, 3, etc.",
        "",
        "### Command class MQTT capabilities",
        "",
        "| Command Class | MQTT Support | Support | Control |",
        "|---------------|--------------|---------|---------|",
    ]
    for cc_name, rel_link, flags in cc_pages:
        parts.append(f"| [{cc_name}]({rel_link}) | {flags} |")
    parts.extend(["", "### Command Classes", ""])
    for cc_name, rel_link, _flags in cc_pages:
        parts.append(f"- [{cc_name}]({rel_link})")
    parts.append("")
    return "\n".join(parts)


def render_index(document: dict, grouped: dict, meta_by_group: dict) -> str:
    parts = [
        "# MQTT API Index",
        "",
        GENERATED_BANNER,
        "",
        "This page is generated from the assembled AsyncAPI document. "
        "Edit per-component `asyncapi/*.yaml` or `zwave.xml`, not this file.",
        "",
        "## Table of Contents",
        "",
        "- [Discovery](#discovery)",
        "- [Network Management](#network-management)",
        "- [SmartStart](#smartstart)",
        "- [Device Interview](#device-interview)",
        "- [Network Status](#network-status)",
        "- [OTA (Firmware Update)](#ota-firmware-update)",
        "- [Security Keys Dump](#security-keys-dump)",
        "- [Command Classes](#command-classes)",
        "- [Sequences](#sequences)",
        "",
    ]

    def table_for(group: str, heading: str):
        parts.append(f"## {heading}")
        parts.append("")
        preamble = (meta_by_group.get(group) or {}).get("preamble") or ""
        if preamble.strip():
            first_line = preamble.strip().split("\n")[0]
            parts.extend([first_line, ""])
        parts.extend(["| Topic | Direction | Description |", "|-------|-----------|-------------|"])
        for channel_id, channel in grouped.get(group, []):
            ops = operations_by_channel(document).get(channel_id, [])
            desc = (ops[0].get("description") if ops else None) or channel.get("description") or ""
            desc = " ".join(desc.split())
            if len(desc) > 120:
                desc = desc[:117] + "..."
            parts.append(
                f"| `{human_topic(channel.get('address') or '')}` | {direction_label(ops)} | {desc} |"
            )
        parts.append("")

    table_for("discovery", "Discovery")
    parts.append("**Full reference:** [Discovery MQTT API](discovery_mqtt_api.md)")
    parts.append("")
    table_for("network", "Network Management")
    parts.append(
        "**Full reference:** [Network Management MQTT API](../../network_manager/doc/network_management_mqtt_api.md)"
    )
    parts.append("")
    table_for("smartstart", "SmartStart")
    parts.append("**Full reference:** [SmartStart MQTT API](smartstart_mqtt_api.md)")
    parts.append("")
    table_for("interview", "Device Interview")
    parts.append(
        "**Full reference:** [Device Interview MQTT API](../../device_interviewer/docs/interview_mqtt_api.md)"
    )
    parts.append("")
    table_for("network_status", "Network Status")
    parts.append(
        "**Full reference:** [Network Status MQTT](../../network_monitor/doc/network_status_mqtt_api.md) "
        "and [Network Status](../../network_monitor/doc/network_status.md)"
    )
    parts.append("")
    table_for("ota", "OTA (Firmware Update)")
    parts.append(
        "**Full reference:** [OTA MQTT API](../../ota/docs/ota_mqtt_api.md) "
        "and [OTA Firmware Manager](../../ota/docs/ota.md)"
    )
    parts.append("")
    table_for("security_keys_dump", "Security Keys Dump")
    parts.append(
        "**Full reference:** [Security Keys Dump topics](../../security/doc/security_keys_dump_mqtt_topics.md) "
        "and [setup](../../security/doc/security_keys_dump_mqtt_api.md)"
    )
    parts.append("")
    parts.extend(
        [
            "## Command Classes",
            "",
            "Per–command-class MQTT topics:",
            "",
            "`zpc/<home_id>/<node_id>/ep<endpoint_id>/<CommandClass>/Command/<Command>` "
            "and `.../Report/<Report>`.",
            "",
            "**Full reference:** [Command Classes MQTT Interface](../../command_classes/doc/generated/mqtt_interface.md)",
            "",
            "## Sequences",
            "",
            "- **[Inclusion flow](../../../docs/sequences/inclusion_flow.md)**",
            "- **[Exclusion flow](../../../docs/sequences/exclusion_flow.md)**",
            "",
            "## See also",
            "",
            "- [MQTT API Overview](mqtt_api_overview.md)",
            "- [MQTT API Interface](mqtt_api_interface.md)",
            "",
        ]
    )
    return "\n".join(parts)


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Assemble AsyncAPI and render MQTT markdown")
    parser.add_argument("--root", type=Path, default=REPO_ROOT)
    parser.add_argument(
        "--asyncapi-dir",
        type=Path,
        default=None,
        help="Directory containing the root asyncapi.yaml",
    )
    parser.add_argument(
        "--cc-yaml",
        type=Path,
        default=None,
        help="Generated command_classes.asyncapi.yaml",
    )
    parser.add_argument(
        "--assembled-out",
        type=Path,
        default=None,
    )
    parser.add_argument(
        "--cc-md-root",
        type=Path,
        default=None,
        help="Root to write per-CC mqtt_interface markdown (command_classes tree)",
    )
    args = parser.parse_args()
    root_dir = args.root
    asyncapi_dir = args.asyncapi_dir or (root_dir / "components/mqtt_api/asyncapi")
    cc_yaml = args.cc_yaml or (
        root_dir / "components/command_classes/doc/generated/command_classes.asyncapi.yaml"
    )
    assembled_out = args.assembled_out or (root_dir / "build" / "asyncapi.assembled.yaml")
    cc_md_root = args.cc_md_root or (root_dir / "components/command_classes")

    root_doc = load_yaml(asyncapi_dir / "asyncapi.yaml")
    fragments = load_component_fragments(root_dir)
    cc_doc = load_yaml(cc_yaml) if cc_yaml.exists() else {}

    assembled, meta_by_group = assemble(root_doc, fragments, cc_doc)
    assembled_out.parent.mkdir(parents=True, exist_ok=True)
    with assembled_out.open("w", encoding="utf-8") as handle:
        yaml.safe_dump(
            assembled,
            handle,
            sort_keys=False,
            allow_unicode=True,
            width=120,
        )

    ops_map = operations_by_channel(assembled)
    grouped = group_channels(assembled)

    for group, rel_path in DOC_PAGES.items():
        channels = grouped.get(group, [])
        if not channels:
            continue
        write_text(
            root_dir / rel_path,
            render_group_markdown(
                assembled, group, channels, ops_map, meta_by_group.get(group) or {}
            ),
        )

    write_text(root_dir / INDEX_PATH, render_index(assembled, grouped, meta_by_group))

    cc_by_name = defaultdict(list)
    for channel_id, channel in grouped.get("command_class", []):
        cc_by_name[channel.get("x-cc-name") or "UNKNOWN"].append((channel_id, channel))

    cc_index_rows = []
    for cc_name in sorted(cc_by_name.keys()):
        channels = cc_by_name[cc_name]
        cc_dir = channels[0][1].get("x-cc-dir") or cc_name.lower()
        md_name = f"{cc_dir}_mqtt_interface.md"
        write_text(
            cc_md_root / cc_dir / "doc" / "generated" / md_name,
            render_cc_page(assembled, cc_name, channels, ops_map),
        )
        flags = channels[0][1]
        flag_cells = (
            f"{str(flags.get('x-mqtt-support', 'false')).lower()} | "
            f"{str(flags.get('x-support', 'false')).lower()} | "
            f"{str(flags.get('x-control', 'false')).lower()}"
        )
        cc_index_rows.append(
            (cc_name, f"{cc_dir}/doc/generated/{md_name}", flag_cells)
        )

    write_text(
        cc_md_root / CC_INDEX_PATH,
        render_cc_index(cc_index_rows),
    )
    print(f"Loaded {len(fragments)} component fragments")
    print(f"Wrote assembled spec to {assembled_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
