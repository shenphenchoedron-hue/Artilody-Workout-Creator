// Tests of the Artilody Workout Creator back-end: files written by the
// creator must import unchanged into Artilody Watts Trainer (same WorkoutIO
// code), .zwo export must round-trip, and the folder library must scan,
// filter, save, rename, duplicate and delete files.

#include "CreatorBackend.h"
#include "CreatorSettings.h"
#include "DemoWorkouts.h"
#include "WorkoutBuilder.h"
#include "WorkoutFileLibrary.h"
#include "WorkoutIO.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <cmath>
#include <memory>

using namespace awc;
using namespace watt;
namespace io = watt::workoutio;

namespace {
QByteArray readAll(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

void compareWorkouts(const Workout &a, const Workout &b)
{
    QCOMPARE(a.name, b.name);
    QCOMPARE(a.category, b.category);
    QCOMPARE(a.focus, b.focus);
    QCOMPARE(a.description, b.description);
    QCOMPARE(a.sport, b.sport);
    QCOMPARE(a.intervals.size(), b.intervals.size());
    for (int i = 0; i < a.intervals.size(); ++i)
        QVERIFY2(a.intervals.at(i) == b.intervals.at(i), qPrintable(QStringLiteral("interval %1 differs").arg(i)));
}
} // namespace

class TestCreator : public QObject
{
    Q_OBJECT

    // Creator back-end on a temp folder + temp ini settings.
    struct Env
    {
        QTemporaryDir dir;
        std::unique_ptr<CreatorSettings> settings;
        std::unique_ptr<WorkoutFileLibrary> library;
        std::unique_ptr<CreatorBackend> backend;
        std::unique_ptr<WorkoutBuilder> builder;
        QString folder;

        Env()
        {
            folder = dir.filePath(QStringLiteral("Workouts"));
            settings = std::make_unique<CreatorSettings>(dir.filePath(QStringLiteral("settings.ini")));
            settings->setWorkoutsFolder(folder);
            library = std::make_unique<WorkoutFileLibrary>();
            library->setFolder(folder);
            backend = std::make_unique<CreatorBackend>(settings.get(), library.get());
            builder = std::make_unique<WorkoutBuilder>(backend.get());
            library->setBuilder(builder.get());
        }
    };

private slots:
    void builderWritesAwtThatTheMainAppImports()
    {
        Env env;
        WorkoutBuilder &b = *env.builder;
        b.newWorkout(QStringLiteral("rowing"));
        b.setName(QStringLiteral("Row / Test: 4 × 5"));
        b.setCategory(QStringLiteral("Endurance"));
        b.setFocus(QStringLiteral("Threshold"));
        b.setDescription(QStringLiteral("Created in the creator"));
        b.addBlock(QStringLiteral("ramp"));
        b.setPercent(1, 95);
        b.setCadence(1, 26);
        b.setRpe(1, 4);
        QVERIFY(b.setDurationText(1, QStringLiteral("7:30")));
        b.moveBlock(2, 0);
        QVERIFY(b.dirty());
        // watts follow the creator's rowing FTP
        QCOMPARE(b.ftp(), 150);
        env.settings->setRowingFtp(200);
        QCOMPARE(b.ftp(), 200);

        const qint64 id = b.save();
        QVERIFY2(id >= 0, qPrintable(b.lastError()));
        QVERIFY(!b.dirty());
        const QString path = env.library->currentPath();
        QVERIFY(path.endsWith(QStringLiteral(".awt")));
        QCOMPARE(QFileInfo(path).absolutePath(), QDir(env.folder).absolutePath());
        QCOMPARE(QFileInfo(path).completeBaseName(), io::safeFileName(b.name()));

        // The main app's import (WorkoutIO::parseFile, used by WorkoutLibrary::importFile)
        const io::ImportResult r = io::parseFile(path);
        QVERIFY2(r.ok(), qPrintable(r.error));
        QVERIFY(r.warnings.isEmpty());
        QCOMPARE(r.workouts.size(), 1);
        compareWorkouts(r.workouts.constFirst(), b.currentWorkout());

        // Same bytes as the main app's exporter would write (apart from the
        // informational "app" / "exportedAt" fields).
        QJsonObject creatorDoc = QJsonDocument::fromJson(readAll(path)).object();
        QJsonObject appDoc = QJsonDocument::fromJson(io::toAwt({ b.currentWorkout() }, QStringLiteral("1.1.2"))).object();
        QCOMPARE(creatorDoc.value(QStringLiteral("format")).toString(), QString::fromLatin1(io::kFormatId));
        QCOMPARE(creatorDoc.value(QStringLiteral("version")).toInt(), io::kFormatVersion);
        QVERIFY(creatorDoc.value(QStringLiteral("app")).toString().startsWith(QStringLiteral("Artilody Workout Creator")));
        for (const QString &k : { QStringLiteral("app"), QStringLiteral("exportedAt") }) {
            creatorDoc.remove(k);
            appDoc.remove(k);
        }
        QCOMPARE(creatorDoc, appDoc);

        // reopen: identical workout, clean
        WorkoutBuilder b2(env.backend.get());
        env.library->setBuilder(&b2);
        QVERIFY(env.library->openFile(path));
        compareWorkouts(b2.currentWorkout(), b.currentWorkout());
        QVERIFY(!b2.dirty());
    }

