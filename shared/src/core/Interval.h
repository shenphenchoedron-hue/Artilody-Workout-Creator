#pragma once

#include <QJsonObject>
#include <QString>

namespace watt {

enum class BlockType { WarmUp, Steady, Interval, Recovery, CoolDown, Ramp };

QString blockTypeToString(BlockType type);
BlockType blockTypeFromString(const QString &text);
QString blockTypeDisplayName(BlockType type);

// One block of a structured workout. Intensity is stored relative to FTP so a
// workout scales automatically with the rider's profile. Watts are derived.
struct Interval
{
    QString name;
    BlockType type = BlockType::Steady;
    int durationSec = 300;
    double startPercent = 0.60;   // fraction of FTP at the start of the block
    double endPercent = 0.60;     // fraction of FTP at the end (== start unless ramp)
    int cadence = 0;              // target cadence (rpm), 0 = none
    int rpeZone = -1;             // explicit RPE zone 0..5, -1 = derive from intensity

    bool isRamp() const;
    double percentAt(double secondsIntoInterval) const;
    int wattsAt(double secondsIntoInterval, int ftp) const;
    double averagePercent() const;

    QJsonObject toJson() const;
    static Interval fromJson(const QJsonObject &obj);
    static Interval defaultFor(BlockType type);

    bool operator==(const Interval &o) const;
    bool operator!=(const Interval &o) const { return !(*this == o); }
};

} // namespace watt
