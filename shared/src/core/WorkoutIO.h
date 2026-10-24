#pragma once

#include "Workout.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

// Sharing of workouts between users: export/import of the native ".awt"
// format (UTF-8 JSON) and Zwift ".zwo" files (XML). Pure functions, no DB.
//
// .awt layout (version 1):
//   { "format": "artilody-watts-trainer/workout", "version": 1,
//     "exportedAt": "<ISO-8601 UTC>", "app": "Artilody Watts Trainer <ver>",
//     "workouts": [ { "name", "category", "focus", "description",
//                     "sport": "cycling"|"rowing",
//                     "intervals": [ Interval::toJson() objects ] } ] }
// "sport" was added without bumping the format version (older readers ignore
// it); a missing/unknown sport is read as "cycling".
//
// .zwo: Zwift only knows sportType "bike" and "run". Rowing workouts can be
// exported, but are written with sportType "bike" (their intervals are % FTP
// like bike workouts), so the sport is lost on a .zwo round trip. Imported
// .zwo files are always cycling workouts.
// Intensities are fractions of FTP, so shared workouts scale to the
// importing rider's FTP.
namespace watt::workoutio {

inline constexpr const char *kFormatId = "artilody-watts-trainer/workout";
inline constexpr int kFormatVersion = 1;

// Import limits (files beyond these are rejected or sanitised).
inline constexpr qint64 kMaxFileBytes = 2 * 1024 * 1024;
inline constexpr int kMaxWorkouts = 500;
inline constexpr int kMaxIntervals = 2000;          // per workout
inline constexpr int kMinIntervalSec = 5;
inline constexpr int kMaxIntervalSec = 6 * 3600;
inline constexpr int kMaxWorkoutSec = 12 * 3600;
inline constexpr double kMaxFraction = 4.0;         // 400 % FTP
inline constexpr int kMaxCadence = 200;

enum class Format { Awt, Zwo };

struct ImportResult
{
    QVector<Workout> workouts;   // sanitised; id = -1, builtIn/favorite = false
    QStringList warnings;        // non-fatal adjustments, human readable
    QString error;               // non-empty when nothing could be imported
    bool ok() const { return error.isEmpty() && !workouts.isEmpty(); }
};

// appName: written to the informational "app" field (.awt) / <author> (.zwo);
// empty = "Artilody Watts Trainer". It does not affect importing.
QByteArray toAwt(const QVector<Workout> &workouts, const QString &appVersion, const QString &appName = QString());
QByteArray toZwo(const Workout &workout, const QString &appName = QString());

ImportResult parseAwt(const QByteArray &data);
ImportResult parseZwo(const QByteArray &data);
// Detects the format by content (JSON "{"/"[" or XML "<"), falling back to
// the file name extension (".awt"/".json" or ".zwo"/".xml").
ImportResult parseData(const QByteArray &data, const QString &fileName = QString());
ImportResult parseFile(const QString &path);

// Categories an imported workout may have (same as the Builder offers).
QStringList allowedCategories();
// Format from a file name: ".zwo"/".xml" -> Zwo, everything else -> Awt.
Format formatForFileName(const QString &fileName);
// Turns a workout name into a safe file name (without extension).
QString safeFileName(const QString &name);

// Writes binary data atomically (QSaveFile). Returns false and sets error on failure.
bool writeFile(const QString &path, const QByteArray &data, QString *error = nullptr);

} // namespace watt::workoutio
