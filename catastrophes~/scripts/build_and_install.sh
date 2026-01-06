#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

die() { echo "ERROR: $*" >&2; exit 1; }

# Where the repo stores prebuilt binaries
REPO_EXTERNALS_DIR="$ROOT_DIR/externals"
[[ -d "$REPO_EXTERNALS_DIR" ]] || die "Cannot find '$REPO_EXTERNALS_DIR' (run this script from inside the catastrophes~ folder)."

# Detect a reasonable default Max Packages folder (Max 9 → 8 → 7)
detect_packages_dir() {
  local v p
  for v in 9 8 7; do
    p="$HOME/Documents/Max $v/Packages"
    if [[ -d "$p" ]]; then
      echo "$p"
      return 0
    fi
  done
  # Fallback (folder may not exist yet)
  echo "$HOME/Documents/Max 9/Packages"
}

DEFAULT_MAX_PACKAGES="$(detect_packages_dir)"
DEFAULT_INSTALL_DIR="$DEFAULT_MAX_PACKAGES/catastrophes~/externals"

# Override with: INSTALL_DIR=/path ./build_and_install.sh
INSTALL_DIR="${INSTALL_DIR:-$DEFAULT_INSTALL_DIR}"

# Convenience: allow INSTALL_DIR to point to Packages/ or to the package root
if [[ "$INSTALL_DIR" == */Packages ]]; then
  INSTALL_DIR="$INSTALL_DIR/catastrophes~/externals"
elif [[ "$INSTALL_DIR" == */catastrophes~ ]]; then
  INSTALL_DIR="$INSTALL_DIR/externals"
fi

mkdir -p "$INSTALL_DIR"

OBJECTS=( "fold~" "cusp~" "swallowtail~" "butterfly~" )

echo "Installing catastrophes~ externals to:"
echo "  $INSTALL_DIR"
echo

for obj in "${OBJECTS[@]}"; do
  SRC_REPO="$REPO_EXTERNALS_DIR/$obj.mxo"
  [[ -d "$SRC_REPO" ]] || die "Missing binary: $SRC_REPO"

  DST_MAX="$INSTALL_DIR/$obj.mxo"
  rm -rf "$DST_MAX"
  ditto "$SRC_REPO" "$DST_MAX" || die "Failed to install $obj"

  # If the bundle is quarantined by Gatekeeper, Max may refuse to load it.
  # This is safe to ignore if xattr is not available or if nothing is quarantined.
  xattr -dr com.apple.quarantine "$DST_MAX" 2>/dev/null || true

  echo "  ✓ $obj installed"
done

echo
echo "Done."
