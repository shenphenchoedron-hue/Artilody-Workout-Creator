#pragma once

#include "Interval.h"
#include "Zones.h"

#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

namespace watt {

// One coloured bar of a workout profile chart.
struct Segment
{
    int startSec = 0;
    int durationSec = 0;
    int watts = 0;
    int zone = 0;   // 0..5
};

// Sport a workout / ride belongs to. Stored as lower-case text in the DB
// ("cycling" is the default for everything created before schema v2).
namespace sport {
inline QString cycling() { return QStringLiteral("cycling"); }
inline QString rowing() { return QStringLiteral("rowing"); }
// Maps any input to a known sport; unknown/empty -> cycling.
QString normalize(const QString &value);
QStringList all();
} // namespace sport

// FTP selection for a sport: rowing workouts are prescribed as % of the
// rider's rowing FTP, everything else as % of the cycling FTP.
// Wave 2 (rowing UI / PM5 sessions) should always go through these helpers
// (or Profile::ftpFor()) instead of reading Profile::ftp directly.
int ftpForSport(const QString &sport, int ftp, int rowingFtp);
// Target watts of an interval at secondInInterval for the given sport.
int targetWatts(const Interval &interval, double secondInInterval, const QString &sport, int ftp, int rowingFtp);
inline int targetWatts(const Interval &interval, const QString &sport, int ftp, int rowingFtp)
{
    return targetWatts(interval, 0.0, sport, ftp, rowingFtp);
}

struct Workout
{
    qint64 id = -1;
    QString name;
    QString category = QStringLiteral("Intervals");   // Endurance, Intervals, Recovery, FTP Tests, Custom
    QString focus;          // short focus text, e.g. "Sweet Spot"
    QString description;
    bool favorite = false;
    bool builtIn = false;
    int timesUsed = 0;
    QString sport = sport::cycling();   // "cycling" or "rowing"
    QVector<Interval> intervals;

    bool isRowing() const { return sport == sport::rowing(); }

    int totalDurationSec() const;
    int intervalIndexAt(int second) const;   // -1 when outside the workout
    int intervalStartSec(int index) const;
    int targetWattsAt(double second, int ftp) const;

    double averagePercent() const;           // time weighted mean intensity
    double intensityFactor() const;          // 4th-power mean, like NP / FTP
    double estimatedTss() const;

    QString summary(int ftp) const;          // "3 × 12 min @ 225 W"
    QString intervalsLabel() const;          // "3 × 12 min"

    QVector<Segment> segments(int ftp, const ZoneSet &zones, int rampStepSec = 60) const;
    static QVariantList segmentsToVariant(const QVector<Segment> &segments);

    QJsonArray intervalsToJson() const;
    static QVector<Interval> intervalsFromJson(const QJsonArray &array);

    static QVector<Workout> builtInLibrary();

    // Workouts in this category are FTP tests (incl. personal copies).
    static QString ftpTestCategory() { return QStringLiteral("FTP Tests"); }
    bool isFtpTest() const { return category == ftpTestCategory(); }
    // The built-in FTP test is the basis for FTP testing and can never be deleted.
    bool isProtected() const { return builtIn && isFtpTest(); }
};

int zoneForInterval(const Interval &interval, double percent, const ZoneSet &zones);

} // namespace watt
