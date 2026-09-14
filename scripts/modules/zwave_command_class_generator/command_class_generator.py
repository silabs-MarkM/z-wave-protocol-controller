#!/usr/bin/env python3
# -*- mode: python; python-indent-offset: 4; indent-tabs-mode: nil -*-
# -*- coding: utf-8 -*-
# coding=utf-8
# Local Variables:
# mode:python
# indent-tabs-mode: nil
# vim: expandtab ts=4 sw=4
# End:

""" Generate Z-Wave command classes handling
"""

from pathlib import Path
import json
import logging
import subprocess
import sys
import yaml
from itertools import chain
from jinja2 import Environment, FileSystemLoader

_SCRIPTS_DIR = Path(__file__).resolve().parents[2]
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))
from mqtt_asyncapi.mqtt_schema import command_payload_schema


def _mqtt_examples_by_address(path: Path) -> dict:
    if not path.is_file():
        return {}
    data = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    examples = {}
    for channel in (data.get("channels") or {}).values():
        if not isinstance(channel, dict):
            continue
        address = channel.get("address")
        messages = channel.get("messages") or {}
        if not address or not isinstance(messages, dict) or not messages:
            continue
        message = next(iter(messages.values()))
        if not isinstance(message, dict):
            continue
        sample = message.get("examples") or []
        if sample and isinstance(sample[0], dict) and "payload" in sample[0]:
            examples[address] = sample[0]
    return examples


def _mqtt_examples_from_tree(output_dir: Path) -> dict:
    examples = {}
    paths = sorted(output_dir.glob("*/asyncapi/*.yaml"))
    legacy = output_dir / "doc/generated/command_classes.asyncapi.yaml"
    if legacy.is_file():
        paths.append(legacy)
    for path in paths:
        examples.update(_mqtt_examples_by_address(path))
    return examples


class ClangFormat:
    def __init__(self):
        self._clang_format_path = self._find_clang_format()

    def _find_clang_format(self):
        """Find clang-format executable in the system PATH."""
        try:
            result = subprocess.run(
                ["which", "clang-format"], capture_output=True, text=True, check=True
            )
            return result.stdout.strip()
        except subprocess.CalledProcessError:
            logging.warning(
                "clang-format not found in PATH, formatting will be skipped"
            )
            return None

    def format(self, content, style=None):
        """Format C/C++ code using clang-format."""
        if not self._clang_format_path:
            logging.warning("clang-format not available, returning unformatted content")
            return content

        try:
            # Default style if none specified
            if style is None:
                style = "file"

            # Run clang-format with explicit language specification
            result = subprocess.run(
                [
                    self._clang_format_path,
                    f"--style={style}",
                    "--assume-filename=file.cpp",
                ],
                input=content,
                capture_output=True,
                text=True,
                check=True,
            )
            return result.stdout
        except subprocess.CalledProcessError as e:
            logging.error(f"clang-format failed: {e}")
            logging.error(f"stderr: {e.stderr}")
            return content
        except Exception as e:
            logging.error(f"Error running clang-format: {e}")
            return content


