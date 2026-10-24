#pragma once

#include "BuilderBackend.h"

namespace awc {

class CreatorSettings;
class WorkoutFileLibrary;

// BuilderBackend of the Workout Creator: FTP / cadence from the creator's
// settings, workouts stored as files by WorkoutFileLibrary (no database).
class CreatorBackend : public watt::BuilderBackend
{
    Q_OBJECT
public:
    CreatorBackend(CreatorSettings *settings, WorkoutFileLibrary *library, QObject *parent = nullptr);

    int ftpFor(const QString &sport) const override;
    watt::ZoneSet zones() const override;
    int preferredCadence() const override;
    std::optional<watt::Workout> workout(qint64 id) const override;
    qint64 saveWorkout(const watt::Workout &workout) override;
    QString lastError() const override;

private:
    CreatorSettings *m_settings;
    WorkoutFileLibrary *m_library;
};

} // namespace awc
