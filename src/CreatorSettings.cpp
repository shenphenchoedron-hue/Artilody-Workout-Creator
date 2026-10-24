#include "CreatorSettings.h"

#include "Zones.h"

#include <QDir>
#include <QStandardPaths>
#include <algorithm>

namespace awc {

namespace {
const QString kFtp = QStringLiteral("profile/ftp");
const QString kRowingFtp = QStringLiteral("profile/rowingFtp");
const QString kCadence = QStringLiteral("profile/cadence");
const QString kDark = QStringLiteral("ui/darkTheme");
const QString kShowZwo = QStringLiteral("library/showZwo");
const QString kFolder = QStringLiteral("library/folder");
const QString kLastFile = QStringLiteral("library/lastFile");
}

CreatorSettings::CreatorSettings(const QString &iniFile, QObject *parent)
    : QObject(parent)
    , m_settings(iniFile.isEmpty() ? std::make_unique<QSettings>()
                                   : std::make_unique<QSettings>(iniFile, QSettings::IniFormat))
{
    m_ftp = std::clamp(m_settings->value(kFtp, 250).toInt(), minFtp(), maxFtp());
    m_rowingFtp = std::clamp(m_settings->value(kRowingFtp, 150).toInt(), minFtp(), maxFtp());
    m_cadence = std::clamp(m_settings->value(kCadence, 85).toInt(), 40, 140);
    m_dark = m_settings->value(kDark, true).toBool();
    m_showZwo = m_settings->value(kShowZwo, true).toBool();
    m_folder = m_settings->value(kFolder).toString();
    if (m_folder.isEmpty())
        m_folder = defaultWorkoutsFolder();
    m_lastFile = m_settings->value(kLastFile).toString();
}

void CreatorSettings::setFtp(int watts)
{
    watts = std::clamp(watts, minFtp(), maxFtp());
    if (watts == m_ftp)
        return;
    m_ftp = watts;
    m_settings->setValue(kFtp, watts);
    emit profileChanged();
}

void CreatorSettings::setRowingFtp(int watts)
{
    watts = std::clamp(watts, minFtp(), maxFtp());
    if (watts == m_rowingFtp)
        return;
    m_rowingFtp = watts;
    m_settings->setValue(kRowingFtp, watts);
    emit profileChanged();
}

void CreatorSettings::setPreferredCadence(int rpm)
{
    rpm = std::clamp(rpm, 40, 140);
    if (rpm == m_cadence)
        return;
    m_cadence = rpm;
    m_settings->setValue(kCadence, rpm);
    emit profileChanged();
}

QVariantList CreatorSettings::zoneBounds() const
{
    QVariantList list;
    for (double u : watt::ZoneSet().upper)
        list.append(u);
    return list;
}

void CreatorSettings::setDarkTheme(bool dark)
{
    if (dark == m_dark)
        return;
    m_dark = dark;
    m_settings->setValue(kDark, dark);
    emit darkThemeChanged();
}

void CreatorSettings::setShowZwoFiles(bool show)
{
    if (show == m_showZwo)
        return;
    m_showZwo = show;
    m_settings->setValue(kShowZwo, show);
    emit showZwoFilesChanged();
}

void CreatorSettings::setWorkoutsFolder(const QString &folder)
{
    const QString f = folder.isEmpty() ? defaultWorkoutsFolder() : QDir::cleanPath(folder);
    if (f == m_folder)
        return;
    m_folder = f;
    m_settings->setValue(kFolder, f);
    emit workoutsFolderChanged();
}

QString CreatorSettings::defaultWorkoutsFolder()
{
    QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docs.isEmpty())
        docs = QDir::homePath();
    return QDir(docs).filePath(QStringLiteral("Artilody Workouts"));
}

void CreatorSettings::setLastFile(const QString &path)
{
    if (path == m_lastFile)
        return;
    m_lastFile = path;
    m_settings->setValue(kLastFile, path);
    emit lastFileChanged();
}

} // namespace awc
