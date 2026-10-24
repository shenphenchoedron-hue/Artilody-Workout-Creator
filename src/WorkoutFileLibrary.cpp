#include "WorkoutFileLibrary.h"

#include "TimeFormat.h"
#include "WorkoutBuilder.h"
#include "WorkoutIO.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantMap>
#include <algorithm>

namespace io = watt::workoutio;
using watt::Workout;

namespace awc {

namespace {

QString formatOf(const QString &path)
{
    return QFileInfo(path).suffix().compare(QLatin1String("zwo"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("zwo") : QStringLiteral("awt");
}

QString withSuffix(QString path, const QString &ext)
{
    if (QFileInfo(path).suffix().compare(ext, Qt::CaseInsensitive) != 0)
        path += QLatin1Char('.') + ext;
    return path;
}

QString cleanAbs(const QString &path)
{
    return path.isEmpty() ? QString() : QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

// Mini profile for the library list: [{s, w, h, z}] as fractions of the
// duration / of 150 % FTP.
QVariantList miniBars(const Workout &w)
{
    QVariantList list;
    const int total = w.totalDurationSec();
    if (total <= 0)
        return list;
    const int step = std::max(60, total / 40);
    const auto segs = w.segments(100, watt::ZoneSet(), step);   // ftp 100 -> watts == percent
    for (const auto &s : segs) {
        QVariantMap m;
        m.insert(QStringLiteral("s"), s.startSec / double(total));
        m.insert(QStringLiteral("w"), s.durationSec / double(total));
        m.insert(QStringLiteral("h"), std::clamp(s.watts / 150.0, 0.06, 1.0));
        m.insert(QStringLiteral("z"), s.zone);
        list.append(m);
    }
    return list;
}

} // namespace

WorkoutFileLibrary::WorkoutFileLibrary(QObject *parent)
    : QAbstractListModel(parent)
{
    m_rescan.setSingleShot(true);
    m_rescan.setInterval(400);
    connect(&m_rescan, &QTimer::timeout, this, &WorkoutFileLibrary::refresh);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_rescan.start(); });
    m_lastExportDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

// --- model ------------------------------------------------------------------

int WorkoutFileLibrary::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_view.size());
}

QVariant WorkoutFileLibrary::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_view.size())
        return {};
    const Entry &e = m_entries.at(m_view.at(index.row()));
    const Workout &w = e.workout;
    switch (role) {
    case PathRole: return e.path;
    case FileNameRole: return e.fileName;
    case FormatRole: return e.format;
    case NameRole: return e.error.isEmpty() ? w.name : QFileInfo(e.path).completeBaseName();
    case SportRole: return w.sport;
    case CategoryRole: return e.error.isEmpty() ? w.category : QString();
    case FocusRole: return w.focus;
    case DurationRole: return w.totalDurationSec();
    case DurationTextRole: return watt::formatHoursMinutes(w.totalDurationSec());
    case BlockCountRole: return int(w.intervals.size());
    case WorkoutCountRole: return e.workoutCount;
    case ModifiedRole: return e.modified;
    case ValidRole: return e.error.isEmpty();
    case ErrorRole: return e.error;
    case BarsRole: return miniBars(w);
    default: return {};
    }
}

QHash<int, QByteArray> WorkoutFileLibrary::roleNames() const
{
    return {
        { PathRole, "path" }, { FileNameRole, "fileName" }, { FormatRole, "format" }, { NameRole, "name" },
        { SportRole, "sport" }, { CategoryRole, "category" }, { FocusRole, "focus" },
        { DurationRole, "duration" }, { DurationTextRole, "durationText" }, { BlockCountRole, "blockCount" },
        { WorkoutCountRole, "workoutCount" }, { ModifiedRole, "modified" }, { ValidRole, "valid" },
        { ErrorRole, "error" }, { BarsRole, "bars" },
    };
}

// --- properties ---------------------------------------------------------------

void WorkoutFileLibrary::setFolder(const QString &folder)
{
    const QString f = cleanAbs(toLocalPath(folder));
    if (f == m_folder)
        return;
    m_folder = f;
    emit folderChanged();
    emit currentPathChanged();   // currentInFolder
    refresh();
}

