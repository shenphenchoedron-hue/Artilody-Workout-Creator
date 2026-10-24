#include "Interval.h"

#include <QtMath>
#include <algorithm>
#include <cmath>

namespace watt {

QString blockTypeToString(BlockType type)
{
    switch (type) {
    case BlockType::WarmUp: return QStringLiteral("warmup");
    case BlockType::Steady: return QStringLiteral("steady");
    case BlockType::Interval: return QStringLiteral("interval");
    case BlockType::Recovery: return QStringLiteral("recovery");
    case BlockType::CoolDown: return QStringLiteral("cooldown");
    case BlockType::Ramp: return QStringLiteral("ramp");
    }
    return QStringLiteral("steady");
}

BlockType blockTypeFromString(const QString &text)
{
    const QString t = text.toLower().remove(QLatin1Char(' ')).remove(QLatin1Char('_'));
    if (t == QLatin1String("warmup")) return BlockType::WarmUp;
    if (t == QLatin1String("interval") || t == QLatin1String("intervals") || t == QLatin1String("work")) return BlockType::Interval;
    if (t == QLatin1String("recovery") || t == QLatin1String("rest")) return BlockType::Recovery;
    if (t == QLatin1String("cooldown")) return BlockType::CoolDown;
    if (t == QLatin1String("ramp")) return BlockType::Ramp;
    return BlockType::Steady;
}

QString blockTypeDisplayName(BlockType type)
{
    switch (type) {
    case BlockType::WarmUp: return QStringLiteral("Warm Up");
    case BlockType::Steady: return QStringLiteral("Steady");
    case BlockType::Interval: return QStringLiteral("Interval");
    case BlockType::Recovery: return QStringLiteral("Recovery");
    case BlockType::CoolDown: return QStringLiteral("Cool Down");
    case BlockType::Ramp: return QStringLiteral("Ramp");
    }
    return QString();
}

bool Interval::isRamp() const
{
    return std::abs(endPercent - startPercent) > 1e-6;
}

double Interval::percentAt(double t) const
{
    if (!isRamp() || durationSec <= 0)
        return startPercent;
    const double f = std::clamp(t / double(durationSec), 0.0, 1.0);
    return startPercent + (endPercent - startPercent) * f;
}

int Interval::wattsAt(double t, int ftp) const
{
    return int(std::lround(percentAt(t) * ftp));
}

double Interval::averagePercent() const
{
    return (startPercent + endPercent) / 2.0;
}

QJsonObject Interval::toJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("name"), name);
    o.insert(QStringLiteral("type"), blockTypeToString(type));
    o.insert(QStringLiteral("duration"), durationSec);
    o.insert(QStringLiteral("start"), startPercent);
    o.insert(QStringLiteral("end"), endPercent);
    if (cadence > 0)
        o.insert(QStringLiteral("cadence"), cadence);
    if (rpeZone >= 0)
        o.insert(QStringLiteral("rpe"), rpeZone);
    return o;
}

Interval Interval::fromJson(const QJsonObject &o)
{
    Interval i;
    i.type = blockTypeFromString(o.value(QStringLiteral("type")).toString());
    i.name = o.value(QStringLiteral("name")).toString(blockTypeDisplayName(i.type));
    i.durationSec = std::max(1, o.value(QStringLiteral("duration")).toInt(300));
    i.startPercent = std::max(0.0, o.value(QStringLiteral("start")).toDouble(0.6));
    i.endPercent = std::max(0.0, o.value(QStringLiteral("end")).toDouble(i.startPercent));
    i.cadence = std::max(0, o.value(QStringLiteral("cadence")).toInt(0));
    i.rpeZone = std::clamp(o.value(QStringLiteral("rpe")).toInt(-1), -1, 5);
    return i;
}

Interval Interval::defaultFor(BlockType type)
{
    Interval i;
    i.type = type;
    i.name = blockTypeDisplayName(type);
    switch (type) {
    case BlockType::WarmUp:
        i.durationSec = 600; i.startPercent = 0.40; i.endPercent = 0.60; break;
    case BlockType::Steady:
        i.durationSec = 600; i.startPercent = i.endPercent = 0.70; break;
    case BlockType::Interval:
        i.name = QStringLiteral("Sweet Spot");
        i.durationSec = 720; i.startPercent = i.endPercent = 0.90; i.cadence = 85; break;
    case BlockType::Recovery:
        i.durationSec = 300; i.startPercent = i.endPercent = 0.55; break;
    case BlockType::CoolDown:
        i.durationSec = 300; i.startPercent = 0.55; i.endPercent = 0.35; break;
    case BlockType::Ramp:
        i.durationSec = 600; i.startPercent = 0.50; i.endPercent = 1.00; break;
    }
    return i;
}

bool Interval::operator==(const Interval &o) const
{
    return name == o.name && type == o.type && durationSec == o.durationSec
        && qFuzzyCompare(1.0 + startPercent, 1.0 + o.startPercent)
        && qFuzzyCompare(1.0 + endPercent, 1.0 + o.endPercent)
        && cadence == o.cadence && rpeZone == o.rpeZone;
}

} // namespace watt