    void saveInPlaceAndFollowRename()
    {
        Env env;
        WorkoutBuilder &b = *env.builder;
        b.setName(QStringLiteral("Tempo"));
        QVERIFY(b.save() >= 0);
        const QString first = env.library->currentPath();
        QCOMPARE(QFileInfo(first).fileName(), QStringLiteral("Tempo.awt"));
        b.setPercent(1, 80);
        QVERIFY(b.save() >= 0);
        QCOMPARE(env.library->currentPath(), first);   // written in place
        QCOMPARE(env.library->totalCount(), 1);
        // renaming the workout renames a file that was named after it
        b.setName(QStringLiteral("Tempo Long"));
        QVERIFY(b.save() >= 0);
        QCOMPARE(QFileInfo(env.library->currentPath()).fileName(), QStringLiteral("Tempo Long.awt"));
        QVERIFY(!QFileInfo::exists(first));
        QCOMPARE(env.library->totalCount(), 1);
        // a second new workout with the same name gets a unique file
        env.library->newWorkout(QStringLiteral("cycling"));
        QVERIFY(env.library->currentPath().isEmpty());
        b.setName(QStringLiteral("Tempo Long"));
        QVERIFY(b.save() >= 0);
        QCOMPARE(QFileInfo(env.library->currentPath()).fileName(), QStringLiteral("Tempo Long 2.awt"));
        QCOMPARE(env.library->totalCount(), 2);
        // Save As
        const QString other = env.dir.filePath(QStringLiteral("usb/Copy"));
        QDir().mkpath(env.dir.filePath(QStringLiteral("usb")));
        QVERIFY(env.library->saveAs(other));
        QCOMPARE(env.library->currentPath(), QDir::cleanPath(other + QStringLiteral(".awt")));
        QVERIFY(!env.library->currentInFolder());
    }

