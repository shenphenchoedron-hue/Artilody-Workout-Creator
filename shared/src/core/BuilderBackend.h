#pragma once

#include "Workout.h"
#include "Zones.h"

#include <QObject>
#include <QString>
#include <optional>

namespace watt {

// What the WorkoutBuilder needs from the outside world: the rider's settings
// (FTP per sport, zones, cadence) and somewhere to load / store workouts.
//
// The Watts Trainer app uses AppBuilderBackend (SQLite database + profile);
// the standalone "Artilody Workout Creator" implements it on top of plain
// .awt files, so WorkoutBuilder.cpp itself has no database dependency.
class BuilderBackend : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~BuilderBackend() override = default;

    virtual int ftpFor(const QString &sport) const = 0;
    virtual ZoneSet zones() const = 0;
    virtual int preferredCadence() const = 0;

    virtual std::optional<Workout> workout(qint64 id) const = 0;
    // Inserts (id < 0) or updates the workout. Returns its id or -1 (see lastError()).
    virtual qint64 saveWorkout(const Workout &workout) = 0;
    virtual QString lastError() const = 0;

signals:
    // FTP, zones or cadence changed: watts / zones shown by the Builder are refreshed.
    void profileChanged();
};

} // namespace watt
