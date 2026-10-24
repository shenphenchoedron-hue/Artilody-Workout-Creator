#include "Workout.h"

#include <QJsonObject>
#include <QVariantMap>
#include <algorithm>
#include <cmath>

namespace watt {

namespace sport {
QString normalize(const QString &value)
{
    const QString v = value.trimmed().toLower();
    if (v == QLatin1String("rowing") || v == QLatin1String("row") || v == QLatin1String("rower"))
        return rowing();
    return cycling();
}

QStringList all()
{
    return { cycling(), rowing() };
}
} // namespace sport

int ftpForSport(const QString &s, int ftp, int rowingFtp)
{
    return sport::normalize(s) == sport::rowing() ? rowingFtp : ftp;
}

int targetWatts(const Interval &interval, double secondInInterval, const QString &s, int ftp, int rowingFtp)
{
    return interval.wattsAt(secondInInterval, ftpForSport(s, ftp, rowingFtp));
}

int zoneForInterval(const Interval &interval, double percent, const ZoneSet &zones)
{
    if (interval.rpeZone >= 0)
        return std::min(interval.rpeZone, kZoneCount - 1);
    return zones.zoneForFraction(percent);
}

int Workout::totalDurationSec() const
{
    int total = 0;
    for (const Interval &i : intervals)
        total += i.durationSec;
    return total;
}

int Workout::intervalIndexAt(int second) const
{
    if (second < 0)
        return -1;
    int start = 0;
    for (int idx = 0; idx < intervals.size(); ++idx) {
        start += intervals.at(idx).durationSec;
        if (second < start)
            return idx;
    }
    return -1;
}

int Workout::intervalStartSec(int index) const
{
    int start = 0;
    for (int idx = 0; idx < index && idx < intervals.size(); ++idx)
        start += intervals.at(idx).durationSec;
    return start;
}

int Workout::targetWattsAt(double second, int ftp) const
{
    double start = 0;
    for (const Interval &i : intervals) {
        if (second < start + i.durationSec)
            return i.wattsAt(second - start, ftp);
        start += i.durationSec;
    }
    return 0;
}

double Workout::averagePercent() const
{
    const int total = totalDurationSec();
    if (total <= 0)
        return 0.0;
    double sum = 0.0;
    for (const Interval &i : intervals)
        sum += i.averagePercent() * i.durationSec;
    return sum / total;
}

double Workout::intensityFactor() const
{
    const int total = totalDurationSec();
    if (total <= 0)
        return 0.0;
    double sum = 0.0;
    for (const Interval &i : intervals) {
        // integrate per second so ramps are handled correctly
        for (int s = 0; s < i.durationSec; ++s)
            sum += std::pow(i.percentAt(s + 0.5), 4.0);
    }
    return std::pow(sum / total, 0.25);
}

double Workout::estimatedTss() const
{
    const double ifactor = intensityFactor();
    return totalDurationSec() / 3600.0 * ifactor * ifactor * 100.0;
}

namespace {
bool isWorkBlock(const Interval &i)
{
    return i.type == BlockType::Interval || i.type == BlockType::Steady || i.type == BlockType::Ramp;
}

QString minutesText(int sec)
{
    if (sec % 60 == 0)
        return QStringLiteral("%1 min").arg(sec / 60);
    if (sec < 60)
        return QStringLiteral("%1 s").arg(sec);
    return QStringLiteral("%1:%2 min").arg(sec / 60).arg(sec % 60, 2, 10, QLatin1Char('0'));
}

// Finds the main work block and how many times it is repeated.
int mainBlock(const QVector<Interval> &intervals, int *count)
{
    int best = -1;
    for (int idx = 0; idx < intervals.size(); ++idx) {
        const Interval &i = intervals.at(idx);
        if (!isWorkBlock(i))
            continue;
        if (best < 0 || i.averagePercent() > intervals.at(best).averagePercent() + 1e-6)
            best = idx;
    }
    if (best < 0) {
        for (int idx = 0; idx < intervals.size(); ++idx) {
            if (best < 0 || intervals.at(idx).durationSec > intervals.at(best).durationSec)
                best = idx;
        }
    }
    int n = 0;
    if (best >= 0) {
        const Interval &b = intervals.at(best);
        for (const Interval &i : intervals) {
            if (i.durationSec == b.durationSec && std::abs(i.startPercent - b.startPercent) < 1e-6
                && std::abs(i.endPercent - b.endPercent) < 1e-6)
                ++n;
        }
    }
    if (count)
        *count = n;
    return best;
}
} // namespace

QString Workout::summary(int ftp) const
{
    int count = 0;
    const int idx = mainBlock(intervals, &count);
    if (idx < 0)
        return QStringLiteral("Empty workout");
    const Interval &b = intervals.at(idx);
    if (b.isRamp()) {
        return QStringLiteral("Ramp %1 – %2 W")
            .arg(b.wattsAt(0, ftp)).arg(b.wattsAt(b.durationSec, ftp));
    }
    const QString watts = QStringLiteral("%1 W").arg(b.wattsAt(0, ftp));
    if (count > 1)
        return QStringLiteral("%1 × %2 @ %3").arg(count).arg(minutesText(b.durationSec), watts);
    return QStringLiteral("%1 @ %2").arg(minutesText(b.durationSec), watts);
}

QString Workout::intervalsLabel() const
{
    int count = 0;
    const int idx = mainBlock(intervals, &count);
    if (idx < 0)
        return QStringLiteral("–");
    const Interval &b = intervals.at(idx);
    if (count > 1)
        return QStringLiteral("%1 × %2").arg(count).arg(minutesText(b.durationSec));
    return minutesText(b.durationSec);
}

QVector<Segment> Workout::segments(int ftp, const ZoneSet &zones, int rampStepSec) const
{
    QVector<Segment> out;
    int start = 0;
    rampStepSec = std::max(5, rampStepSec);
    for (const Interval &i : intervals) {
        if (!i.isRamp()) {
            out.append({ start, i.durationSec, i.wattsAt(0, ftp), zoneForInterval(i, i.startPercent, zones) });
        } else {
            const int steps = std::max(1, int(std::ceil(i.durationSec / double(rampStepSec))));
            int offset = 0;
            for (int s = 0; s < steps; ++s) {
                const int len = (s == steps - 1) ? i.durationSec - offset : std::min(rampStepSec, i.durationSec - offset);
                if (len <= 0)
                    break;
                const double pct = i.percentAt(offset + len / 2.0);
                out.append({ start + offset, len, int(std::lround(pct * ftp)), zoneForInterval(i, pct, zones) });
                offset += len;
            }
        }
        start += i.durationSec;
    }
    return out;
}

QVariantList Workout::segmentsToVariant(const QVector<Segment> &segments)
{
    QVariantList list;
    list.reserve(segments.size());
    for (const Segment &s : segments) {
        QVariantMap m;
        m.insert(QStringLiteral("start"), s.startSec);
        m.insert(QStringLiteral("duration"), s.durationSec);
        m.insert(QStringLiteral("watts"), s.watts);
        m.insert(QStringLiteral("zone"), s.zone);
        list.append(m);
    }
    return list;
}

QJsonArray Workout::intervalsToJson() const
{
    QJsonArray a;
    for (const Interval &i : intervals)
        a.append(i.toJson());
    return a;
}

QVector<Interval> Workout::intervalsFromJson(const QJsonArray &array)
{
    QVector<Interval> out;
    out.reserve(array.size());
    for (const QJsonValue &v : array)
        out.append(Interval::fromJson(v.toObject()));
    return out;
}

// ---------------------------------------------------------------------------
// Built-in workouts (seeded into the database on first start)
// ---------------------------------------------------------------------------
namespace {
Interval block(BlockType t, const QString &name, int minutes, double start, double end = -1, int cadence = 0)
{
    Interval i;
    i.type = t;
    i.name = name;
    i.durationSec = int(minutes * 60);
    i.startPercent = start;
    i.endPercent = end < 0 ? start : end;
    i.cadence = cadence;
    return i;
}
} // namespace

QVector<Workout> Workout::builtInLibrary()
{
    QVector<Workout> lib;

    {
        Workout w;
        w.name = QStringLiteral("Sweet Spot 3 × 12");
        w.category = QStringLiteral("Intervals");
        w.focus = QStringLiteral("Sweet Spot");
        w.description = QStringLiteral("Three 12-minute sweet spot efforts at 88–92% FTP with recovery intervals. "
                                       "A great workout to build sustained power and improve aerobic capacity.");
        w.intervals = {
            block(BlockType::WarmUp, QStringLiteral("Warm Up"), 6, 0.40, 0.65),
            block(BlockType::Interval, QStringLiteral("Sweet Spot"), 12, 0.90, -1, 90),
            block(BlockType::Recovery, QStringLiteral("Recovery"), 5, 0.55),
            block(BlockType::Interval, QStringLiteral("Sweet Spot"), 12, 0.90, -1, 90),
            block(BlockType::Recovery, QStringLiteral("Recovery"), 5, 0.55),
            block(BlockType::Interval, QStringLiteral("Sweet Spot"), 12, 0.90, -1, 90),
            block(BlockType::CoolDown, QStringLiteral("Cool Down"), 4, 0.55, 0.35),
        };
        lib.append(w);
    }
    {
        Workout w;
        w.name = QStringLiteral("Endurance Base");
        w.category = QStringLiteral("Endurance");
        w.focus = QStringLiteral("Build your base");
        w.description = QStringLiteral("Steady aerobic ride in zone 2. Builds fat metabolism, capillary density "
                                       "and the aerobic engine all other training relies on.");
        w.intervals = {
            block(BlockType::WarmUp, QStringLiteral("Warm Up"), 10, 0.40, 0.62),
            block(BlockType::Steady, QStringLiteral("Endurance"), 20, 0.68, -1, 88),
            block(BlockType::Steady, QStringLiteral("Endurance+"), 20, 0.72, -1, 90),
            block(BlockType::CoolDown, QStringLiteral("Cool Down"), 10, 0.60, 0.40),
        };
        lib.append(w);
    }
    {
        Workout w;
        w.name = QStringLiteral("VO₂ Max 5 × 3");
        w.category = QStringLiteral("Intervals");
        w.focus = QStringLiteral("VO₂ Max");
        w.description = QStringLiteral("Five hard 3-minute efforts at 120% FTP with equal recovery. "
                                       "Raises maximal oxygen uptake and your power ceiling.");
        w.intervals.append(block(BlockType::WarmUp, QStringLiteral("Warm Up"), 10, 0.40, 0.70));
        for (int r = 0; r < 5; ++r) {
            w.intervals.append(block(BlockType::Interval, QStringLiteral("VO₂ Max"), 3, 1.20, -1, 100));
            w.intervals.append(block(BlockType::Recovery, QStringLiteral("Recovery"), 3, 0.55));
        }
        w.intervals.append(block(BlockType::CoolDown, QStringLiteral("Cool Down"), 5, 0.55, 0.35));
        lib.append(w);
    }
    {
        Workout w;
        w.name = QStringLiteral("Recovery Spin");
        w.category = QStringLiteral("Recovery");
        w.focus = QStringLiteral("Active recovery");
        w.description = QStringLiteral("Easy spinning well below endurance pace to promote blood flow and recovery.");
        w.intervals = {
            block(BlockType::WarmUp, QStringLiteral("Easy Start"), 3, 0.40, 0.50),
            block(BlockType::Recovery, QStringLiteral("Easy Spin"), 24, 0.55, -1, 90),
            block(BlockType::CoolDown, QStringLiteral("Cool Down"), 3, 0.50, 0.40),
        };
        lib.append(w);
    }
    {
        Workout w;
        w.name = QStringLiteral("Threshold 2 × 20");
        w.category = QStringLiteral("Intervals");
        w.focus = QStringLiteral("Threshold");
        w.description = QStringLiteral("The classic: two 20-minute blocks at 95% FTP. Improves the power you can "
                                       "hold for an hour.");
        w.intervals = {
            block(BlockType::WarmUp, QStringLiteral("Warm Up"), 10, 0.40, 0.70),
            block(BlockType::Interval, QStringLiteral("Threshold"), 20, 0.95, -1, 90),
            block(BlockType::Recovery, QStringLiteral("Recovery"), 5, 0.55),
            block(BlockType::Interval, QStringLiteral("Threshold"), 20, 0.95, -1, 90),
            block(BlockType::CoolDown, QStringLiteral("Cool Down"), 10, 0.60, 0.40),
        };
        lib.append(w);
    }
    {
        Workout w;
        w.name = QStringLiteral("Ramp FTP Test");
        w.category = Workout::ftpTestCategory();
        w.focus = QStringLiteral("Progressive ramp to test your FTP");
        w.description = QStringLiteral("Power rises every minute until you can no longer hold the target. "
                                       "Your new FTP is roughly 75% of your best 1-minute power.");
        w.intervals = {
            block(BlockType::WarmUp, QStringLiteral("Warm Up"), 5, 0.40, 0.45),
            block(BlockType::Ramp, QStringLiteral("Ramp"), 20, 0.50, 1.50),
        };
        lib.append(w);
    }
    {
        Workout w;
        w.name = QStringLiteral("Tempo 3 × 15");
        w.category = QStringLiteral("Endurance");
        w.focus = QStringLiteral("Muscular endurance");
        w.description = QStringLiteral("Three 15-minute tempo blocks at 80% FTP. Steady, sustainable and very effective.");
        w.intervals = {
            block(BlockType::WarmUp, QStringLiteral("Warm Up"), 8, 0.40, 0.65),
            block(BlockType::Steady, QStringLiteral("Tempo"), 15, 0.80, -1, 90),
            block(BlockType::Recovery, QStringLiteral("Recovery"), 4, 0.55),
            block(BlockType::Steady, QStringLiteral("Tempo"), 15, 0.80, -1, 90),
            block(BlockType::Recovery, QStringLiteral("Recovery"), 4, 0.55),
            block(BlockType::Steady, QStringLiteral("Tempo"), 15, 0.80, -1, 90),
            block(BlockType::CoolDown, QStringLiteral("Cool Down"), 6, 0.55, 0.35),
        };
        lib.append(w);
    }
    {
        Workout w;
        w.name = QStringLiteral("Over-Unders 4 × 8");
        w.category = QStringLiteral("Intervals");
        w.focus = QStringLiteral("Lactate clearance");
        w.description = QStringLiteral("Alternate 2 minutes just under and 1 minute just over threshold. "
                                       "Teaches the body to clear lactate while working hard.");
        w.intervals.append(block(BlockType::WarmUp, QStringLiteral("Warm Up"), 10, 0.40, 0.70));
        for (int r = 0; r < 4; ++r) {
            w.intervals.append(block(BlockType::Interval, QStringLiteral("Under"), 2, 0.92, -1, 90));
            w.intervals.append(block(BlockType::Interval, QStringLiteral("Over"), 1, 1.08, -1, 95));
            w.intervals.append(block(BlockType::Interval, QStringLiteral("Under"), 2, 0.92, -1, 90));
            w.intervals.append(block(BlockType::Interval, QStringLiteral("Over"), 1, 1.08, -1, 95));
            w.intervals.append(block(BlockType::Interval, QStringLiteral("Under"), 2, 0.92, -1, 90));
            if (r < 3)
                w.intervals.append(block(BlockType::Recovery, QStringLiteral("Recovery"), 4, 0.55));
        }
        w.intervals.append(block(BlockType::CoolDown, QStringLiteral("Cool Down"), 8, 0.55, 0.35));
        lib.append(w);
    }

    for (Workout &w : lib)
        w.builtIn = true;
    return lib;
}

} // namespace watt