    void zwoExportRoundTrip()
    {
        Env env;
        const QVector<Workout> demos = demo::workouts();
        for (const Workout &w : demos) {
            const io::ImportResult r = io::parseZwo(io::toZwo(w, WorkoutFileLibrary::appName()));
            QVERIFY2(r.ok(), qPrintable(r.error));
            const Workout &back = r.workouts.constFirst();
            QCOMPARE(back.name, w.name);
            QCOMPARE(back.sport, sport::cycling());   // .zwo is always bike
            QCOMPARE(back.totalDurationSec(), w.totalDurationSec());
            QCOMPARE(back.intervals.size(), w.intervals.size());
            for (int i = 0; i < w.intervals.size(); ++i) {
                QVERIFY(qAbs(back.intervals.at(i).startPercent - w.intervals.at(i).startPercent) < 0.006);
                QVERIFY(qAbs(back.intervals.at(i).endPercent - w.intervals.at(i).endPercent) < 0.006);
                QCOMPARE(back.intervals.at(i).cadence, w.intervals.at(i).cadence);
            }
        }
        // through the library API (Builder content -> file)
        QVERIFY(env.library->openFile(QString()) == false);
        env.builder->setName(QStringLiteral("Zwift Export"));
        const QString zwo = env.dir.filePath(QStringLiteral("export"));
        QSignalSpy msg(env.library.get(), &WorkoutFileLibrary::message);
        QVERIFY(env.library->exportZwo(zwo));
        QVERIFY(QFileInfo::exists(zwo + QStringLiteral(".zwo")));
        const QByteArray xml = readAll(zwo + QStringLiteral(".zwo"));
        QVERIFY(xml.contains("<sportType>bike</sportType>"));
        QVERIFY(xml.contains("<author>Artilody Workout Creator</author>"));
        const io::ImportResult r = io::parseFile(zwo + QStringLiteral(".zwo"));
        QVERIFY(r.ok());
        QCOMPARE(r.workouts.constFirst().totalDurationSec(), env.builder->totalDurationSec());
        // rowing exports warn about the bike sport type
        env.builder->setSport(QStringLiteral("rowing"));
        QVERIFY(env.library->exportZwo(zwo + QStringLiteral("-row.zwo")));
        QVERIFY(msg.last().at(0).toString().contains(QStringLiteral("bike")));
        // .awt export anywhere
        QVERIFY(env.library->exportAwt(QUrl::fromLocalFile(env.dir.filePath(QStringLiteral("x.awt"))).toString()));
        QCOMPARE(io::parseFile(env.dir.filePath(QStringLiteral("x.awt"))).workouts.constFirst().sport, sport::rowing());
    }

