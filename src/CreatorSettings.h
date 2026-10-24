#pragma once

#include <QObject>
#include <QSettings>
#include <QVariantList>
#include <memory>

namespace awc {

// Persistent settings of the Workout Creator (QSettings). Exposed to QML as
// "Settings" and – for the components shared with Artilody Watts Trainer
// (WorkoutChart, BuilderPage) – also as "Profiles": it provides the same
// ftp / rowingFtp / preferredCadence / zoneBounds properties.
//
// FTP values only affect the watts preview; workout files store % of FTP.
class CreatorSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int ftp READ ftp WRITE setFtp NOTIFY profileChanged)
    Q_PROPERTY(int rowingFtp READ rowingFtp WRITE setRowingFtp NOTIFY profileChanged)
    Q_PROPERTY(int preferredCadence READ preferredCadence WRITE setPreferredCadence NOTIFY profileChanged)
    Q_PROPERTY(QVariantList zoneBounds READ zoneBounds NOTIFY profileChanged)   // fractions of FTP
    Q_PROPERTY(int minFtp READ minFtp CONSTANT)
    Q_PROPERTY(int maxFtp READ maxFtp CONSTANT)
    Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)
    Q_PROPERTY(bool showZwoFiles READ showZwoFiles WRITE setShowZwoFiles NOTIFY showZwoFilesChanged)
    Q_PROPERTY(QString workoutsFolder READ workoutsFolder WRITE setWorkoutsFolder NOTIFY workoutsFolderChanged)
    Q_PROPERTY(QString defaultWorkoutsFolder READ defaultWorkoutsFolder CONSTANT)
    Q_PROPERTY(QString lastFile READ lastFile WRITE setLastFile NOTIFY lastFileChanged)

public:
    // iniFile empty: the platform's native settings store; otherwise an .ini
    // file (screenshot mode, tests, AWC_SETTINGS_FILE).
    explicit CreatorSettings(const QString &iniFile = QString(), QObject *parent = nullptr);

    int ftp() const { return m_ftp; }
    void setFtp(int watts);
    int rowingFtp() const { return m_rowingFtp; }
    void setRowingFtp(int watts);
    int preferredCadence() const { return m_cadence; }
    void setPreferredCadence(int rpm);
    QVariantList zoneBounds() const;
    static int minFtp() { return 50; }
    static int maxFtp() { return 600; }

    bool darkTheme() const { return m_dark; }
    void setDarkTheme(bool dark);
    bool showZwoFiles() const { return m_showZwo; }
    void setShowZwoFiles(bool show);
    QString workoutsFolder() const { return m_folder; }
    void setWorkoutsFolder(const QString &folder);
    static QString defaultWorkoutsFolder();
    QString lastFile() const { return m_lastFile; }
    void setLastFile(const QString &path);

signals:
    void profileChanged();
    void darkThemeChanged();
    void showZwoFilesChanged();
    void workoutsFolderChanged();
    void lastFileChanged();

private:
    std::unique_ptr<QSettings> m_settings;
    int m_ftp = 250;
    int m_rowingFtp = 150;
    int m_cadence = 85;
    bool m_dark = true;
    bool m_showZwo = true;
    QString m_folder;
    QString m_lastFile;
};

} // namespace awc
