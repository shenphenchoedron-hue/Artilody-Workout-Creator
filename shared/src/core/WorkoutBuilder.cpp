#include "WorkoutBuilder.h"

#include "TimeFormat.h"

#include <algorithm>
#include <cmath>

namespace watt {

// ---------------------------------------------------------------------------
// IntervalModel
// ---------------------------------------------------------------------------

IntervalModel::IntervalModel(BuilderBackend *backend, QObject *parent)
    : QAbstractListModel(parent)
    , m_backend(backend)
{
}

int IntervalModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant IntervalModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const Interval &i = m_items.at(index.row());
    const int ftp = this->ftp();
    switch (role) {
    case NameRole: return i.name;
    case TypeRole: return blockTypeToString(i.type);
    case TypeNameRole: return blockTypeDisplayName(i.type);
    case DurationRole: return i.durationSec;
    case DurationTextRole: return formatClock(i.durationSec);
    case WattsRole: return i.wattsAt(0, ftp);
    case EndWattsRole: return i.wattsAt(i.durationSec, ftp);
    case PercentRole: return int(std::lround(i.startPercent * 100));
    case EndPercentRole: return int(std::lround(i.endPercent * 100));
    case CadenceRole: return i.cadence;
    case RpeRole: return i.rpeZone;
    case ZoneRole: return zoneForInterval(i, i.averagePercent(), m_backend->zones());
    case RampRole: return i.isRamp();
    default: return {};
    }
}

QHash<int, QByteArray> IntervalModel::roleNames() const
{
    return {
        { NameRole, "name" }, { TypeRole, "type" }, { TypeNameRole, "typeName" }, { DurationRole, "duration" },
        { DurationTextRole, "durationText" }, { WattsRole, "watts" }, { EndWattsRole, "endWatts" },
        { PercentRole, "percent" }, { EndPercentRole, "endPercent" }, { CadenceRole, "cadence" },
        { RpeRole, "rpe" }, { ZoneRole, "zone" }, { RampRole, "isRamp" },
    };
}