    void libraryScansFiltersAndManagesFiles()
    {
        Env env;
        QCOMPARE(env.library->totalCount(), 0);
        QCOMPARE(demo::seedFolder(env.folder), 8);
        // a broken file and an unrelated file
        QFile bad(QDir(env.folder).filePath(QStringLiteral("broken.awt")));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("{ not json");
        bad.close();
        QFile txt(QDir(env.folder).filePath(QStringLiteral("notes.txt")));
        QVERIFY(txt.open(QIODevice::WriteOnly));
        txt.close();

        env.library->refresh();
        QCOMPARE(env.library->totalCount(), 9);
        QCOMPARE(env.library->rowCount(), 9);
        // sorted by name, roles present
        const QModelIndex first = env.library->index(0);
        QVERIFY(!env.library->data(first, WorkoutFileLibrary::PathRole).toString().isEmpty());
        int invalid = 0, zwo = 0;
        for (int i = 0; i < env.library->rowCount(); ++i) {
            const QModelIndex idx = env.library->index(i);
            if (!env.library->data(idx, WorkoutFileLibrary::ValidRole).toBool())
                ++invalid;
            if (env.library->data(idx, WorkoutFileLibrary::FormatRole).toString() == QLatin1String("zwo"))
                ++zwo;
            if (env.library->data(idx, WorkoutFileLibrary::ValidRole).toBool())
                QVERIFY(!env.library->data(idx, WorkoutFileLibrary::BarsRole).toList().isEmpty());
        }
        QCOMPARE(invalid, 1);
        QCOMPARE(zwo, 1);

        env.library->setSportFilter(QStringLiteral("rowing"));
        QCOMPARE(env.library->count(), 2);
        env.library->setSportFilter(QStringLiteral("cycling"));
        QCOMPARE(env.library->count(), 6);
        env.library->setSportFilter(QStringLiteral("all"));
        env.library->setFilterText(QStringLiteral("sweet"));
        QCOMPARE(env.library->count(), 2);   // cycling + rowing sweet spot
        env.library->setFilterText(QStringLiteral("FTP Tests"));   // category
        QCOMPARE(env.library->count(), 1);
        env.library->setFilterText(QString());
        env.library->setIncludeZwo(false);
        QCOMPARE(env.library->totalCount(), 8);
        env.library->setIncludeZwo(true);

        // open / duplicate / rename / delete
        const QString ss = env.library->findByName(QStringLiteral("Sweet Spot 3 × 12"));
        QVERIFY(!ss.isEmpty());
        QVERIFY(env.library->openFile(ss));
        QCOMPARE(env.builder->name(), QStringLiteral("Sweet Spot 3 × 12"));
        QCOMPARE(env.library->currentPath(), ss);
        const QString copy = env.library->duplicateFile(ss);
        QVERIFY(QFileInfo::exists(copy));
        QCOMPARE(io::parseFile(copy).workouts.constFirst().name, QStringLiteral("Sweet Spot 3 × 12 (copy)"));
        QCOMPARE(env.library->totalCount(), 10);

        QVERIFY(env.library->renameFile(ss, QStringLiteral("Sweet Spot Classic")));
        QVERIFY(!QFileInfo::exists(ss));
        QCOMPARE(QFileInfo(env.library->currentPath()).fileName(), QStringLiteral("Sweet Spot Classic.awt"));
        QCOMPARE(env.builder->name(), QStringLiteral("Sweet Spot Classic"));
        QVERIFY(!env.builder->dirty());

        QVERIFY(env.library->removeFile(copy));
        QVERIFY(!QFileInfo::exists(copy));
        QCOMPARE(env.library->totalCount(), 9);

        // opening a .zwo and saving writes an .awt next to it
        const QString zwoPath = env.library->findByName(QStringLiteral("Zwift Over-Unders"));
        QVERIFY(env.library->openFile(zwoPath));
        env.builder->setPercent(1, 110);
        QVERIFY(env.builder->save() >= 0);
        QVERIFY(QFileInfo::exists(zwoPath));
        QCOMPARE(QFileInfo(env.library->currentPath()).suffix(), QStringLiteral("awt"));
        QCOMPARE(env.library->totalCount(), 10);
    }

    void importSplitsCollections()
    {
        Env env;
        QTemporaryDir src;
        // a multi-workout export as written by Artilody Watts Trainer
        const QString multi = src.filePath(QStringLiteral("export.awt"));
        QVERIFY(io::writeFile(multi, io::toAwt(demo::workouts().mid(0, 3), QStringLiteral("1.1.2"))));
        const QString zwo = src.filePath(QStringLiteral("z.zwo"));
        QVERIFY(io::writeFile(zwo, io::toZwo(demo::workouts().at(4))));
        const QString junk = src.filePath(QStringLiteral("junk.awt"));
        QVERIFY(io::writeFile(junk, "nope"));
        QCOMPARE(env.library->importFiles({ multi, QUrl::fromLocalFile(zwo), junk }), 4);
        QCOMPARE(env.library->totalCount(), 4);
        for (const auto &e : env.library->entries())
            QCOMPARE(e.format, QStringLiteral("awt"));
        // opening a collection splits it and opens the first workout
        QVERIFY(env.library->openFile(multi));
        QCOMPARE(env.library->totalCount(), 7);
        QCOMPARE(env.builder->name(), demo::workouts().at(0).name);
    }

