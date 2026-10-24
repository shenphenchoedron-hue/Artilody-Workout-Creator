#include "Zones.h"

#include <QtMath>
#include <algorithm>

namespace watt {

int ZoneSet::zoneForFraction(double f) const
{
    for (int i = 0; i < kZoneCount - 1; ++i) {
        if (f < upper[size_t(i)])
            return i;
    }
    return kZoneCount - 1;
}

int ZoneSet::zoneForWatts(double watts, int ftp) const
{
    if (ftp <= 0)
        return 0;
    return zoneForFraction(watts / double(ftp));
}

double ZoneSet::lowerBound(int zone) const
{
    if (zone <= 0)
        return 0.0;
    zone = std::min(zone, kZoneCount - 1);
    return upper[size_t(zone - 1)];
}

double ZoneSet::upperBound(int zone) const
{
    if (zone < 0 || zone >= kZoneCount - 1)
        return 0.0;
    return upper[size_t(zone)];
}

bool ZoneSet::isValid() const
{
    double prev = 0.0;
    for (double u : upper) {
        if (u <= prev || u > 3.0)
            return false;
        prev = u;
    }
    return true;
}

QJsonArray ZoneSet::toJson() const
{
    QJsonArray a;
    for (double u : upper)
        a.append(u);
    return a;
}

ZoneSet ZoneSet::fromJson(const QJsonArray &array)
{
    ZoneSet z;
    if (array.size() != kZoneCount - 1)
        return z;
    ZoneSet candidate;
    for (int i = 0; i < kZoneCount - 1; ++i)
        candidate.upper[size_t(i)] = array.at(i).toDouble();
    return candidate.isValid() ? candidate : z;
}

QStringList powerZoneNames()
{
    return { QStringLiteral("Z1 Recovery"), QStringLiteral("Z2 Endurance"), QStringLiteral("Z3 Tempo"),
             QStringLiteral("Z4 Sweet Spot"), QStringLiteral("Z5 Threshold"), QStringLiteral("Z6 VO2 Max") };
}

QStringList rpeZoneLabels()
{
    return { QStringLiteral("1"), QStringLiteral("2–3"), QStringLiteral("4–6"),
             QStringLiteral("7–8"), QStringLiteral("9"), QStringLiteral("10") };
}

QStringList rpeZoneDescriptions()
{
    return { QStringLiteral("1  Very Easy"), QStringLiteral("2 – 3  Easy"), QStringLiteral("4 – 6  Moderate"),
             QStringLiteral("7 – 8  Vigorous"), QStringLiteral("9  Very Hard"), QStringLiteral("10  Max Effort") };
}

static constexpr double kHrZoneBounds[] = { 0.50, 0.60, 0.70, 0.80, 0.90, 1.00 };

int heartRateZone(int bpm, int maxHr, int restHr)
{
    if (bpm <= 0 || maxHr <= restHr)
        return 0;
    const double reserve = double(maxHr - restHr);
    const double f = (bpm - restHr) / reserve;
    if (f < kHrZoneBounds[0])
        return 0;
    for (int z = 1; z <= 5; ++z) {
        if (f < kHrZoneBounds[z])
            return z;
    }
    return 5;
}

int heartRateZoneLow(int zone, int maxHr, int restHr)
{
    zone = std::clamp(zone, 1, 5);
    return int(qRound(restHr + kHrZoneBounds[zone - 1] * (maxHr - restHr)));
}

int heartRateZoneHigh(int zone, int maxHr, int restHr)
{
    zone = std::clamp(zone, 1, 5);
    return int(qRound(restHr + kHrZoneBounds[zone] * (maxHr - restHr)));
}

QStringList heartRateZoneNames()
{
    return { QStringLiteral("Z1 Very Light"), QStringLiteral("Z2 Light"), QStringLiteral("Z3 Moderate"),
             QStringLiteral("Z4 Hard"), QStringLiteral("Z5 Maximum") };
}

} // namespace watt
