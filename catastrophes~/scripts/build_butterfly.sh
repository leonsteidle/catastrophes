#!/usr/bin/env bash
set -euo pipefail

# Build within this package only. This script does not install or publish.
TASK_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TASK_PACKAGE_DIR="$(cd "$TASK_SCRIPT_DIR/.." && pwd)"
: "${MAXSDK_PATH:?Set MAXSDK_PATH to the root of the Cycling 74 max-sdk checkout}"
exec make -C "$TASK_PACKAGE_DIR/src/butterfly~" MAXSDK_PATH="$MAXSDK_PATH" DEPLOYMENT_TARGET=15.0 all