    void exportAllWritesEveryShownWorkout()
    {
        Env env;
        QCOMPARE(demo::seedFolder(env.folder), 8);
        env.library->refresh();
        QCOMPARE(env.library->exportAllCount(), 8);
        QVERIFY(!env.library->filtered());

        QTemporaryDir out;
        const QString zwoDir = out.filePath(QStringLiteral("zwift series"));   // created on demand
        QCOMPARE(env.library->exportAll(zwoDir, QStringLiteral("zwo")), 8);
        const QFileInfoList zwos = QDir(zwoDir).entryInfoList({ QStringLiteral("*.zwo") }, QDir::Files);
        QCOMPARE(zwos.size(), 8);
        for (const QFileInfo &fi : zwos)
            QVERIFY2(io::parseFile(fi.absoluteFilePath()).ok(), qPrintable(fi.fileName()));

        // .awt export is identical to the files in the folder (and re-exporting replaces them)
        QCOMPARE(env.library->exportAll(QUrl::fromLocalFile(out.path()).toString(), QStringLiteral("awt")), 8);
        QCOMPARE(env.library->exportAll(out.path(), QStringLiteral("awt")), 8);
        QCOMPARE(QDir(out.path()).entryList({ QStringLiteral("*.awt") }, QDir::Files).size(), 8);
        for (const auto &e : env.library->entries()) {
            const QString exported = QDir(out.path()).filePath(io::safeFileName(e.workout.name) + QStringLiteral(".awt"));
            compareWorkouts(io::parseFile(exported).workouts.constFirst(), e.workout);
        }

        // never into the Workouts folder itself (or a folder inside it)
        const QByteArray before = readAll(env.library->entries().constFirst().path);
        QCOMPARE(env.library->exportAll(env.folder, QStringLiteral("awt")), 0);
        QCOMPARE(env.library->exportAll(QDir(env.folder).filePath(QStringLiteral("sub")), QStringLiteral("zwo")), 0);
        QVERIFY(!QDir(env.folder).exists(QStringLiteral("sub")));
        QVERIFY(!env.library->lastError().isEmpty());
        QCOMPARE(readAll(env.library->entries().constFirst().path), before);
        env.library->refresh();
        QCOMPARE(env.library->totalCount(), 8);

        // only the workouts shown in the list
        env.library->setSportFilter(QStringLiteral("rowing"));
        QVERIFY(env.library->filtered());
        const int rowing = env.library->exportAllCount();
        QVERIFY(rowing > 0 && rowing < 8);
        QTemporaryDir out2;
        QCOMPARE(env.library->exportAll(out2.path(), QStringLiteral("awt")), rowing);
    }

    void builderGeneratesStepsAndIntervalSets()
    {
        Env env;
        WorkoutBuilder &b = *env.builder;   // warm up, sweet spot, cool down
        b.setSelectedIndex(1);

        // 5 × 30 s @ 120 % + 1 min @ 55 %, inserted after the selection, all selected
        b.addIntervalSet(5, 30, 120, 60, 55, true);
        QCOMPARE(b.blockCount(), 13);
        QCOMPARE(b.selectionCount(), 10);
        QCOMPARE(b.selectedRows().constFirst(), 2);
        const auto &items = b.intervals()->intervals();
        QCOMPARE(items.at(2).durationSec, 30);
        QCOMPARE(items.at(2).startPercent, 1.20);
        QCOMPARE(items.at(3).durationSec, 60);
        QCOMPARE(items.at(3).endPercent, 0.55);
        QCOMPARE(items.at(11).type, BlockType::Recovery);
        b.addIntervalSet(3, 60, 110, 60, 50, false);   // no recovery after the last interval
        QCOMPARE(b.blockCount(), 18);

        // falling staircase 100 → 60 % in 5 steps of 2 min
        b.setSelectedIndex(0);
        b.addSteps(5, 120, 100, 60);
        QCOMPARE(b.selectionCount(), 5);
        QList<int> percents;
        for (int r : b.selectedRows()) {
            QCOMPARE(b.intervals()->intervals().at(r).durationSec, 120);
            percents << int(std::lround(b.intervals()->intervals().at(r).startPercent * 100));
        }
        QCOMPARE(percents, QList<int>({ 100, 90, 80, 70, 60 }));

        b.addRamp(300, 80, 40);
        QVERIFY(b.intervals()->intervals().at(b.selectedIndex()).isRamp());
        b.setEndPercent(b.selectedIndex(), 30);
        QCOMPARE(b.intervals()->intervals().at(b.selectedIndex()).endPercent, 0.30);

        // still a plain workout: saves and re-imports unchanged
        QVERIFY(b.save() >= 0);
        compareWorkouts(io::parseFile(env.library->currentPath()).workouts.constFirst(), b.currentWorkout());
    }

