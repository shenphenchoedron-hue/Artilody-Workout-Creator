#!/bin/bash
# Removes what macdeployqt bundles but Artilody Workout Creator never uses:
# SQL drivers, QtQuick.LocalStorage and the QtSql framework they pull in.
# Fails if anything left in the bundle still links QtSql.
#   packaging/macos/prune_bundle.sh "<path>/Artilody Workout Creator.app"
set -euo pipefail
APP="${1:?usage: prune_bundle.sh <app bundle>}"
C="$APP/Contents"

rm -rf "$C/PlugIns/sqldrivers"
rm -f  "$C/PlugIns/quick/libqmllocalstorageplugin.dylib"
rm -rf "$C/Resources/qml/QtQuick/LocalStorage"
rm -rf "$C/Frameworks/QtQmlLocalStorage.framework"
rm -rf "$C/Frameworks/QtSql.framework"

left=0
while IFS= read -r -d '' f; do
    if file "$f" | grep -q Mach-O && otool -L "$f" | grep -q "QtSql.framework"; then
        echo "error: ${f#$APP/} still links QtSql" >&2
        left=1
    fi
done < <(find "$C" -type f -print0)
exit $left
