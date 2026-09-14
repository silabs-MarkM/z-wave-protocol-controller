#!/usr/bin/env bash
# Generate single-file AsyncAPI HTML + YAML into docs/ (gitignored).
# Requires build/asyncapi.assembled.yaml (or pass a path as $1).
set -euo pipefail
cd "$(dirname "$0")/../.."
ASSEMBLED="${1:-build/asyncapi.assembled.yaml}"
if [[ ! -f "$ASSEMBLED" ]]; then
  echo "missing assembled spec: $ASSEMBLED" >&2
  exit 1
fi
mkdir -p docs/asyncapi
npx --yes @asyncapi/cli@6.0.2 generate fromTemplate \
  "$ASSEMBLED" \
  @asyncapi/html-template@3.5.6 \
  --output docs/asyncapi \
  --force-write \
  --param singleFile=true \
  --param sidebarOrganization=byTagsNoRoot \
  --param config='{"show":{"messages":false},"expand":{"messageExamples":true}}'
cp "$ASSEMBLED" docs/asyncapi.yaml
