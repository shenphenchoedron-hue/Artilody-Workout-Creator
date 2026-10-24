// Artilody Workout Creator – standalone editor for Artilody Watts Trainer
// workouts (.awt) with Zwift (.zwo) import / export.
//
// It shares the workout model, the Builder back-end (WorkoutBuilder) and the
// file formats (WorkoutIO) with Artilody Watts Trainer (../src/core), and the
// QML Builder page / components (../qml). Workouts are plain files in a
// "Workouts folder" – there is no database.

#include "CreatorBackend.h"
#include "CreatorSettings.h"
#include "DemoWorkouts.h"
#include "WorkoutBuilder.h"
#include "WorkoutFileLibrary.h"

#include <QDir>
#include <QFileOpenEvent>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTimer>
#include <memory>

using namespace awc;

namespace {
// Files opened from Finder / Explorer (double click, "Open With", drop on the
// Dock icon) go through Main.qml requestOpen(), which asks about unsaved changes.
class FileOpenFilter : public QObject
{
public:
    QQuickWindow *window = nullptr;
    QStringList pending;
    void open(const QString &path)
    {
        if (window)
            QMetaObject::invokeMethod(window, "requestOpen", Q_ARG(QVariant, path));
        else
            pending << path;
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::FileOpen) {
            const QString file = static_cast<QFileOpenEvent *>(event)->file();
            if (!file.isEmpty()) {
                open(file);
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
};
} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("Artilody"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("artilody.app"));
    QGuiApplication::setApplicationName(QStringLiteral("Artilody Workout Creator"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Artilody Workout Creator"));
    QGuiApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
#ifdef Q_OS_LINUX
    QGuiApplication::setDesktopFileName(QStringLiteral("artilody-workout-creator"));
#endif
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    FileOpenFilter fileOpen;
    app.installEventFilter(&fileOpen);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app-icon-256.png")));

    // Screenshot mode (WATT_SCREENSHOT_DIR=<dir>): temporary settings and a
    // temporary workouts folder seeded with demo workouts; renders a few views
    // to PNG and quits. Never touches the user's settings or files.
    //   WATT_SCREENSHOT_STEP_MS   step time (default 900 ms)
    //   WATT_SCREENSHOT_SIZE      window size (default 1700x992)
    // AWC_SETTINGS_FILE=<file.ini> / AWC_WORKOUTS_DIR=<dir> override the
    // settings store and the workouts folder (tests, launch checks).
    const QString shotDir = qEnvironmentVariable("WATT_SCREENSHOT_DIR");
    std::unique_ptr<QTemporaryDir> tmp;
    QString iniFile = qEnvironmentVariable("AWC_SETTINGS_FILE");
    QString workoutsDir = qEnvironmentVariable("AWC_WORKOUTS_DIR");
    if (!shotDir.isEmpty()) {
        tmp = std::make_unique<QTemporaryDir>();
        if (!tmp->isValid()) {
            qCritical("Screenshot mode: could not create a temporary folder");
            return 1;
        }
        iniFile = tmp->filePath(QStringLiteral("settings.ini"));
        workoutsDir = tmp->filePath(QStringLiteral("Artilody Workouts"));
        demo::seedFolder(workoutsDir);
    }

    CreatorSettings settings(iniFile);
    if (!workoutsDir.isEmpty())
        settings.setWorkoutsFolder(workoutsDir);
    if (!shotDir.isEmpty())
        settings.setLastFile(QString());

    WorkoutFileLibrary library;
    library.setIncludeZwo(settings.showZwoFiles());
    library.setFolder(settings.workoutsFolder());
    QObject::connect(&settings, &CreatorSettings::workoutsFolderChanged, &library,
                     [&] { library.setFolder(settings.workoutsFolder()); });
    QObject::connect(&settings, &CreatorSettings::showZwoFilesChanged, &library,
                     [&] { library.setIncludeZwo(settings.showZwoFiles()); });

    CreatorBackend backend(&settings, &library);
    watt::WorkoutBuilder builder(&backend);
    library.setBuilder(&builder);
    QObject::connect(&library, &WorkoutFileLibrary::currentPathChanged, &settings,
                     [&] { settings.setLastFile(library.currentPath()); });

    // Reopen the workout of the last session.
    if (const QString last = settings.lastFile(); !last.isEmpty() && QFileInfo::exists(last))
        library.openFile(last);

    const char *uri = "Artilody.Backend";
    qmlRegisterSingletonInstance(uri, 1, 0, "Builder", &builder);
    qmlRegisterSingletonInstance(uri, 1, 0, "Library", &library);
    qmlRegisterSingletonInstance(uri, 1, 0, "Settings", &settings);
    // The shared components read FTP / zones / cadence from "Profiles".
    qmlRegisterSingletonInstance(uri, 1, 0, "Profiles", &settings);
    qmlRegisterUncreatableType<watt::IntervalModel>(uri, 1, 0, "IntervalModel", QStringLiteral("Provided by Builder"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appVersion"), QStringLiteral(APP_VERSION));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    engine.loadFromModule("ArtilodyCreator", "Main");
#else
    engine.load(QUrl(QStringLiteral("qrc:/ArtilodyCreator/Main.qml")));
#endif

    QQuickWindow *window = !engine.rootObjects().isEmpty()
        ? qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst()) : nullptr;
    if (window && shotDir.isEmpty()) {
        fileOpen.window = window;
        // a workout file on the command line (Windows / Linux "Open with")
        const QStringList args = QCoreApplication::arguments().mid(1);
        for (const QString &arg : args)
            if (!arg.startsWith(QLatin1Char('-')) && QFileInfo(arg).isFile())
                fileOpen.pending << QFileInfo(arg).absoluteFilePath();
        for (const QString &path : std::as_const(fileOpen.pending))
            fileOpen.open(path);
        fileOpen.pending.clear();
    }
    if (!shotDir.isEmpty() && window) {
        QDir().mkpath(shotDir);
        QSize size(1700, 992);
        const QStringList sz = qEnvironmentVariable("WATT_SCREENSHOT_SIZE").split(QLatin1Char('x'));
        if (sz.size() == 2 && sz.at(0).toInt() > 0 && sz.at(1).toInt() > 0)
            size = QSize(sz.at(0).toInt(), sz.at(1).toInt());
        window->resize(size);

        // name of the PNG -> view prepared by Main.qml prepareScreenshot()
        const QStringList shots { QStringLiteral("main"), QStringLiteral("rowing"), QStringLiteral("light"),
                                  QStringLiteral("export"), QStringLiteral("menu-file"),
                                  QStringLiteral("menu-block"), QStringLiteral("menu-filemenu"),
                                  QStringLiteral("settings"),
                                  QStringLiteral("small") };
        auto *timer = new QTimer(&app);
        auto step = std::make_shared<int>(0);
        QObject::connect(timer, &QTimer::timeout, &app, [=] {
            const int s = (*step)++;
            const int k = s / 2;
            if (k >= shots.size()) {
                QCoreApplication::quit();
                return;
            }
            const QString name = shots.at(k);
            if (s % 2 == 0) {
                if (name == QLatin1String("small"))
                    window->resize(960, 620);
                QMetaObject::invokeMethod(window, "prepareScreenshot", Q_ARG(QVariant, name));
                return;
            }
            window->grabWindow().save(QDir(shotDir).filePath(name + QStringLiteral(".png")));
        });
        timer->start(qEnvironmentVariableIntValue("WATT_SCREENSHOT_STEP_MS") > 0
                         ? qEnvironmentVariableIntValue("WATT_SCREENSHOT_STEP_MS") : 900);
    } else if (!shotDir.isEmpty()) {
        qWarning("Screenshot mode: no window to capture");
        return 1;
    }

    return app.exec();
}
