#!/bin/bash
#
# Build the portal upload package for HybridX Race.
#
# The layout is the one Docs/deploy.md specifies: the .uapp, app-manifest.json,
# icon.png, and a previews/ folder of screenshots. Everything the manifest says
# about the binary is taken FROM the binary, so appVersion and the filename can
# never drift apart, and both SDK validators must pass before a zip is written.
#
# Usage:
#   UNA_SDK=/path/to/una-sdk ./pack-store-zip.sh [--expect-version X.Y.Z]
#
# --expect-version asserts; it does not override. A release build is expected to
# say "./pack-store-zip.sh --expect-version 0.1.0" and have the script refuse if
# the binary in Output/ is not that version -- which is what catches a forgotten
# tag or a stale build, the two ways a package ends up describing a binary it
# does not contain.
#
# Build the app first:
#   cmake -S Software/Apps/HybridXRace-CMake -B build && cmake --build build
#
set -euo pipefail

APP=$(cd "$(dirname "$0")/.." && pwd)
OUT="$APP/Output"
STAGE="$OUT/Release"
PACKER="${UNA_SDK:-}/Utilities/Scripts/app_packer"

EXPECT_VERSION=""
while [ $# -gt 0 ]; do
    case "$1" in
        --expect-version) EXPECT_VERSION="${2:-}"; shift 2 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

if [ -z "${UNA_SDK:-}" ]; then
    echo "UNA_SDK is not set." >&2
    exit 1
fi
for tool in min_kernel_version.py validate_app_config.py; do
    if [ ! -f "$PACKER/$tool" ]; then
        echo "Not in this SDK checkout: $PACKER/$tool" >&2
        exit 1
    fi
done

# -- The binary decides the version ------------------------------------------

mapfile -t UAPPS < <(find "$OUT" -maxdepth 1 -name '*.uapp' -printf '%f\n' | sort)
case ${#UAPPS[@]} in
    0) echo "No .uapp in $OUT -- build the app first." >&2; exit 1 ;;
    1) ;;
    *) echo "More than one .uapp in $OUT; delete the stale ones:" >&2
       printf '  %s\n' "${UAPPS[@]}" >&2; exit 1 ;;
esac
BINARY="${UAPPS[0]}"

# HybridXRace_0.1.0.uapp -> 0.1.0, HybridXRace_0.0.0-dev.uapp -> 0.0.0-dev.
VERSION=$(printf '%s' "$BINARY" | sed -n 's/^HybridXRace_\(.*\)\.uapp$/\1/p')
if [ -z "$VERSION" ]; then
    echo "Cannot read a version out of '$BINARY'." >&2
    exit 1
fi
if [ -n "$EXPECT_VERSION" ] && [ "$EXPECT_VERSION" != "$VERSION" ]; then
    echo "Expected version $EXPECT_VERSION but $BINARY is $VERSION." >&2
    echo "Tag the commit apps-v$EXPECT_VERSION and rebuild, or build with" >&2
    echo "-DBUILD_VERSION=$EXPECT_VERSION." >&2
    exit 1
fi
case "$VERSION" in
    *-dev|*-dirty|*-g*)
        echo "NOTE: '$VERSION' is a development build, so this package is a dry run."
        echo "      The version comes from an apps-v* git tag; see"
        echo "      Utilities/Scripts/build-cube/una-version.sh in the SDK."
        ;;
esac

# -- The ID in the binary is the ID in the manifest ---------------------------
# APP_ID is baked into the .uapp by CMake and named again in the manifest. After
# the portal issues the real one it has to be pasted in two places, and the
# portal matches new versions to the app by it (Docs/deploy.md), so one place
# updated and one forgotten is exactly the mistake worth refusing.
DEV_APP_ID="8C345EF26E3350E7"
CMAKE_ID=$(sed -n 's/^set(APP_ID "\([0-9A-Fa-f]\{16\}\)").*/\1/p' \
           "$APP/Software/Apps/HybridXRace-CMake/CMakeLists.txt" | head -n 1)