class CommandClassGenerator:
    def __init__(self):
        self._templates = []
        self._clang_format = ClangFormat()
        self._output_dir = None

    def load_templates(self, templates_dir):
        self._templates_dir = templates_dir
        allowed_extensions = ["*.j2", "*.jinja", "*.jinja2"]
        self._templates = list(
            chain.from_iterable(templates_dir.rglob(ext) for ext in allowed_extensions)
        )

        logging.debug(f"Loaded {len(self._templates)} templates")
        for template in self._templates:
            logging.debug(f"\t{template}")

    def load_command_classes(self, command_classes):
        self._command_classes = command_classes

        logging.debug(f"Loaded {len(self._command_classes)} command classes")
        for command_class in self._command_classes:
            logging.debug(f"\t{command_class.name}")

    @property
    def output_dir(self):
        return self._output_dir

    @output_dir.setter
    def output_dir(self, output_dir):
        self._output_dir = output_dir

    # Templates whose output is only meaningful when the CC has full mqtt_support.
    # The mqtt_interface.md doc is also rendered for CCs that publish reports only
    # (see has_mqtt_interface_doc).
    _MQTT_ONLY_TEMPLATE_NAMES = {
        "+command_class+_mqtt.hpp.j2",
        "+command_class+_mqtt.cpp.j2",
    }
    _MQTT_ASYNCAPI_TEMPLATE_NAME = "+command_class+.yaml.j2"

    @classmethod
    def _should_render(cls, template_name, command_class):
        if template_name == cls._MQTT_ASYNCAPI_TEMPLATE_NAME:
            return command_class.has_mqtt_interface_doc
        if command_class.mqtt_support:
            return True
        if template_name in cls._MQTT_ONLY_TEMPLATE_NAMES:
            return False
        return True

    def _mqtt_interface_doc_path(self, command_class, output_dir: Path) -> Path:
        cc_name = command_class.name.lower()
        return (
            output_dir
            / cc_name
            / "doc"
            / "generated"
            / f"{cc_name}_mqtt_interface.md"
        )

    def _cleanup_stale_mqtt_interface_docs(self, output_dir: Path) -> None:
        for command_class in self._command_classes:
            if command_class.has_mqtt_interface_doc:
                continue
            doc_path = self._mqtt_interface_doc_path(command_class, output_dir)
            if doc_path.exists():
                doc_path.unlink()

    def _cleanup_stale_mqtt_asyncapi(self, output_dir: Path) -> None:
        keep = {
            command_class.name.lower()
            for command_class in self._command_classes
            if command_class.has_mqtt_interface_doc
        }
        for path in output_dir.glob("*/asyncapi/*.yaml"):
            if path.parent.parent.name not in keep:
                path.unlink()

    def generate_sources(self):
        templateLoader = FileSystemLoader(searchpath=self._templates_dir)
        
        def _as_json(value, **kwargs):
            return json.dumps(value, **kwargs)

        def _parse_json(value):
            return json.loads(value)

        # Default environment for C/C++ files
        templateEnv = Environment( # NOSONAR: no injection offline
            autoescape=False,  # NOSONAR: no injection offline
            loader=templateLoader,
            trim_blocks=True,
            lstrip_blocks=True,
            extensions=["jinja2.ext.do"],
        )
        templateEnv.filters["as_json"] = _as_json
        templateEnv.filters["parse_json"] = _parse_json

        # Special environment for markdown files (preserve whitespace)
        markdownTemplateEnv = Environment( # NOSONAR: no injection offline
            autoescape=False,  # NOSONAR: no injection offline
            loader=templateLoader,
            trim_blocks=True,
            lstrip_blocks=False,  # Don't strip leading whitespace for markdown
            extensions=["jinja2.ext.do", "jinja2.ext.loopcontrols"],
        )
        markdownTemplateEnv.filters["as_json"] = _as_json
        markdownTemplateEnv.filters["parse_json"] = _parse_json
        markdownTemplateEnv.filters["mqtt_payload_schema"] = command_payload_schema
        templateEnv.filters["mqtt_payload_schema"] = command_payload_schema

        utility_templates = [
            "utils.j2",
            "definitions.j2",
            "function_generator.j2",
            "config.j2",
        ]

        output_dir = self._output_dir if self._output_dir else Path("generated")
        mqtt_examples = _mqtt_examples_from_tree(output_dir)

        for template in self._templates:
            if template.name in utility_templates:
                continue

            template_relative_path = template.relative_to(self._templates_dir)
            template_name = template.name

            is_doc_template = template_name.endswith(".md.j2") or template_name.endswith(
                ".yaml.j2"
            )
            if is_doc_template:
                loaded = markdownTemplateEnv.get_template(str(template_relative_path))
            else:
                loaded = templateEnv.get_template(str(template_relative_path))

            is_per_cc = "+command_class+" in str(template_relative_path)
            classes_to_render = (
                self._command_classes
                if is_per_cc
                else [self._command_classes[0] if self._command_classes else None]
            )

            for command_class in classes_to_render:
                if command_class is None:
                    continue
                if is_per_cc and not self._should_render(template_name, command_class):
                    continue

                output = loaded.render(
                    command_classes=self._command_classes,
                    command_class=command_class,
                    mqtt_examples=mqtt_examples,
                )

                output_file_name = (
                    str(template_relative_path)
                    .replace(".j2", "")
                    .replace(".jinja", "")
                    .replace(".jinja2", "")
                )
                output_file_name = output_file_name.replace(
                    "+command_class+", f"{command_class.name.lower()}"
                )

                skip_format = (
                    template.name.endswith(".txt.j2")
                    or template.name.endswith(".md.j2")
                    or template.name.endswith(".yaml.j2")
                )
                if skip_format:
                    formatted_output = output
                else:
                    formatted_output = self._clang_format.format(output)

                output_file_path = output_dir / Path(output_file_name)
                output_file_path.parent.mkdir(parents=True, exist_ok=True)

                posix_path = output_file_path.as_posix()
                if (
                    "generated" in posix_path
                    or "/asyncapi/" in posix_path
                    or output_file_path.exists() is False
                ):
                    with open(output_file_path, "w") as f:
                        f.write(formatted_output)

        output_dir = self._output_dir if self._output_dir else Path("generated")
        self._cleanup_stale_mqtt_interface_docs(output_dir)
        self._cleanup_stale_mqtt_asyncapi(output_dir)
