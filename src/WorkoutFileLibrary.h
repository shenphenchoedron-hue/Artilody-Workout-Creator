#pragma once

#include "Workout.h"

#include <QAbstractListModel>
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QHash>
#include <QStringList>
#include <QTimer>
#include <optional>

namespace watt { class WorkoutBuilder; }

namespace awc {

// File based workout library of the Workout Creator: every .awt (and,
// optionally, Zwift .zwo) file in the "Workouts folder" is one entry. No
// database – the files are the library, so they can be copied to the training
// computer (Artilody Watts Trainer → Workouts → Import) or to Zwift.
//
// Exposed to QML as "Library". The Builder (shared WorkoutBuilder) reads and
// writes workouts through CreatorBackend, which forwards to workout() /
// storeWorkout() below. Workout ids are session-local handles for file paths.
class WorkoutFileLibrary : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(bool includeZwo READ includeZwo WRITE setIncludeZwo NOTIFY includeZwoChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterChanged)
    Q_PROPERTY(QString sportFilter READ sportFilter WRITE setSportFilter NOTIFY filterChanged)   // all|cycling|rowing
    Q_PROPERTY(int count READ count NOTIFY countChanged)              // entries after filtering
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countChanged)    // all files in the folder
    // workouts that "Export all" writes: every readable file shown in the list
    Q_PROPERTY(int exportAllCount READ exportAllCount NOTIFY countChanged)
    Q_PROPERTY(bool filtered READ filtered NOTIFY filterChanged)       // search / sport filter active
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)   // file in the Builder ("" = unsaved)
    Q_PROPERTY(QString currentFileName READ currentFileName NOTIFY currentPathChanged)
    Q_PROPERTY(bool currentInFolder READ currentInFolder NOTIFY currentPathChanged)
    Q_PROPERTY(QStringList zwiftFolders READ zwiftFolders NOTIFY zwiftFoldersChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1, FileNameRole, FormatRole, NameRole, SportRole, CategoryRole, FocusRole,
        DurationRole, DurationTextRole, BlockCountRole, WorkoutCountRole, ModifiedRole, ValidRole, ErrorRole,
        BarsRole
    };

    struct Entry
    {
        QString path;
        QString fileName;
        QString format;          // "awt" | "zwo"
        QDateTime modified;
        QString error;           // non-empty: file could not be read
        int workoutCount = 0;
        watt::Workout workout;   // first workout of the file
    };

    explicit WorkoutFileLibrary(QObject *parent = nullptr);

    static QString appName() { return QStringLiteral("Artilody Workout Creator"); }

    void setBuilder(watt::WorkoutBuilder *builder) { m_builder = builder; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString folder() const { return m_folder; }
    void setFolder(const QString &folder);
    bool includeZwo() const { return m_includeZwo; }
    void setIncludeZwo(bool include);
    QString filterText() const { return m_filterText; }
    void setFilterText(const QString &text);
    QString sportFilter() const { return m_sportFilter; }
    void setSportFilter(const QString &sport);
    int count() const { return int(m_view.size()); }
    int totalCount() const { return int(m_entries.size()); }
    int exportAllCount() const;
    bool filtered() const;
    QString currentPath() const { return m_currentPath; }
    QString currentFileName() const;
    bool currentInFolder() const;
    QStringList zwiftFolders() const;
    QString lastError() const { return m_lastError; }
    const QVector<Entry> &entries() const { return m_entries; }

    // --- storage used by the Builder (CreatorBackend) ---------------------
    std::optional<watt::Workout> workout(qint64 id) const;
    // Writes the workout to its file (or a new .awt in the folder, or the
    // pending "Save As" target). Returns its id or -1 (lastError).
    qint64 storeWorkout(const watt::Workout &workout);
    qint64 idForPath(const QString &path);
    QString pathForId(qint64 id) const { return m_idPaths.value(id); }

    // --- QML API (paths may be local paths or file: URLs) --------------------
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void newWorkout(const QString &sport = QString());
    Q_INVOKABLE bool openFile(const QString &path);
    Q_INVOKABLE bool saveAs(const QString &path);
    Q_INVOKABLE QString duplicateFile(const QString &path);
    Q_INVOKABLE bool renameFile(const QString &path, const QString &newName);
    Q_INVOKABLE bool removeFile(const QString &path);
    Q_INVOKABLE int importFiles(const QVariantList &paths);   // copies into the folder as .awt
    Q_INVOKABLE bool exportZwo(const QString &path);
    Q_INVOKABLE bool exportAwt(const QString &path);
    Q_INVOKABLE bool exportToZwift(const QString &zwiftFolder);   // copies <name>.zwo into it
    // "Export all": writes every workout shown in the list (all of them when
    // no search / sport filter is set) as one file each – format "zwo" or
    // "awt" – into folder. Files of the same name are replaced. Returns the
    // number of files written.
    Q_INVOKABLE int exportAll(const QString &folder, const QString &format);
    Q_INVOKABLE int exportAllToZwift(const QString &zwiftFolder);
    Q_INVOKABLE void reveal(const QString &path) const;          // Finder / Explorer / file manager
    Q_INVOKABLE void openFolder() const;
    Q_INVOKABLE bool ensureFolder();
    Q_INVOKABLE QString findByName(const QString &name) const;   // path of the first entry with that workout name
    Q_INVOKABLE QString nameOf(const QString &path) const;
    Q_INVOKABLE QString suggestedFileName(const QString &extension) const;   // "<Workout>.zwo"
    Q_INVOKABLE QUrl folderUrl() const;
    Q_INVOKABLE QUrl suggestedSaveUrl(const QString &extension) const;     // in the workouts folder
    Q_INVOKABLE QUrl suggestedExportUrl(const QString &extension) const;   // last export location
    Q_INVOKABLE static QString toLocalPath(const QString &pathOrUrl);
    Q_INVOKABLE static QString nativePath(const QString &path);

    // Unique "<folder>/<base>.<ext>" (adds " 2", " 3", … when taken).
    static QString uniquePath(const QString &folder, const QString &base, const QString &extension,
                              const QString &ignore = QString());

signals:
    void folderChanged();
    void includeZwoChanged();
    void filterChanged();
    void countChanged();
    void currentPathChanged();
    void zwiftFoldersChanged();
    void lastErrorChanged();
    void message(const QString &text);   // user feedback (toast)

private:
    void setCurrentPath(const QString &path);
    void setError(const QString &e);
    void applyFilter();
    void watchFolder();
    QByteArray awtBytes(const watt::Workout &w) const;
    bool writeWorkoutFile(const QString &path, const watt::Workout &w);
    void movePathId(const QString &from, const QString &to);
    const Entry *entryFor(const QString &path) const;
    QVector<watt::Workout> readFile(const QString &path, QString *error) const;
    void rememberExportDir(const QString &path);

    watt::WorkoutBuilder *m_builder = nullptr;
    QString m_folder;
    bool m_includeZwo = true;
    QString m_filterText;
    QString m_sportFilter = QStringLiteral("all");
    QVector<Entry> m_entries;
    QVector<int> m_view;          // indices into m_entries
    QString m_currentPath;
    QString m_saveAsPath;         // pending "Save As" target
    QString m_lastExportDir;
    QString m_lastError;
    QHash<QString, qint64> m_pathIds;
    QHash<qint64, QString> m_idPaths;
    qint64 m_nextId = 1;
    QFileSystemWatcher m_watcher;
    QTimer m_rescan;
};

} // namespace awc