void WorkoutFileLibrary::setIncludeZwo(bool include)
{
    if (include == m_includeZwo)
        return;
    m_includeZwo = include;
    emit includeZwoChanged();
    refresh();
}

void WorkoutFileLibrary::setFilterText(const QString &text)
{
    if (text == m_filterText)
        return;
    m_filterText = text;
    emit filterChanged();
    applyFilter();
}

void WorkoutFileLibrary::setSportFilter(const QString &sport)
{
    const QString s = sport == QLatin1String("all") || sport.isEmpty() ? QStringLiteral("all") : watt::sport::normalize(sport);
    if (s == m_sportFilter)
        return;
    m_sportFilter = s;
    emit filterChanged();
    applyFilter();
}

QString WorkoutFileLibrary::currentFileName() const
{
    return m_currentPath.isEmpty() ? QString() : QFileInfo(m_currentPath).fileName();
}

bool WorkoutFileLibrary::currentInFolder() const
{
    return !m_currentPath.isEmpty() && QFileInfo(m_currentPath).absolutePath() == m_folder;
}

QStringList WorkoutFileLibrary::zwiftFolders() const
{
    // Zwift keeps custom workouts in Documents/Zwift/Workouts/<ZwiftID>/
    QStringList out;
    const QDir root(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                        .filePath(QStringLiteral("Zwift/Workouts")));
    if (!root.exists())
        return out;
    for (const QFileInfo &fi : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
        out << fi.absoluteFilePath();
    return out;
}

void WorkoutFileLibrary::setCurrentPath(const QString &path)
{
    const QString p = cleanAbs(path);
    if (p == m_currentPath)
        return;
    m_currentPath = p;
    emit currentPathChanged();
}

void WorkoutFileLibrary::setError(const QString &e)
{
    if (e == m_lastError)
        return;
    m_lastError = e;
    emit lastErrorChanged();
}

// --- scanning -----------------------------------------------------------------

QVector<Workout> WorkoutFileLibrary::readFile(const QString &path, QString *error) const
{
    const io::ImportResult r = io::parseFile(path);
    if (!r.ok()) {
        if (error)
            *error = r.error.isEmpty() ? QStringLiteral("No workouts in file") : r.error;
        return {};
    }
    return r.workouts;
}

void WorkoutFileLibrary::refresh()
{
    QVector<Entry> entries;
    if (!m_folder.isEmpty()) {
        QStringList filters { QStringLiteral("*.awt") };
        if (m_includeZwo)
            filters << QStringLiteral("*.zwo");
        const QFileInfoList files = QDir(m_folder).entryInfoList(filters, QDir::Files | QDir::Readable);
        for (const QFileInfo &fi : files) {
            Entry e;
            e.path = cleanAbs(fi.absoluteFilePath());
            e.fileName = fi.fileName();
            e.format = formatOf(e.path);
            e.modified = fi.lastModified();
            const QVector<Workout> ws = readFile(e.path, &e.error);
            e.workoutCount = int(ws.size());
            if (!ws.isEmpty())
                e.workout = ws.constFirst();
            entries.append(e);
        }
    }
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
        const QString na = a.error.isEmpty() ? a.workout.name : a.fileName;
        const QString nb = b.error.isEmpty() ? b.workout.name : b.fileName;
        const int c = QString::compare(na, nb, Qt::CaseInsensitive);
        return c != 0 ? c < 0 : a.fileName < b.fileName;
    });
    m_entries = entries;
    applyFilter();
    watchFolder();
    emit zwiftFoldersChanged();
}

void WorkoutFileLibrary::applyFilter()
{
    beginResetModel();
    m_view.clear();
    const QString needle = m_filterText.trimmed();
    for (int i = 0; i < m_entries.size(); ++i) {
        const Entry &e = m_entries.at(i);
        if (m_sportFilter != QLatin1String("all") && (!e.error.isEmpty() || e.workout.sport != m_sportFilter))
            continue;
        if (!needle.isEmpty() && !e.workout.name.contains(needle, Qt::CaseInsensitive)
            && !e.fileName.contains(needle, Qt::CaseInsensitive)
            && !e.workout.category.contains(needle, Qt::CaseInsensitive)
            && !e.workout.focus.contains(needle, Qt::CaseInsensitive))
            continue;
        m_view.append(i);
    }
    endResetModel();
    emit countChanged();
}

