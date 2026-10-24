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
