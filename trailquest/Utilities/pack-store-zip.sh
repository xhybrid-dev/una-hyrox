#!/bin/bash
#
# Build the portal upload package for Trail Quest. A copy of X Weigh-in's
# (x-weighin/Utilities/pack-store-zip.sh, itself HybridX Race's), with
# Trail Quest's names and paths and without the glance, which Trail Quest does not
# have. The simulator's APP_ID is checked too.
#
# One store listing, one package: the app.
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
# say "./pack-store-zip.sh --expect-version 1.0.0" and have the script refuse if
# the binary in Output/ is not that version -- which is what catches a forgotten
# tag or a stale build, the two ways a package ends up describing a binary it
# does not contain.
#
# Build the app first:
#   cmake -S Software/Apps/TrailQuest-CMake -B build && cmake --build build
#
set -euo pipefail

APP=$(cd "$(dirname "$0")/.." && pwd)
PACKER="${UNA_SDK:-}/Utilities/Scripts/app_packer"

EXPECT_VERSION=""
while [ $# -gt 0 ]; do
    case "$1" in
        --expect-version) EXPECT_VERSION="${2:-}"; shift 2 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

# What this package is.
NAME="TrailQuest"
OUT="$APP/Output"
RES="$APP/Resources"
CMAKE_LISTS="$APP/Software/Apps/TrailQuest-CMake/CMakeLists.txt"
SIM_LISTS="$APP/Software/Apps/LVGL-GUI/simulator/CMakeLists.txt"
DEV_APP_ID="B5FA567FFD85BA6D"                      # first 16 hex of md5("TrailQuest")
STAGE="$OUT/Release"

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

# TrailQuest_1.0.0.uapp -> 1.0.0, TrailQuest_0.0.0-dev.uapp -> 0.0.0-dev.
VERSION=$(printf '%s' "$BINARY" | sed -n "s/^${NAME}_\(.*\)\.uapp\$/\1/p")
if [ -z "$VERSION" ]; then
    echo "Cannot read a version out of '$BINARY'." >&2
    exit 1
fi
if [ -n "$EXPECT_VERSION" ] && [ "$EXPECT_VERSION" != "$VERSION" ]; then
    echo "Expected version $EXPECT_VERSION but $BINARY is $VERSION." >&2
    echo "Tag the commit trailquest-v$EXPECT_VERSION and rebuild, or build with" >&2
    echo "-DBUILD_VERSION=$EXPECT_VERSION." >&2
    exit 1
fi
case "$VERSION" in
    *-dev|*-dirty|*-g*)
        echo "NOTE: '$VERSION' is a development build, so this package is a dry run."
        echo "      The version comes from a trailquest-v* git tag; see"
        echo "      Software/cmake/trailquest-version.cmake."
        ;;
esac

# -- The ID in the binary is the ID in the manifest ---------------------------
# APP_ID is baked into the .uapp by CMake and named again in the manifest and
# the simulator's build. After the portal issues the real one it has to be
# pasted in all three, and the
# portal matches new versions to the app by it (Docs/deploy.md), so one place
# updated and one forgotten is exactly the mistake worth refusing.
CMAKE_ID=$(sed -n 's/^set(APP_ID "\([0-9A-Fa-f]\{16\}\)").*/\1/p' \
           "$CMAKE_LISTS" | head -n 1)
SIM_ID=$(sed -n 's/.*APP_ID="\([0-9A-Fa-f]\{16\}\)".*/\1/p' "$SIM_LISTS" | head -n 1)
MANIFEST_ID=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["id"])' \
              "$RES/app-manifest.json")
if [ -z "$CMAKE_ID" ]; then
    echo "Cannot read APP_ID out of $CMAKE_LISTS." >&2
    exit 1
fi
if [ "$CMAKE_ID" != "$MANIFEST_ID" ]; then
    echo "APP_ID differs: CMakeLists.txt has $CMAKE_ID, app-manifest.json has $MANIFEST_ID." >&2
    echo "Paste the portal's App ID into both, re-run cmake and rebuild." >&2
    exit 1
fi
if [ "$CMAKE_ID" != "$SIM_ID" ]; then
    echo "APP_ID differs: CMakeLists.txt has $CMAKE_ID, the simulator's has '$SIM_ID'." >&2
    exit 1
fi
if [ -n "$EXPECT_VERSION" ] && [ "$MANIFEST_ID" = "$DEV_APP_ID" ]; then
    echo "A release must carry the App ID the portal issued, not the development ID" >&2
    echo "$DEV_APP_ID. Create the app on apps.unawatch.com first (docs/SUBMISSION.md)." >&2
    exit 1
fi

# -- Stage -------------------------------------------------------------------

rm -rf "$STAGE"
mkdir -p "$STAGE/previews"

cp "$OUT/$BINARY" "$STAGE/$BINARY"
cp "$APP/Resources/icon_60x60.png" "$STAGE/icon.png"

shopt -s nullglob
PREVIEWS=("$RES/previews"/*.png)
shopt -u nullglob
if [ ${#PREVIEWS[@]} -eq 0 ]; then
    echo "No previews in $RES/previews -- see docs/experiments/store_previews.sh." >&2
    exit 1
fi
cp "${PREVIEWS[@]}" "$STAGE/previews/"

# The manifest names the binary and the version it was built from, not what a
# human last typed into it.
python3 - "$RES/app-manifest.json" "$STAGE/app-manifest.json" \
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

# -- The portal's own manifest check ----------------------------------------
# The UNA portal (apps.unawatch.com) validates the manifest more strictly than
# the SDK's tools. It refused 1.0.0 (13 October 2026, x-weighin/docs/NOTES.md, W5):
#   - it requires the activity keys for every app type, so a utility
#     still carries them, set to "not used" (Docs/app-config-json.md's Files
#     example has the same set);
#   - it allows no other keys: one was extra, and "previews" is the only key
#     ours shared that no SDK example carries. The previews/ folder itself
#     passed ("Valid assets").
python3 - "$STAGE/app-manifest.json" <<'PY'
import json, sys
m = json.load(open(sys.argv[1]))
need = ["supportsLaps", "supportsDistance", "supportsTrack", "supportsHeartbeat", "supportsElevation",
        "supportsStep", "supportsSpeed", "customMeasures", "stravaExport"]
missing = [k for k in need if k not in m]
banned = [k for k in ("previews",) if k in m]
if missing or banned:
    sys.exit("The portal would refuse this manifest: missing %s, not allowed %s" % (missing, banned))
PY

# -- Validate, then and only then zip ----------------------------------------

python3 "$PACKER/min_kernel_version.py" --stamp "$STAGE/app-manifest.json"
python3 "$PACKER/min_kernel_version.py" --check "$STAGE/app-manifest.json"
# Trail Quest has no phone settings, so there is no field table to compare: the
# validator checks the manifest declares none that could mismatch.
python3 "$PACKER/validate_app_config.py" --check "$STAGE/app-manifest.json"

ZIP="$OUT/$NAME-$VERSION.zip"
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
    echo "$CMAKE_LISTS"
    echo "and $RES/app-manifest.json and $SIM_LISTS,"
    echo "re-run cmake, rebuild, and re-run this script."
else
    echo "App ID $MANIFEST_ID matches in CMakeLists.txt and app-manifest.json and the simulator."
fi
