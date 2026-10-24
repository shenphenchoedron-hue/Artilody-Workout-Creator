#!/usr/bin/env bash
# Copies the code that Artilody Workout Creator shares with Artilody Watts
# Trainer (GPL-3.0) into the shared/ folder of a standalone creator checkout
# (https://github.com/shenphenchoedron-hue/Artilody-Workout-Creator).
#
#   tools/sync_shared.sh <creator checkout> [Artilody Watts Trainer checkout]
#
# The second argument defaults to the repository this script lives in when it
# is run from "Artilody Workout Creator/tools" inside Artilody Watts Trainer.
# Only the files the creator actually compiles/bundles are copied, keeping the
# same relative layout (shared/src/core/..., shared/qml/...). Afterwards the
# target's CMake picks them up automatically (ARTILODY_SHARED_DIR).
set -euo pipefail

TARGET="${1:?usage: sync_shared.sh <creator checkout> [Artilody Watts Trainer checkout]}"
HERE="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE="${2:-$(cd "$HERE/.." && pwd)}"

if [ ! -f "$SOURCE/src/core/WorkoutIO.h" ]; then
    echo "error: $SOURCE is not an Artilody Watts Trainer checkout (src/core/WorkoutIO.h missing)" >&2
    exit 1
fi
if [ ! -f "$TARGET/CMakeLists.txt" ] || ! grep -q ArtilodyWorkoutCreator "$TARGET/CMakeLists.txt"; then
    echo "error: $TARGET is not an Artilody Workout Creator checkout" >&2
    exit 1
fi

FILES=(
    src/core/Interval.h src/core/Interval.cpp
    src/core/Workout.h src/core/Workout.cpp
    src/core/Zones.h src/core/Zones.cpp
    src/core/TimeFormat.h src/core/TimeFormat.cpp
    src/core/WorkoutIO.h src/core/WorkoutIO.cpp
    src/core/BuilderBackend.h
    src/core/WorkoutBuilder.h src/core/WorkoutBuilder.cpp
    qml/Theme.qml
    qml/components/Icon.qml
    qml/components/Card.qml
    qml/components/PageHeader.qml
    qml/components/AppButton.qml
    qml/components/IconButton.qml
    qml/components/AppTextField.qml
    qml/components/AppComboBox.qml
    qml/components/AppMenu.qml
    qml/components/AppMenuItem.qml
    qml/components/AppMenuSeparator.qml
    qml/components/StepperField.qml
    qml/components/UnitSpinBox.qml
    qml/components/ToggleSwitch.qml
    qml/components/CategoryBadge.qml
    qml/components/SportBadge.qml
    qml/components/StatTile.qml
    qml/components/ZoneLegend.qml
    qml/components/WorkoutChart.qml
    qml/components/SegmentedControl.qml
    qml/components/AppDialog.qml
    qml/components/Toast.qml
    qml/dialogs/ConfirmDialog.qml
    qml/pages/BuilderPage.qml
)

DEST="$TARGET/shared"
rm -rf "$DEST/src" "$DEST/qml"
for f in "${FILES[@]}"; do
    mkdir -p "$DEST/$(dirname "$f")"
    cp "$SOURCE/$f" "$DEST/$f"
done

REV="$(git -C "$SOURCE" rev-parse --short HEAD 2>/dev/null || echo unknown)"
cat > "$DEST/README.md" <<NOTE
# Shared code from Artilody Watts Trainer

The files in this folder are copied unchanged from
[Artilody Watts Trainer](https://github.com/shenphenchoedron-hue/Artilody-Watts-Trainer)
(GPL-3.0), so that the Workout Creator edits workouts with exactly the same
Builder and writes exactly the same \`.awt\` / \`.zwo\` files as the app.

**Do not edit them here.** Change them in Artilody Watts Trainer and refresh
this folder with:

\`\`\`sh
tools/sync_shared.sh <this checkout> <Artilody Watts Trainer checkout>
\`\`\`

Synced from Artilody Watts Trainer commit \`$REV\`.
NOTE

echo "Copied ${#FILES[@]} shared files from $SOURCE ($REV) to $DEST"