void WorkoutFileLibrary::watchFolder()
{
    const QStringList dirs = m_watcher.directories();
    if (dirs.size() == 1 && dirs.constFirst() == m_folder)
        return;
    if (!dirs.isEmpty())
        m_watcher.removePaths(dirs);
    if (!m_folder.isEmpty() && QDir(m_folder).exists())
        m_watcher.addPath(m_folder);
}

bool WorkoutFileLibrary::ensureFolder()
{
    if (m_folder.isEmpty() || !QDir().mkpath(m_folder)) {
        setError(QStringLiteral("Could not create the workouts folder %1").arg(nativePath(m_folder)));
        return false;
    }
    watchFolder();
    return true;
}

const WorkoutFileLibrary::Entry *WorkoutFileLibrary::entryFor(const QString &path) const
{
    const QString p = cleanAbs(path);
    for (const Entry &e : m_entries)
        if (e.path == p)
            return &e;
    return nullptr;
}

// --- ids ----------------------------------------------------------------------

qint64 WorkoutFileLibrary::idForPath(const QString &path)
{
    const QString p = cleanAbs(path);
    auto it = m_pathIds.constFind(p);
    if (it != m_pathIds.constEnd())
        return it.value();
    const qint64 id = m_nextId++;
    m_pathIds.insert(p, id);
    m_idPaths.insert(id, p);
    return id;
}

void WorkoutFileLibrary::movePathId(const QString &from, const QString &to)
{
    const QString f = cleanAbs(from);
    const QString t = cleanAbs(to);
    if (f == t || !m_pathIds.contains(f))
        return;
    const qint64 id = m_pathIds.take(f);
    if (m_pathIds.contains(t))
        m_idPaths.remove(m_pathIds.value(t));
    m_pathIds.insert(t, id);
    m_idPaths.insert(id, t);
}

std::optional<Workout> WorkoutFileLibrary::workout(qint64 id) const
{
    const QString path = m_idPaths.value(id);
    if (path.isEmpty())
        return std::nullopt;
    const QVector<Workout> ws = readFile(path, nullptr);
    if (ws.isEmpty())
        return std::nullopt;
    Workout w = ws.constFirst();
    w.id = id;
    return w;
}

// --- writing ------------------------------------------------------------------

QByteArray WorkoutFileLibrary::awtBytes(const Workout &w) const
{
    Workout copy = w;
    copy.id = -1;
    return io::toAwt({ copy }, QCoreApplication::applicationVersion(), appName());
}

bool WorkoutFileLibrary::writeWorkoutFile(const QString &path, const Workout &w)
{
    QString err;
    const QByteArray data = formatOf(path) == QLatin1String("zwo") ? io::toZwo(w, appName()) : awtBytes(w);
    if (!io::writeFile(path, data, &err)) {
        setError(QStringLiteral("Could not write %1: %2").arg(nativePath(path), err));
        return false;
    }
    return true;
}

qint64 WorkoutFileLibrary::storeWorkout(const Workout &w)
{
    QString target;
    QString renameFrom;
    if (!m_saveAsPath.isEmpty()) {
        target = withSuffix(m_saveAsPath, QStringLiteral("awt"));
    } else if (w.id >= 0 && m_idPaths.contains(w.id)) {
        const QString existing = m_idPaths.value(w.id);
        QString err;
        const QVector<Workout> stored = QFileInfo::exists(existing) ? readFile(existing, &err) : QVector<Workout>();
        // Only single-workout .awt files are written in place; .zwo files keep
        // their (lossy) format and get an .awt next to them in the folder.
        if (formatOf(existing) == QLatin1String("awt") && stored.size() <= 1) {
            target = existing;
            // The file was named after the workout: follow a rename of the workout.
            const QFileInfo fi(existing);
            const QString newBase = io::safeFileName(w.name);
            if (stored.size() == 1 && fi.completeBaseName() == io::safeFileName(stored.constFirst().name)
                && newBase != fi.completeBaseName()) {
                const QString renamed = uniquePath(fi.absolutePath(), newBase, QStringLiteral("awt"), existing);
                if (!QFileInfo::exists(renamed)) {
                    target = renamed;
                    renameFrom = existing;
                }
            }
        }
    }
    if (target.isEmpty()) {
        if (!ensureFolder())
            return -1;
        target = uniquePath(m_folder, io::safeFileName(w.name), QStringLiteral("awt"));
    }
    target = cleanAbs(target);

    if (!writeWorkoutFile(target, w))
        return -1;
    if (!renameFrom.isEmpty()) {
        QFile::remove(renameFrom);
        movePathId(renameFrom, target);
    }
    setError(QString());
    const qint64 id = (w.id >= 0 && m_saveAsPath.isEmpty() && m_idPaths.value(w.id) == target) ? w.id : idForPath(target);
    m_saveAsPath.clear();
    setCurrentPath(target);
    refresh();
    return id;
}