MANIFEST_ID=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["id"])' \
              "$APP/Resources/app-manifest.json")
if [ -z "$CMAKE_ID" ]; then
    echo "Cannot read APP_ID out of HybridXRace-CMake/CMakeLists.txt." >&2
    exit 1
fi
if [ "$CMAKE_ID" != "$MANIFEST_ID" ]; then
    echo "APP_ID differs: CMakeLists.txt has $CMAKE_ID, app-manifest.json has $MANIFEST_ID." >&2
    echo "Paste the portal's App ID into both, re-run cmake and rebuild." >&2
    exit 1
fi
if [ -n "$EXPECT_VERSION" ] && [ "$MANIFEST_ID" = "$DEV_APP_ID" ]; then
    echo "A release must carry the App ID the portal issued, not the development ID" >&2
    echo "$DEV_APP_ID. Create the app on apps.unawatch.com first (README, 'Building" >&2
    echo "the store package')." >&2
    exit 1
fi

# -- Stage -------------------------------------------------------------------

rm -rf "$STAGE"
mkdir -p "$STAGE/previews"

cp "$OUT/$BINARY" "$STAGE/$BINARY"
cp "$APP/Resources/icon_60x60.png" "$STAGE/icon.png"

shopt -s nullglob
PREVIEWS=("$APP/Resources/previews"/*.png)
shopt -u nullglob
if [ ${#PREVIEWS[@]} -eq 0 ]; then
    echo "No previews in Resources/previews -- see docs/experiments/capture_screens.sh." >&2
    exit 1
fi
cp "${PREVIEWS[@]}" "$STAGE/previews/"

# The manifest names the binary and the version it was built from, not what a
# human last typed into it.
python3 - "$APP/Resources/app-manifest.json" "$STAGE/app-manifest.json" \
         "$BINARY" "$VERSION" <<'PY'
import json, sys
src, dst, binary, version = sys.argv[1:5]
with open(src) as f:
    manifest = json.load(f)
manifest["binary"] = binary
manifest["appVersion"] = version
with open(dst, "w") as f:
    json.dump(manifest, f, indent=2)
    f.write("\n")
PY

# -- Validate, then and only then zip ----------------------------------------

python3 "$PACKER/min_kernel_version.py" --stamp "$STAGE/app-manifest.json"
python3 "$PACKER/min_kernel_version.py" --check "$STAGE/app-manifest.json"
python3 "$PACKER/validate_app_config.py" \
        --check "$STAGE/app-manifest.json" \
        --check-bounds "$APP/Software/Libs/Sources/AppConfigFields.cpp" \
        --check-bounds "$APP/Software/Libs/Header/AppConfigFields.hpp"

ZIP="$OUT/HybridXRace-$VERSION.zip"
rm -f "$ZIP"
if command -v zip >/dev/null 2>&1; then
    (cd "$STAGE" && zip -q -r "$ZIP" .)
else
    # The CI image has Python and not always zip. Same layout: everything at the
    # root of the archive, previews/ as a folder.
    (cd "$STAGE" && python3 -m zipfile -c "$ZIP" ./*)
fi

echo
echo "Package: $ZIP"
if command -v unzip >/dev/null 2>&1; then
    unzip -l "$ZIP"
else
    python3 -m zipfile -l "$ZIP"
fi
echo
if [ "$MANIFEST_ID" = "$DEV_APP_ID" ]; then
    echo "Before uploading: the id in the manifest is a development APP_ID."
    echo "Create the app on apps.unawatch.com, paste its App ID into"
    echo "Software/Apps/HybridXRace-CMake/CMakeLists.txt and Resources/app-manifest.json,"
    echo "re-run cmake, rebuild, and re-run this script."
else
    echo "App ID $MANIFEST_ID matches in CMakeLists.txt and app-manifest.json."
fi
