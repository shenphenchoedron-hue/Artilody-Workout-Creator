#!/bin/bash
# Builds a standalone, ad-hoc signed "Artilody Workout Creator.app" and a DMG
# in dist/ (inside the creator folder / repository).
#   packaging/macos/make_dmg.sh <Qt prefix, e.g. ~/Qt/6.8.3/macos> [build dir]
set -euo pipefail

QT="${1:?usage: make_dmg.sh <Qt prefix> [build dir]}"
HERE="$(cd "$(dirname "$0")/../.." && pwd)"          # creator folder
# Shared QML: parent repository (Artilody Watts Trainer) or vendored shared/.
if [ -f "$HERE/../src/core/WorkoutIO.h" ]; then
    ROOT="$(cd "$HERE/.." && pwd)"
else
    ROOT="$HERE/shared"
fi
BUILD="${2:-$HERE/build/release}"
VERSION="$(sed -n 's/^ *VERSION \([0-9.]*\)$/\1/p' "$HERE/CMakeLists.txt" | head -1)"
NAME="Artilody Workout Creator"
DIST="$HERE/dist"
APP="$DIST/$NAME.app"
DMG="$DIST/ArtilodyWorkoutCreator-$VERSION-macOS.dmg"

cmake -S "$HERE" -B "$BUILD" -DCMAKE_PREFIX_PATH="$QT" -DCMAKE_BUILD_TYPE=Release -DAWC_BUILD_TESTS=OFF
cmake --build "$BUILD" -j8

mkdir -p "$DIST"
rm -rf "$APP" "$DMG"
cp -R "$BUILD/$NAME.app" "$APP"
"$QT/bin/macdeployqt" "$APP" -qmldir="$HERE/qml" -qmldir="$ROOT/qml" -always-overwrite
# The creator uses no SQL and must never ship the offscreen platform plugin.
rm -rf "$APP/Contents/PlugIns/sqldrivers"
rm -f "$APP/Contents/PlugIns/platforms/libqoffscreen.dylib"
/usr/libexec/PlistBuddy -c "Set :LSMinimumSystemVersion 12.0" "$APP/Contents/Info.plist"
xattr -cr "$APP"
codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"

STAGE="$(mktemp -d)"
cp -R "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
hdiutil create -volname "$NAME" -srcfolder "$STAGE" -ov -format UDZO "$DMG"
rm -rf "$STAGE"
echo "App: $APP"
echo "DMG: $DMG"