// --- QML API --------------------------------------------------------------------

void WorkoutFileLibrary::newWorkout(const QString &sport)
{
    if (m_builder)
        m_builder->newWorkout(sport);
    setCurrentPath(QString());
}

bool WorkoutFileLibrary::openFile(const QString &pathOrUrl)
{
    const QString path = cleanAbs(toLocalPath(pathOrUrl));
    QString err;
    const QVector<Workout> ws = readFile(path, &err);
    if (ws.isEmpty()) {
        setError(QStringLiteral("Could not open %1: %2").arg(QFileInfo(path).fileName(), err));
        emit message(m_lastError);
        return false;
    }
    if (ws.size() > 1) {
        // A collection (e.g. an export of several workouts from Watts Trainer):
        // split it into one file per workout and open the first one.
        const int before = totalCount();
        QString first;
        if (!ensureFolder())
            return false;
        for (const Workout &w : ws) {
            const QString target = uniquePath(m_folder, io::safeFileName(w.name), QStringLiteral("awt"));
            if (writeWorkoutFile(target, w) && first.isEmpty())
                first = target;
        }
        refresh();
        if (first.isEmpty())
            return false;
        emit message(QStringLiteral("%1 contains %2 workouts – added them to your Workouts folder")
                         .arg(QFileInfo(path).fileName()).arg(totalCount() - before));
        return openFile(first);
    }
    const qint64 id = idForPath(path);
    if (!m_builder || !m_builder->loadWorkout(id)) {
        setError(QStringLiteral("Could not open %1").arg(QFileInfo(path).fileName()));
        emit message(m_lastError);
        return false;
    }
    setCurrentPath(path);
    setError(QString());
    return true;
}

bool WorkoutFileLibrary::saveAs(const QString &pathOrUrl)
{
    if (!m_builder)
        return false;
    m_saveAsPath = withSuffix(toLocalPath(pathOrUrl), QStringLiteral("awt"));
    const bool ok = m_builder->save() >= 0;
    m_saveAsPath.clear();
    if (ok)
        rememberExportDir(m_currentPath);
    return ok;
}

QString WorkoutFileLibrary::duplicateFile(const QString &pathOrUrl)
{
    const QString path = cleanAbs(toLocalPath(pathOrUrl));
    QString err;
    QVector<Workout> ws = readFile(path, &err);
    if (ws.isEmpty()) {
        setError(err);
        emit message(QStringLiteral("Could not duplicate: %1").arg(err));
        return {};
    }
    if (!ensureFolder())
        return {};
    Workout w = ws.constFirst();
    w.name = w.name + QStringLiteral(" (copy)");
    const QString target = uniquePath(m_folder, io::safeFileName(w.name), QStringLiteral("awt"));
    if (!writeWorkoutFile(target, w)) {
        emit message(m_lastError);
        return {};
    }
    refresh();
    emit message(QStringLiteral("Duplicated as \"%1\"").arg(w.name));
    return cleanAbs(target);
}