void IntervalModel::setIntervals(const QVector<Interval> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

void IntervalModel::update(int row, const Interval &interval)
{
    if (row < 0 || row >= m_items.size())
        return;
    m_items[row] = interval;
    emit dataChanged(index(row), index(row));
}

void IntervalModel::insert(int row, const Interval &interval)
{
    row = std::clamp(row, 0, int(m_items.size()));
    beginInsertRows(QModelIndex(), row, row);
    m_items.insert(row, interval);
    endInsertRows();
}

void IntervalModel::remove(int row)
{
    if (row < 0 || row >= m_items.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_items.removeAt(row);
    endRemoveRows();
}

void IntervalModel::move(int from, int to)
{
    if (from < 0 || from >= m_items.size() || to < 0 || to >= m_items.size() || from == to)
        return;
    // QAbstractItemModel::beginMoveRows uses "insert before" semantics
    const int dest = to > from ? to + 1 : to;
    if (!beginMoveRows(QModelIndex(), from, from, QModelIndex(), dest))
        return;
    m_items.move(from, to);
    endMoveRows();
}

int IntervalModel::ftp() const
{
    return m_backend->ftpFor(m_sport);
}

void IntervalModel::setSport(const QString &sport)
{
    const QString s = sport::normalize(sport);
    if (s == m_sport)
        return;
    m_sport = s;
    refreshAll();
}

void IntervalModel::refreshAll()
{
    if (!m_items.isEmpty())
        emit dataChanged(index(0), index(m_items.size() - 1));
}

// ---------------------------------------------------------------------------
// WorkoutBuilder
// ---------------------------------------------------------------------------

WorkoutBuilder::WorkoutBuilder(BuilderBackend *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
    , m_model(backend)
{
    connect(m_backend, &BuilderBackend::profileChanged, this, [this] {
        m_model.refreshAll();
        emit ftpChanged();
        emit contentChanged();
        emit selectedChanged();
    });
    // the sport (and with it the FTP) changes with the workout
    connect(this, &WorkoutBuilder::workoutChanged, this, &WorkoutBuilder::ftpChanged);
    newWorkout();
}

void WorkoutBuilder::setError(const QString &e)
{
    if (m_lastError != e) {
        m_lastError = e;
        emit lastErrorChanged();
    }
}

void WorkoutBuilder::markDirty()
{
    if (!m_dirty) {
        m_dirty = true;
        emit dirtyChanged();
    }
}

void WorkoutBuilder::setName(const QString &name)
{
    if (m_workout.name == name)
        return;
    m_workout.name = name;
    markDirty();
    emit workoutChanged();
}

void WorkoutBuilder::setCategory(const QString &c)
{
    if (m_workout.category == c)
        return;
    m_workout.category = c;
    markDirty();
    emit workoutChanged();
}

void WorkoutBuilder::setFocus(const QString &f)
{
    if (m_workout.focus == f)
        return;
    m_workout.focus = f;
    markDirty();
    emit workoutChanged();
}

void WorkoutBuilder::setDescription(const QString &d)
{
    if (m_workout.description == d)
        return;
    m_workout.description = d;
    markDirty();
    emit workoutChanged();
}

void WorkoutBuilder::setSport(const QString &value)
{
    const QString s = sport::normalize(value);
    if (m_workout.sport == s)
        return;
    m_workout.sport = s;
    m_model.setSport(s);
    markDirty();
    emit workoutChanged();
    emit contentChanged();
    emit selectedChanged();
}

int WorkoutBuilder::ftp() const
{
    return m_backend->ftpFor(m_workout.sport);
}

void WorkoutBuilder::setSelectedIndex(int index)
{
    index = std::clamp(index, -1, m_model.rowCount() - 1);
    if (m_selected == index)
        return;
    m_selected = index;
    emit selectedChanged();
}

QVariantMap WorkoutBuilder::selected() const
{
    QVariantMap m;
    if (m_selected < 0 || m_selected >= m_model.rowCount())
        return m;
    const QModelIndex idx = m_model.index(m_selected);
    const auto roles = m_model.roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it)
        m.insert(QString::fromLatin1(it.value()), m_model.data(idx, it.key()));
    m.insert(QStringLiteral("index"), m_selected);
    return m;
}

Workout WorkoutBuilder::currentWorkout() const
{
    Workout w = m_workout;
    w.intervals = m_model.intervals();
    return w;
}

int WorkoutBuilder::totalDurationSec() const
{
    return currentWorkout().totalDurationSec();
}

QVariantList WorkoutBuilder::segments() const
{
    return Workout::segmentsToVariant(currentWorkout().segments(ftp(), m_backend->zones()));
}

int WorkoutBuilder::intensity() const
{
    return int(std::lround(currentWorkout().averagePercent() * 100));
}

int WorkoutBuilder::tss() const
{
    return int(std::lround(currentWorkout().estimatedTss()));
}

QStringList WorkoutBuilder::rpeOptions() const
{
    QStringList l { QStringLiteral("Auto (from power)") };
    l.append(rpeZoneDescriptions());
    return l;
}

QStringList WorkoutBuilder::categories() const
{
    return { QStringLiteral("Endurance"), QStringLiteral("Intervals"), QStringLiteral("Recovery"),
             QStringLiteral("FTP Tests") };
}

void WorkoutBuilder::newWorkout(const QString &sport)
{
    m_workout = Workout();
    m_workout.sport = sport::normalize(sport);
    m_workout.name = QStringLiteral("New Workout");
    m_workout.category = QStringLiteral("Intervals");
    m_model.setSport(m_workout.sport);
    QVector<Interval> blocks { Interval::defaultFor(BlockType::WarmUp), Interval::defaultFor(BlockType::Interval),
                               Interval::defaultFor(BlockType::CoolDown) };
    m_model.setIntervals(blocks);
    m_selected = 1;
    m_dirty = false;
    emit workoutChanged();
    emit contentChanged();
    emit selectedChanged();
    emit dirtyChanged();
}

bool WorkoutBuilder::loadWorkout(qint64 id)
{
    auto w = m_backend->workout(id);
    if (!w) {
        setError(QStringLiteral("Workout not found"));
        return false;
    }
    m_workout = *w;
    m_model.setSport(w->sport);
    m_model.setIntervals(w->intervals);
    m_selected = w->intervals.isEmpty() ? -1 : 0;
    m_dirty = false;
    emit workoutChanged();
    emit contentChanged();
    emit selectedChanged();
    emit dirtyChanged();
    return true;
}

qint64 WorkoutBuilder::save()
{
    Workout w = currentWorkout();
    if (w.name.trimmed().isEmpty()) {
        setError(QStringLiteral("Please give the workout a name"));
        return -1;
    }
    if (w.intervals.isEmpty()) {
        setError(QStringLiteral("Add at least one block"));
        return -1;
    }
    // Editing a built-in workout creates a personal copy, keeping the originals.
    if (w.builtIn) {
        w.id = -1;
        w.builtIn = false;
        w.timesUsed = 0;
        w.favorite = false;
    } else if (w.id >= 0) {
        // Favourite flag and usage count may have changed in the library since
        // the workout was opened here – keep the stored values.
        if (const auto stored = m_backend->workout(w.id)) {
            w.favorite = stored->favorite;
            w.timesUsed = stored->timesUsed;
        }
    }
    const qint64 id = m_backend->saveWorkout(w);
    if (id < 0) {
        setError(m_backend->lastError());
        return -1;
    }
    m_workout = w;
    m_workout.id = id;
    m_dirty = false;
    setError(QString());
    emit workoutChanged();
    emit dirtyChanged();
    emit saved(id);
    return id;
}

void WorkoutBuilder::addBlock(const QString &type)
{
    const BlockType t = blockTypeFromString(type);
    int at = m_selected >= 0 ? m_selected + 1 : m_model.rowCount();
    if (t == BlockType::WarmUp)
        at = 0;
    else if (t == BlockType::CoolDown)
        at = m_model.rowCount();

    if (t == BlockType::Interval) {
        // An interval set: 3 × (5 min @ 105% + 3 min recovery)
        for (int r = 0; r < 3; ++r) {
            Interval work = Interval::defaultFor(BlockType::Interval);
            work.name = QStringLiteral("Interval");
            work.durationSec = 300;
            work.startPercent = work.endPercent = 1.05;
            work.cadence = 95;
            m_model.insert(at++, work);
            Interval rec = Interval::defaultFor(BlockType::Recovery);
            rec.durationSec = 180;
            m_model.insert(at++, rec);
        }
        m_selected = at - 6;
    } else {
        Interval i = Interval::defaultFor(t);
        if (t == BlockType::Steady)
            i.cadence = m_backend->preferredCadence();
        m_model.insert(at, i);
        m_selected = at;
    }
    markDirty();
    emit contentChanged();
    emit selectedChanged();
}

void WorkoutBuilder::addInterval()
{
    addBlock(QStringLiteral("steady"));
}

void WorkoutBuilder::duplicateBlock(int index)
{
    if (index < 0 || index >= m_model.rowCount())
        return;
    m_model.insert(index + 1, m_model.intervals().at(index));
    m_selected = index + 1;
    markDirty();
    emit contentChanged();
    emit selectedChanged();
}

void WorkoutBuilder::removeBlock(int index)
{
    if (index < 0 || index >= m_model.rowCount())
        return;
    m_model.remove(index);
    if (index < m_selected)
        --m_selected;   // keep the same block selected
    if (m_selected >= m_model.rowCount())
        m_selected = m_model.rowCount() - 1;
    markDirty();
    emit contentChanged();
    emit selectedChanged();
}

void WorkoutBuilder::moveBlock(int from, int to)
{
    const int n = m_model.rowCount();
    if (from < 0 || from >= n || to < 0 || to >= n || from == to)
        return;
    m_model.move(from, to);
    if (m_selected == from)
        m_selected = to;
    else if (from < m_selected && m_selected <= to)
        --m_selected;
    else if (to <= m_selected && m_selected < from)
        ++m_selected;
    markDirty();
    emit contentChanged();
    emit selectedChanged();
}

void WorkoutBuilder::modify(int index, const std::function<void(Interval &)> &fn)
{
    if (index < 0 || index >= m_model.rowCount())
        return;
    Interval i = m_model.intervals().at(index);
    fn(i);
    if (i == m_model.intervals().at(index))
        return;
    m_model.update(index, i);
    markDirty();
    emit contentChanged();
    if (index == m_selected)
        emit selectedChanged();
}

void WorkoutBuilder::setBlockName(int index, const QString &name)
{
    modify(index, [&](Interval &i) { i.name = name; });
}

void WorkoutBuilder::setDuration(int index, int seconds)
{
    modify(index, [&](Interval &i) { i.durationSec = std::clamp(seconds, 5, 6 * 3600); });
}

bool WorkoutBuilder::setDurationText(int index, const QString &text)
{
    int sec = parseClock(text);
    if (sec <= 0)
        return false;
    if (!text.contains(QLatin1Char(':')))
        sec *= 60; // plain number => minutes
    setDuration(index, sec);
    return true;
}

void WorkoutBuilder::setWatts(int index, int watts)
{
    const int ftp = std::max(1, this->ftp());
    modify(index, [&](Interval &i) {
        const double p = std::clamp(watts, 0, 2000) / double(ftp);
        const bool ramp = i.isRamp();
        i.startPercent = p;
        if (!ramp)
            i.endPercent = p;
    });
}

void WorkoutBuilder::setEndWatts(int index, int watts)
{
    const int ftp = std::max(1, this->ftp());
    modify(index, [&](Interval &i) { i.endPercent = std::clamp(watts, 0, 2000) / double(ftp); });
}

void WorkoutBuilder::setPercent(int index, int percent)
{
    modify(index, [&](Interval &i) {
        const double p = std::clamp(percent, 0, 400) / 100.0;
        const bool ramp = i.isRamp();
        i.startPercent = p;
        if (!ramp)
            i.endPercent = p;
    });
}

void WorkoutBuilder::setCadence(int index, int rpm)
{
    modify(index, [&](Interval &i) { i.cadence = std::clamp(rpm, 0, 200); });
}

void WorkoutBuilder::setRpe(int index, int zone)
{
    modify(index, [&](Interval &i) { i.rpeZone = std::clamp(zone, -1, kZoneCount - 1); });
}

int WorkoutBuilder::blockAtSecond(int second) const
{
    return currentWorkout().intervalIndexAt(second);
}

int WorkoutBuilder::blockStartSec(int index, int) const
{
    return currentWorkout().intervalStartSec(index);
}

} // namespace watt