    void builderMovesSelectionAsAGroup()
    {
        Env env;
        WorkoutBuilder &b = *env.builder;
        auto names = [&] {
            QStringList l;
            for (const auto &i : b.intervals()->intervals())
                l << i.name;
            return l;
        };
        for (int k = 0; k < 3; ++k)
            b.addBlock(QStringLiteral("steady"));
        for (int r = 0; r < b.blockCount(); ++r)
            b.setBlockName(r, QStringLiteral("B%1").arg(r));
        QCOMPARE(names(), QStringList({ "B0", "B1", "B2", "B3", "B4", "B5" }));

        b.setSelectedIndex(1);
        b.selectRange(2);                 // B1, B2
        QCOMPARE(b.selectionCount(), 2);
        b.shiftSelection(1);
        QCOMPARE(names(), QStringList({ "B0", "B3", "B1", "B2", "B4", "B5" }));
        QCOMPARE(b.selectedRows(), QVector<int>({ 2, 3 }));
        b.moveSelection(b.blockCount());  // to the end (clamped)
        QCOMPARE(names(), QStringList({ "B0", "B3", "B4", "B5", "B1", "B2" }));
        b.shiftSelection(1);              // already at the end: no change
        QCOMPARE(names(), QStringList({ "B0", "B3", "B4", "B5", "B1", "B2" }));

        // non-contiguous selection becomes one group
        b.setSelectedIndex(0);
        b.toggleSelected(3);              // B0, B5
        QCOMPARE(b.selectedIndex(), 3);
        b.moveSelection(1);
        QCOMPARE(names(), QStringList({ "B3", "B0", "B5", "B4", "B1", "B2" }));
        QCOMPARE(b.selectedRows(), QVector<int>({ 1, 2 }));
        QCOMPARE(b.selectedIndex(), 2);   // the primary block followed its block

        b.duplicateSelection();
        QCOMPARE(names(), QStringList({ "B3", "B0", "B5", "B0", "B5", "B4", "B1", "B2" }));
        QCOMPARE(b.selectedRows(), QVector<int>({ 3, 4 }));
        b.removeSelection();
        QCOMPARE(names(), QStringList({ "B3", "B0", "B5", "B4", "B1", "B2" }));
        QCOMPARE(b.selectionCount(), 1);

        b.selectAll();
        QCOMPARE(b.selectionCount(), 6);
        b.setSelectedIndex(2);            // a plain click collapses the selection
        QCOMPARE(b.selectionCount(), 1);
        QVERIFY(b.dirty());
    }

    void settingsPersist()
    {
        QTemporaryDir dir;
        const QString ini = dir.filePath(QStringLiteral("s.ini"));
        {
            CreatorSettings s(ini);
            QCOMPARE(s.ftp(), 250);
            QCOMPARE(s.workoutsFolder(), CreatorSettings::defaultWorkoutsFolder());
            s.setFtp(280);
            s.setRowingFtp(9999);   // clamped
            s.setDarkTheme(false);
            s.setWorkoutsFolder(dir.filePath(QStringLiteral("W")));
        }
        CreatorSettings s(ini);
        QCOMPARE(s.ftp(), 280);
        QCOMPARE(s.rowingFtp(), CreatorSettings::maxFtp());
        QVERIFY(!s.darkTheme());
        QCOMPARE(s.workoutsFolder(), QDir::cleanPath(dir.filePath(QStringLiteral("W"))));
        QCOMPARE(s.zoneBounds().size(), 5);
    }
};

QTEST_MAIN(TestCreator)
#include "tst_creator.moc"