bool WorkoutFileLibrary::renameFile(const QString &pathOrUrl, const QString &newName)
{
    const QString path = cleanAbs(toLocalPath(pathOrUrl));
    const QString name = newName.trimmed();
    if (name.isEmpty()) {
        emit message(QStringLiteral("Please enter a name"));
        return false;
    }
    QString err;
    QVector<Workout> ws = readFile(path, &err);
    if (ws.size() != 1) {
        emit message(ws.isEmpty() ? QStringLiteral("Could not rename: %1").arg(err)
                                  : QStringLiteral("Files with several workouts cannot be renamed"));
        return false;
    }
    Workout w = ws.constFirst();
    w.name = name;
    const QFileInfo fi(path);
    const QString target = cleanAbs(uniquePath(fi.absolutePath(), io::safeFileName(name), fi.suffix().toLower(), path));
    if (!writeWorkoutFile(target, w)) {
        emit message(m_lastError);
        return false;
    }
    if (target != path) {
        QFile::remove(path);
        movePathId(path, target);
    }
    if (path == m_currentPath && m_builder) {
        setCurrentPath(target);
        if (m_builder->dirty())
            m_builder->setName(name);   // keep the unsaved edits
        else
            m_builder->loadWorkout(idForPath(target));
    }
    refresh();
    emit message(QStringLiteral("Renamed to \"%1\"").arg(name));
    return true;
}

bool WorkoutFileLibrary::removeFile(const QString &pathOrUrl)
{
    const QString path = cleanAbs(toLocalPath(pathOrUrl));
    const QString name = nameOf(path);
    // Prefer the trash so a mistake can be undone.
    bool ok = QFile::moveToTrash(path);
    if (!ok)
        ok = QFile::remove(path);
    if (!ok) {
        setError(QStringLiteral("Could not delete %1").arg(nativePath(path)));
        emit message(m_lastError);
        return false;
    }
    if (m_pathIds.contains(path))
        m_idPaths.remove(m_pathIds.take(path));
    if (path == m_currentPath)
        newWorkout(m_builder ? m_builder->sport() : QString());
    refresh();
    emit message(QStringLiteral("\"%1\" moved to the trash").arg(name));
    return true;
}

int WorkoutFileLibrary::importFiles(const QVariantList &paths)
{
    if (!ensureFolder()) {
        emit message(m_lastError);
        return 0;
    }
    int imported = 0;
    QStringList problems;
    QString lastTarget;
    for (const QVariant &v : paths) {
        const QString path = cleanAbs(toLocalPath(v.toUrl().isLocalFile() ? v.toUrl().toLocalFile() : v.toString()));
        const io::ImportResult r = io::parseFile(path);
        if (!r.ok()) {
            problems << QStringLiteral("%1: %2").arg(QFileInfo(path).fileName(), r.error);
            continue;
        }
        for (const Workout &w : r.workouts) {
            const QString target = uniquePath(m_folder, io::safeFileName(w.name), QStringLiteral("awt"));
            if (writeWorkoutFile(target, w)) {
                ++imported;
                lastTarget = target;
            } else {
                problems << m_lastError;
            }
        }
        for (const QString &warn : r.warnings)
            problems << QStringLiteral("%1: %2").arg(QFileInfo(path).fileName(), warn);
    }
    refresh();
    QString msg = imported == 1 ? QStringLiteral("Imported 1 workout") : QStringLiteral("Imported %1 workouts").arg(imported);
    if (!problems.isEmpty())
        msg += QStringLiteral(" – ") + problems.mid(0, 2).join(QStringLiteral("; "))
               + (problems.size() > 2 ? QStringLiteral(" …") : QString());
    emit message(msg);
    return imported;
}

bool WorkoutFileLibrary::exportZwo(const QString &pathOrUrl)
{
    if (!m_builder)
        return false;
    const QString path = withSuffix(toLocalPath(pathOrUrl), QStringLiteral("zwo"));
    QString err;
    const Workout w = m_builder->currentWorkout();
    if (!io::writeFile(path, io::toZwo(w, appName()), &err)) {
        setError(err);
        emit message(QStringLiteral("Export failed: %1").arg(err));
        return false;
    }
    rememberExportDir(path);
    emit message(w.isRowing() ? QStringLiteral("Exported %1 (as a bike workout – Zwift has no rowing)").arg(QFileInfo(path).fileName())
                              : QStringLiteral("Exported %1").arg(QFileInfo(path).fileName()));
    return true;
}

bool WorkoutFileLibrary::exportAwt(const QString &pathOrUrl)
{
    if (!m_builder)
        return false;
    const QString path = withSuffix(toLocalPath(pathOrUrl), QStringLiteral("awt"));
    QString err;
    if (!io::writeFile(path, awtBytes(m_builder->currentWorkout()), &err)) {
        setError(err);
        emit message(QStringLiteral("Export failed: %1").arg(err));
        return false;
    }
    rememberExportDir(path);
    emit message(QStringLiteral("Exported %1").arg(QFileInfo(path).fileName()));
    return true;
}

