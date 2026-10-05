#!/bin/bash
# SessionStart hook for Claude Code on the web: fetch the UNA Watch SDK into
# una-sdk/ (git-ignored, read-only; see CLAUDE.md) at the same commit CI pins
# (UNA_SDK_REF in .github/workflows/watch-builds.yml), and export UNA_SDK.
# Local machines keep their own SDK setup, so this does nothing off the web.
set -uo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

ROOT="${CLAUDE_PROJECT_DIR:-$(cd "$(dirname "$0")/../.." && pwd)}"
SDK="$ROOT/una-sdk"
URL="https://github.com/UNAWatch/una-sdk.git"
WORKFLOW="$ROOT/.github/workflows/watch-builds.yml"

REF="$(sed -n 's/^[[:space:]]*UNA_SDK_REF:[[:space:]]*\([0-9a-fA-F]\{7,40\}\).*/\1/p' "$WORKFLOW" | head -n 1)"
if [ -z "$REF" ]; then
  echo "session-start: no UNA_SDK_REF in $WORKFLOW; SDK not fetched." >&2
  exit 0
fi

fetch_sdk() {
  if [ "$(git -C "$SDK" rev-parse HEAD 2>/dev/null)" = "$REF" ]; then
    echo "session-start: una-sdk already at $REF"
    return 0
  fi
  if [ ! -d "$SDK/.git" ]; then
    rm -rf "$SDK"
    git init -q "$SDK" && git -C "$SDK" remote add origin "$URL" || return 1
  fi
  # Only the pinned commit is needed: fetch it alone, without history.
  git -C "$SDK" fetch -q --depth 1 origin "$REF" &&
    git -C "$SDK" checkout -q --force "$REF" &&
    git -C "$SDK" submodule update -q --init --recursive --depth 1 &&
    echo "session-start: una-sdk checked out at $REF"
}

for attempt in 1 2 3; do
  fetch_sdk && break
  if [ "$attempt" = 3 ]; then
    # Never block the session over the SDK; say so and carry on.
    echo "session-start: could not fetch una-sdk at $REF" >&2
    exit 0
  fi
  sleep $((attempt * 2))
done

if [ -n "${CLAUDE_ENV_FILE:-}" ]; then
  echo "export UNA_SDK=\"$SDK\"" >> "$CLAUDE_ENV_FILE"
fi
