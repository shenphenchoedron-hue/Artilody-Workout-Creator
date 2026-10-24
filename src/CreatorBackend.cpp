#include "CreatorBackend.h"

#include "CreatorSettings.h"
#include "WorkoutFileLibrary.h"

namespace awc {

CreatorBackend::CreatorBackend(CreatorSettings *settings, WorkoutFileLibrary *library, QObject *parent)
    : BuilderBackend(parent)
    , m_settings(settings)
    , m_library(library)
{
    connect(m_settings, &CreatorSettings::profileChanged, this, &BuilderBackend::profileChanged);
}

int CreatorBackend::ftpFor(const QString &sport) const
{
    return watt::ftpForSport(sport, m_settings->ftp(), m_settings->rowingFtp());
}

watt::ZoneSet CreatorBackend::zones() const
{
    return watt::ZoneSet();
}

int CreatorBackend::preferredCadence() const
{
    return m_settings->preferredCadence();
}

std::optional<watt::Workout> CreatorBackend::workout(qint64 id) const
{
    return m_library->workout(id);
}

qint64 CreatorBackend::saveWorkout(const watt::Workout &workout)
{
    return m_library->storeWorkout(workout);
}

QString CreatorBackend::lastError() const
{
    return m_library->lastError();
}

} // namespace awc