bool WorkoutFileLibrary::exportToZwift(const QString &zwiftFolder)
{
    const QString dir = toLocalPath(zwiftFolder);
    if (dir.isEmpty() || !QDir(dir).exists()) {
        emit message(QStringLiteral("Zwift workouts folder not found"));
        return false;
    }
    const QString path = QDir(dir).filePath(suggestedFileName(QStringLiteral("zwo")));
    const QString last = m_lastExportDir;
    const bool ok = exportZwo(path);
    m_lastExportDir = last;   // not a place the user picked
    return ok;
}

void WorkoutFileLibrary::rememberExportDir(const QString &path)
{
    if (!path.isEmpty())
        m_lastExportDir = QFileInfo(path).absolutePath();
}

void WorkoutFileLibrary::reveal(const QString &pathOrUrl) const
{
    const QString path = cleanAbs(toLocalPath(pathOrUrl));
    if (path.isEmpty())
        return;
#if defined(Q_OS_MACOS)
    QProcess::startDetached(QStringLiteral("open"), { QStringLiteral("-R"), path });
#elif defined(Q_OS_WIN)
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            { QStringLiteral("/select,") + QDir::toNativeSeparators(path) });
#else
    const QFileInfo fi(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.isDir() ? path : fi.absolutePath()));
#endif
}

void WorkoutFileLibrary::openFolder() const
{
    QDir().mkpath(m_folder);
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_folder));
}

QString WorkoutFileLibrary::findByName(const QString &name) const
{
    for (const Entry &e : m_entries)
        if (e.workout.name == name)
            return e.path;
    return {};
}

QString WorkoutFileLibrary::nameOf(const QString &path) const
{
    if (const Entry *e = entryFor(toLocalPath(path)))
        return e->error.isEmpty() ? e->workout.name : e->fileName;
    return QFileInfo(path).completeBaseName();
}

QString WorkoutFileLibrary::suggestedFileName(const QString &extension) const
{
    const QString name = m_builder ? m_builder->name() : QString();
    return io::safeFileName(name) + QLatin1Char('.') + extension;
}

QUrl WorkoutFileLibrary::folderUrl() const
{
    return QUrl::fromLocalFile(m_folder);
}

QUrl WorkoutFileLibrary::suggestedSaveUrl(const QString &extension) const
{
    return QUrl::fromLocalFile(QDir(m_folder).filePath(suggestedFileName(extension)));
}

QUrl WorkoutFileLibrary::suggestedExportUrl(const QString &extension) const
{
    const QString dir = m_lastExportDir.isEmpty() ? QDir::homePath() : m_lastExportDir;
    return QUrl::fromLocalFile(QDir(dir).filePath(suggestedFileName(extension)));
}

QString WorkoutFileLibrary::toLocalPath(const QString &pathOrUrl)
{
    if (pathOrUrl.startsWith(QLatin1String("file:"), Qt::CaseInsensitive))
        return QUrl(pathOrUrl).toLocalFile();
    return pathOrUrl;
}

QString WorkoutFileLibrary::nativePath(const QString &path)
{
    QString p = QDir::toNativeSeparators(toLocalPath(path));
    const QString home = QDir::toNativeSeparators(QDir::homePath());
#ifndef Q_OS_WIN
    if (p.startsWith(home + QDir::separator()))
        p = QStringLiteral("~") + p.mid(home.size());
#endif
    return p;
}

QString WorkoutFileLibrary::uniquePath(const QString &folder, const QString &base, const QString &extension,
                                       const QString &ignore)
{
    const QDir dir(folder);
    const QString ign = cleanAbs(ignore);
    QString candidate = dir.filePath(base + QLatin1Char('.') + extension);
    for (int n = 2; QFileInfo::exists(candidate) && cleanAbs(candidate) != ign; ++n)
        candidate = dir.filePath(QStringLiteral("%1 %2.%3").arg(base).arg(n).arg(extension));
    return candidate;
}

} // namespace awc
