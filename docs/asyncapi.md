# MQTT AsyncAPI

Interactive view of the assembled MQTT contract (generated from per-component YAML).

- **[Open the AsyncAPI HTML](asyncapi/index.html)** — channels, operations, and payloads
- **[Raw spec (YAML)](asyncapi.yaml)** — for [AsyncAPI Studio](https://studio.asyncapi.com)

After GitHub Pages deploy, open Studio with:

`https://studio.asyncapi.com/?url=https://SiliconLabsSoftware.github.io/z-wave-protocol-controller/asyncapi.yaml`

Locally, run `bash scripts/mqtt_asyncapi/generate_html.sh` then `mkdocs serve`. The HTML and YAML under `docs/asyncapi/` are generated and not committed.
