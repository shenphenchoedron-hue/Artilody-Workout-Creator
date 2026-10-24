#pragma once

#include "Workout.h"

#include <QString>
#include <QVector>

namespace awc::demo {

// A few sample workouts (cycling + rowing) for screenshot mode and tests.
QVector<watt::Workout> workouts();
// Writes them into folder (.awt files, plus one Zwift .zwo). Returns the number of files.
int seedFolder(const QString &folder);

} // namespace awc::demo
