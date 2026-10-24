#pragma once

#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <array>

namespace watt {

// Six power/RPE zones used everywhere in the app (charts, timeline colours,
// history analysis). The zone index is 0..5:
//   0  Z1 Recovery    RPE 1
//   1  Z2 Endurance   RPE 2-3
//   2  Z3 Tempo       RPE 4-6
//   3  Z4 Sweet Spot  RPE 7-8
//   4  Z5 Threshold   RPE 9
//   5  Z6 VO2 Max     RPE 10
constexpr int kZoneCount = 6;

struct ZoneSet
{
    // Upper bound (fraction of FTP) of zone 0..4. Zone 5 is open ended.
    std::array<double, kZoneCount - 1> upper { 0.55, 0.75, 0.87, 0.94, 1.05 };

    int zoneForFraction(double fractionOfFtp) const;
    int zoneForWatts(double watts, int ftp) const;
    double lowerBound(int zone) const;   // fraction of FTP
    double upperBound(int zone) const;   // fraction of FTP, 0 => open

    bool isValid() const;                // strictly increasing, positive
    QJsonArray toJson() const;
    static ZoneSet fromJson(const QJsonArray &array);

    bool operator==(const ZoneSet &o) const { return upper == o.upper; }
};

QStringList powerZoneNames();   // "Z1 Recovery", ...
QStringList rpeZoneLabels();    // "1", "2–3", "4–6", "7–8", "9", "10"
QStringList rpeZoneDescriptions(); // "1  Very Easy", ...

// Heart-rate zones (Karvonen / heart-rate reserve). Returns 1..5, or 0 when
// below zone 1 (<50% HRR) or invalid input.
int heartRateZone(int bpm, int maxHr, int restHr);
// Lower/upper bpm of heart-rate zone 1..5.
int heartRateZoneLow(int zone, int maxHr, int restHr);
int heartRateZoneHigh(int zone, int maxHr, int restHr);
QStringList heartRateZoneNames();

} // namespace watt
