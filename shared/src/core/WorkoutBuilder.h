#pragma once

#include "BuilderBackend.h"
#include "Workout.h"

#include <QAbstractListModel>
#include <QVariantMap>
#include <functional>

namespace watt {

class Database;
class ProfileManager;

// List model of the blocks of the workout being edited.
class IntervalModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1, TypeRole, TypeNameRole, DurationRole, DurationTextRole, WattsRole,
        EndWattsRole, PercentRole, EndPercentRole, CadenceRole, RpeRole, ZoneRole, RampRole
    };

    explicit IntervalModel(BuilderBackend *backend, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QVector<Interval> &intervals() const { return m_items; }
    void setIntervals(const QVector<Interval> &items);
    void update(int row, const Interval &interval);
    void insert(int row, const Interval &interval);
    void remove(int row);
    void move(int from, int to);
    void refreshAll();
    // Watts are shown relative to the FTP of this sport (see ftpForSport()).
    void setSport(const QString &sport);

private:
    int ftp() const;
    BuilderBackend *m_backend;
    QVector<Interval> m_items;
    QString m_sport = sport::cycling();
};

// Visual workout editor backend. Exposed to QML as "Builder".
class WorkoutBuilder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 workoutId READ workoutId NOTIFY workoutChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY workoutChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY workoutChanged)
    Q_PROPERTY(QString focus READ focus WRITE setFocus NOTIFY workoutChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY workoutChanged)
    // "cycling" (default) or "rowing"; saved with the workout. Watts are % of
    // the profile's FTP or rowing FTP accordingly.
    Q_PROPERTY(QString sport READ sport WRITE setSport NOTIFY workoutChanged)
    Q_PROPERTY(QStringList sports READ sports CONSTANT)
    Q_PROPERTY(int ftp READ ftp NOTIFY ftpChanged)   // also on profile / FTP changes
    Q_PROPERTY(IntervalModel *intervals READ intervals CONSTANT)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedChanged)
    Q_PROPERTY(QVariantMap selected READ selected NOTIFY selectedChanged)
    // Multi-selection (Ctrl/⌘-click, Shift-click, select all). selectedIndex
    // is the "primary" block shown in Interval Settings; selection lists all
    // selected rows in ascending order and always contains selectedIndex.
    Q_PROPERTY(QVariantList selection READ selection NOTIFY selectedChanged)
    Q_PROPERTY(int selectionCount READ selectionCount NOTIFY selectedChanged)
    Q_PROPERTY(int selectionDurationSec READ selectionDurationSec NOTIFY selectionDurationChanged)
    Q_PROPERTY(int totalDurationSec READ totalDurationSec NOTIFY contentChanged)
    Q_PROPERTY(int blockCount READ blockCount NOTIFY contentChanged)
    Q_PROPERTY(QVariantList segments READ segments NOTIFY contentChanged)
    Q_PROPERTY(int intensity READ intensity NOTIFY contentChanged)
    Q_PROPERTY(int tss READ tss NOTIFY contentChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
    Q_PROPERTY(QStringList rpeOptions READ rpeOptions CONSTANT)
    Q_PROPERTY(QStringList categories READ categories CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    // backend: settings + workout storage (not owned). The Watts Trainer app
    // uses the convenience constructor below (AppBuilderBackend, defined in
    // AppBuilderBackend.cpp); the Workout Creator passes its file backend.
    explicit WorkoutBuilder(BuilderBackend *backend, QObject *parent = nullptr);
    WorkoutBuilder(Database *db, ProfileManager *profiles, QObject *parent = nullptr);

    qint64 workoutId() const { return m_workout.id; }
    QString name() const { return m_workout.name; }
    void setName(const QString &name);
    QString category() const { return m_workout.category; }
    void setCategory(const QString &c);
    QString focus() const { return m_workout.focus; }
    void setFocus(const QString &f);
    QString description() const { return m_workout.description; }
    void setDescription(const QString &d);
    QString sport() const { return m_workout.sport; }
    void setSport(const QString &sport);
    QStringList sports() const { return sport::all(); }
    int ftp() const;   // FTP of the current workout's sport
    IntervalModel *intervals() { return &m_model; }
    int selectedIndex() const { return m_selected; }
    void setSelectedIndex(int index);
    QVariantMap selected() const;
    QVariantList selection() const;
    int selectionCount() const { return int(m_selection.size()); }
    int selectionDurationSec() const;
    const QVector<int> &selectedRows() const { return m_selection; }
    int totalDurationSec() const;
    int blockCount() const { return m_model.rowCount(); }
    QVariantList segments() const;
    int intensity() const;
    int tss() const;
    bool dirty() const { return m_dirty; }
    QStringList rpeOptions() const;
    QStringList categories() const;
    QString lastError() const { return m_lastError; }

    Workout currentWorkout() const;

    // sport: "cycling" (default when empty) or "rowing"
    Q_INVOKABLE void newWorkout(const QString &sport = QString());
    Q_INVOKABLE bool loadWorkout(qint64 id);
    Q_INVOKABLE qint64 save();     // returns id or -1

    // Block editing (index = block row)
    Q_INVOKABLE void addBlock(const QString &type);     // warmup, steady, intervals, recovery, cooldown, ramp
    Q_INVOKABLE void addInterval();                     // steady block after the selection
    Q_INVOKABLE void duplicateBlock(int index);
    Q_INVOKABLE void removeBlock(int index);
    Q_INVOKABLE void moveBlock(int from, int to);
    Q_INVOKABLE void setBlockName(int index, const QString &name);
    Q_INVOKABLE void setDuration(int index, int seconds);
    Q_INVOKABLE bool setDurationText(int index, const QString &text);
    Q_INVOKABLE void setWatts(int index, int watts);          // start power (and end for non-ramps)
    Q_INVOKABLE void setPercent(int index, int percent);      // % FTP (start; end too for non-ramps)
    Q_INVOKABLE void setEndPercent(int index, int percent);   // % FTP, makes the block a ramp
    Q_INVOKABLE void setCadence(int index, int rpm);
    Q_INVOKABLE void setRpe(int index, int zone);             // -1 = auto

    // Generated block groups, inserted after the selection. The new blocks
    // are plain blocks (so .awt / .zwo files stay compatible) and are all
    // selected afterwards, so they can be moved or edited together.
    // Smooth ramp: one block going linearly from start to end.
    Q_INVOKABLE void addRamp(int durationSec, int startPercent, int endPercent);
    // Staircase: count steady steps of stepSec each, evenly from startPercent
    // to endPercent (end < start = descending staircase).
    Q_INVOKABLE void addSteps(int count, int stepSec, int startPercent, int endPercent);
    // repeats × (work + recovery); recoveryAfterLast = false drops the last recovery.
    Q_INVOKABLE void addIntervalSet(int repeats, int workSec, int workPercent, int restSec, int restPercent,
                                    bool recoveryAfterLast = true);

    // Multi-selection
    Q_INVOKABLE void toggleSelected(int index);   // Ctrl/⌘-click
    Q_INVOKABLE void selectRange(int index);      // Shift-click: primary block .. index
    Q_INVOKABLE void selectAll();
    // Moves the selected blocks as one contiguous group whose first block
    // ends up at row dest (clamped).
    Q_INVOKABLE void moveSelection(int dest);
    Q_INVOKABLE void shiftSelection(int delta);   // -1 = one block earlier, +1 = later
    Q_INVOKABLE void duplicateSelection();        // copies go after the last selected block
    Q_INVOKABLE void removeSelection();
    // Batch edits (e.g. one change applied to every selected block): block
    // edits between beginBatch() and endBatch() emit contentChanged /
    // selectedChanged once, at endBatch(), instead of once per block.
    Q_INVOKABLE void beginBatch();
    Q_INVOKABLE void endBatch();
    // All roles of one block (as `selected`); the second argument only creates
    // a QML binding dependency on the content.
    Q_INVOKABLE QVariantMap block(int index, int contentRevision = 0) const;
    Q_INVOKABLE int blockAtSecond(int second) const;
    // second argument only creates a QML binding dependency on the content
    Q_INVOKABLE int blockStartSec(int index, int contentRevision = 0) const;

signals:
    void workoutChanged();
    void ftpChanged();
    void selectedChanged();
    void selectionDurationChanged();   // on content and selection changes
    void contentChanged();
    void dirtyChanged();
    void lastErrorChanged();
    void saved(qint64 id);

private:
    void modify(int index, const std::function<void(Interval &)> &fn);
    void markDirty();
    void setError(const QString &e);
    void resetSelection();                 // selection = { m_selected }
    int insertRow() const;                 // where new blocks go: after the selection
    void insertGroup(int at, const QVector<Interval> &blocks);

    BuilderBackend *m_backend;
    IntervalModel m_model;
    Workout m_workout;
    int m_selected = -1;
    QVector<int> m_selection;              // ascending, contains m_selected
    int m_batchDepth = 0;
    bool m_batchContent = false;           // contentChanged pending until endBatch()
    bool m_batchSelected = false;          // selectedChanged pending until endBatch()
    bool m_dirty = false;
    QString m_lastError;
};

} // namespace watt
