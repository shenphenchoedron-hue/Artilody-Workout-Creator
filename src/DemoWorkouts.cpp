#include "DemoWorkouts.h"

#include "WorkoutFileLibrary.h"
#include "WorkoutIO.h"

#include <QCoreApplication>
#include <QDir>

using watt::BlockType;
using watt::Interval;
using watt::Workout;

namespace awc::demo {

namespace {
Interval block(BlockType type, const QString &name, int sec, double from, double to = -1, int cadence = 0)
{
    Interval i;
    i.type = type;
    i.name = name;
    i.durationSec = sec;
    i.startPercent = from;
    i.endPercent = to < 0 ? from : to;
    i.cadence = cadence;
    return i;
}
} // namespace

QVector<Workout> workouts()
{
    QVector<Workout> list;

    Workout ss;
    ss.name = QStringLiteral("Sweet Spot 3 × 12");
    ss.category = QStringLiteral("Intervals");
    ss.focus = QStringLiteral("Sweet Spot");
    ss.description = QStringLiteral("Three long sweet spot efforts with short recoveries.");
    ss.intervals << block(BlockType::WarmUp, QStringLiteral("Warm Up"), 600, 0.45, 0.75, 90);
    for (int r = 0; r < 3; ++r) {
        ss.intervals << block(BlockType::Interval, QStringLiteral("Sweet Spot"), 720, 0.90, -1, 92);
        if (r < 2)
            ss.intervals << block(BlockType::Recovery, QStringLiteral("Recovery"), 240, 0.55, -1, 85);
    }
    ss.intervals << block(BlockType::CoolDown, QStringLiteral("Cool Down"), 480, 0.70, 0.45);
    list << ss;

    Workout vo2;
    vo2.name = QStringLiteral("VO2 Max 5 × 3");
    vo2.category = QStringLiteral("Intervals");
    vo2.focus = QStringLiteral("VO2 Max");
    vo2.intervals << block(BlockType::WarmUp, QStringLiteral("Warm Up"), 600, 0.45, 0.75);
    for (int r = 0; r < 5; ++r)
        vo2.intervals << block(BlockType::Interval, QStringLiteral("VO2 Max"), 180, 1.18, -1, 100)
                      << block(BlockType::Recovery, QStringLiteral("Easy"), 180, 0.50, -1, 85);
    vo2.intervals << block(BlockType::CoolDown, QStringLiteral("Cool Down"), 420, 0.65, 0.40);
    list << vo2;

    Workout endu;
    endu.name = QStringLiteral("Endurance Hour");
    endu.category = QStringLiteral("Endurance");
    endu.focus = QStringLiteral("Aerobic base");
    endu.intervals << block(BlockType::WarmUp, QStringLiteral("Warm Up"), 600, 0.45, 0.65)
                   << block(BlockType::Steady, QStringLiteral("Endurance"), 2400, 0.68, -1, 90)
                   << block(BlockType::CoolDown, QStringLiteral("Cool Down"), 600, 0.60, 0.40);
    list << endu;

    Workout rec;
    rec.name = QStringLiteral("Recovery Spin");
    rec.category = QStringLiteral("Recovery");
    rec.intervals << block(BlockType::Steady, QStringLiteral("Easy Spin"), 1500, 0.48, -1, 90)
                  << block(BlockType::CoolDown, QStringLiteral("Cool Down"), 300, 0.45, 0.35);
    list << rec;

    Workout rowSs;
    rowSs.name = QStringLiteral("Rowing Sweet Spot 3 × 8");
    rowSs.sport = watt::sport::rowing();
    rowSs.category = QStringLiteral("Intervals");
    rowSs.focus = QStringLiteral("Sweet Spot");
    rowSs.intervals << block(BlockType::WarmUp, QStringLiteral("Warm Up"), 480, 0.50, 0.70, 20);
    for (int r = 0; r < 3; ++r) {
        rowSs.intervals << block(BlockType::Interval, QStringLiteral("Sweet Spot"), 480, 0.90, -1, 24);
        if (r < 2)
            rowSs.intervals << block(BlockType::Recovery, QStringLiteral("Easy Paddle"), 180, 0.55, -1, 18);
    }
    rowSs.intervals << block(BlockType::CoolDown, QStringLiteral("Cool Down"), 300, 0.60, 0.40, 18);
    list << rowSs;

    Workout pyr;
    pyr.name = QStringLiteral("Rowing Pyramid 1-2-3-2-1");
    pyr.sport = watt::sport::rowing();
    pyr.category = QStringLiteral("Intervals");
    pyr.focus = QStringLiteral("Threshold");
    pyr.intervals << block(BlockType::WarmUp, QStringLiteral("Warm Up"), 420, 0.50, 0.72, 20);
    const int mins[] = { 1, 2, 3, 2, 1 };
    for (int m : mins)
        pyr.intervals << block(BlockType::Interval, QStringLiteral("%1 min hard").arg(m), m * 60, 1.05, -1, 28)
                      << block(BlockType::Recovery, QStringLiteral("Easy"), 90, 0.55, -1, 18);
    pyr.intervals << block(BlockType::CoolDown, QStringLiteral("Cool Down"), 300, 0.60, 0.40, 18);
    list << pyr;

    Workout ramp;
    ramp.name = QStringLiteral("Ramp FTP Test");
    ramp.category = QStringLiteral("FTP Tests");
    ramp.focus = QStringLiteral("FTP estimate");
    ramp.intervals << block(BlockType::WarmUp, QStringLiteral("Warm Up"), 300, 0.40, 0.55)
                   << block(BlockType::Ramp, QStringLiteral("Ramp"), 1200, 0.55, 1.45)
                   << block(BlockType::CoolDown, QStringLiteral("Cool Down"), 300, 0.50, 0.35);
    list << ramp;

    return list;
}

int seedFolder(const QString &folder)
{
    QDir().mkpath(folder);
    int n = 0;
    for (const Workout &w : workouts()) {
        const QString path = QDir(folder).filePath(watt::workoutio::safeFileName(w.name) + QStringLiteral(".awt"));
        if (watt::workoutio::writeFile(path, watt::workoutio::toAwt({ w }, QCoreApplication::applicationVersion(),
                                                                  WorkoutFileLibrary::appName())))
            ++n;
    }
    // a Zwift file next to them
    Workout ou;
    ou.name = QStringLiteral("Zwift Over-Unders");
    ou.category = QStringLiteral("Intervals");
    ou.intervals << block(BlockType::WarmUp, QStringLiteral("Warm Up"), 600, 0.45, 0.75);
    for (int r = 0; r < 4; ++r)
        ou.intervals << block(BlockType::Interval, QStringLiteral("Over"), 120, 1.05)
                     << block(BlockType::Steady, QStringLiteral("Under"), 120, 0.90)
                     << block(BlockType::Recovery, QStringLiteral("Recovery"), 180, 0.50);
    ou.intervals << block(BlockType::CoolDown, QStringLiteral("Cool Down"), 300, 0.65, 0.40);
    if (watt::workoutio::writeFile(QDir(folder).filePath(QStringLiteral("Zwift Over-Unders.zwo")),
                                   watt::workoutio::toZwo(ou, WorkoutFileLibrary::appName())))
        ++n;
    return n;
}

} // namespace awc::demo
