#!/usr/bin/env bash
# Build HybridX Route Sender (test build) without Gradle or Android Studio.
#
#   hybridx-trail/Tools/RouteSender/build.sh
#
# Runs the host tests, then compiles, dexes, packages and signs the APK into
# build/HybridXRouteSender-<version>.apk.
#
# Tools, in order of preference:
#   - a full Android SDK at $ANDROID_HOME (GitHub's ubuntu runners, Android
#     Studio): the newest platform's android.jar and build-tools' aapt2, d8,
#     zipalign and apksigner;
#   - otherwise Ubuntu's own packages, for a container that can't reach
#     dl.google.com:
#       apt-get install android-sdk-platform-23 aapt apksigner zipalign dalvik-exchange
#     (API 23's android.jar and dx instead of d8).
#
# The source only uses API 23 calls (newer ones by reflection, WatchLink.java)
# so it builds either way. Environment: VERSION_NAME (default 0.1.1),
# VERSION_CODE (default 2), OUT (default ./build).
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="${OUT:-$HERE/build}"
VERSION_NAME="${VERSION_NAME:-0.1.1}"
VERSION_CODE="${VERSION_CODE:-2}"
MIN_SDK=26
TARGET_SDK=34

if [ -n "${ANDROID_HOME:-}" ] && ls "$ANDROID_HOME"/platforms/android-*/android.jar >/dev/null 2>&1; then
    PLATFORM="$(ls -d "$ANDROID_HOME"/platforms/android-* | sort -V | tail -n 1)"
    BUILD_TOOLS="$(ls -d "$ANDROID_HOME"/build-tools/* | sort -V | tail -n 1)"
    ANDROID_JAR="$PLATFORM/android.jar"
    AAPT2="$BUILD_TOOLS/aapt2"
    D8="$BUILD_TOOLS/d8"
    ZIPALIGN="$BUILD_TOOLS/zipalign"
    APKSIGNER="$BUILD_TOOLS/apksigner"
else
    ANDROID_JAR=/usr/lib/android-sdk/platforms/android-23/android.jar
    AAPT2=aapt2
    D8=""
    ZIPALIGN=zipalign
    APKSIGNER=apksigner
fi
[ -f "$ANDROID_JAR" ] || { echo "No android.jar (see the comment at the top of build.sh)" >&2; exit 1; }
echo "android.jar: $ANDROID_JAR"

rm -rf "$OUT"
mkdir -p "$OUT/host" "$OUT/classes" "$OUT/dex"
SRC="$HERE/app/src/club/hybridx/routesender"

echo "== Host tests"
javac -encoding UTF-8 -d "$OUT/host" "$SRC/Fts.java" "$SRC/RouteFile.java" \
    "$HERE/tests/club/hybridx/routesender/HostTests.java"
java -cp "$OUT/host" club.hybridx.routesender.HostTests

echo "== Resources and manifest"
"$AAPT2" compile --dir "$HERE/app/res" -o "$OUT/res.zip"
"$AAPT2" link -o "$OUT/unsigned.apk" -I "$ANDROID_JAR" \
    --manifest "$HERE/app/AndroidManifest.xml" \
    --min-sdk-version "$MIN_SDK" --target-sdk-version "$TARGET_SDK" \
    --version-code "$VERSION_CODE" --version-name "$VERSION_NAME" \
    "$OUT/res.zip"

echo "== Java"
# Java 8 bytecode, and no lambdas in the source: dx can't desugar them.
javac -encoding UTF-8 -source 8 -target 8 -Xlint:-options -bootclasspath "$ANDROID_JAR" \
    -d "$OUT/classes" "$SRC"/*.java

echo "== Dex"
if [ -n "$D8" ]; then
    "$D8" --release --min-api "$MIN_SDK" --lib "$ANDROID_JAR" --output "$OUT/dex" \
        $(find "$OUT/classes" -name '*.class')
else
    dalvik-exchange --dex --min-sdk-version="$MIN_SDK" --output="$OUT/dex/classes.dex" "$OUT/classes"
fi
(cd "$OUT/dex" && zip -q -j "$OUT/unsigned.apk" classes.dex)

echo "== Align and sign"
APK="$OUT/HybridXRouteSender-$VERSION_NAME.apk"
"$ZIPALIGN" -f 4 "$OUT/unsigned.apk" "$OUT/aligned.apk"
# A test-only key, kept in the repo so every build installs over the last one.
# A store release needs its own key, kept out of the repo.
"$APKSIGNER" sign --ks "$HERE/debug.keystore" --ks-pass pass:android --key-pass pass:android \
    --ks-key-alias androiddebugkey --out "$APK" "$OUT/aligned.apk"
"$APKSIGNER" verify "$APK"
echo "Built $APK ($(stat -c %s "$APK") bytes)"
