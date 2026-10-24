#include "WorkoutIO.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <algorithm>
#include <cmath>

namespace watt::workoutio {

namespace {

constexpr int kMaxNameLength = 100;
constexpr int kMaxFocusLength = 100;
constexpr int kMaxDescriptionLength = 4000;
constexpr int kMaxIntervalNameLength = 60;
constexpr int kMaxRepeat = 100;
constexpr double kFreeRideFraction = 0.60;
constexpr double kMaxEffortFraction = 1.50;

QString defaultWorkoutName() { return QStringLiteral("Imported workout"); }

QString tooLargeError()
{
    return QStringLiteral("The file is too large to be a workout file (maximum %1 MB).")
        .arg(kMaxFileBytes / (1024 * 1024));
}

QString tooManyIntervalsError(const QString &name)
{
    return QStringLiteral("\"%1\" has more than %2 intervals – the file was not imported.").arg(name).arg(kMaxIntervals);
}

QByteArray stripBom(const QByteArray &data)
{
    if (data.startsWith("\xEF\xBB\xBF"))
        return data.mid(3);
    return data;
}

QString limited(QString text, int maxLength)
{
    text = text.trimmed();
    if (text.size() > maxLength)
        text = text.left(maxLength).trimmed();
    return text;
}

// Formats a fraction of FTP without trailing zeros ("0.9", "1.075").
QString fractionText(double v)
{
    QString s = QString::number(v, 'f', 3);
    while (s.contains(QLatin1Char('.')) && (s.endsWith(QLatin1Char('0')) || s.endsWith(QLatin1Char('.'))))
        s.chop(1);
    return s;
}

// Sanitises the values of one workout and records each kind of adjustment
// once as a human readable warning.
class Cleaner
{
public:
    explicit Cleaner(QStringList *warnings) : m_warnings(warnings) {}

    void begin(const QString &label)
    {
        m_label = label;
        m_seen.clear();
    }

    void warn(const QString &key, const QString &text)
    {
        if (m_seen.contains(key))
            return;
        m_seen.insert(key);
        m_warnings->append(m_label.isEmpty() ? text : QStringLiteral("%1: %2").arg(m_label, text));
    }

    int duration(double sec)
    {
        if (!std::isfinite(sec))
            sec = 0;
        const double rounded = std::round(sec);
        if (rounded < kMinIntervalSec) {
            warn(QStringLiteral("durMin"), QStringLiteral("very short intervals were lengthened to %1 s").arg(kMinIntervalSec));
            return kMinIntervalSec;
        }
        if (rounded > kMaxIntervalSec) {
            warn(QStringLiteral("durMax"), QStringLiteral("intervals longer than 6 hours were shortened"));
            return kMaxIntervalSec;
        }
        return int(rounded);
    }

    double fraction(double v)
    {
        if (!std::isfinite(v) || v < 0) {
            warn(QStringLiteral("neg"), QStringLiteral("invalid power targets were set to 0"));
            return 0.0;
        }
        if (v > kMaxFraction) {
            // Looks like a percentage (e.g. 75 instead of 0.75).
            warn(QStringLiteral("pct"), QStringLiteral("power targets given in percent were converted to fractions of FTP"));
            v /= 100.0;
            if (v > kMaxFraction) {
                warn(QStringLiteral("pctMax"), QStringLiteral("power targets above 400 % FTP were limited"));
                v = kMaxFraction;
            }
        }
        return std::round(v * 10000.0) / 10000.0;   // drop float noise, keep 0.01 % precision
    }

    int cadence(double v)
    {
        if (!std::isfinite(v) || v <= 0)
            return 0;
        if (v > kMaxCadence) {
            warn(QStringLiteral("cad"), QStringLiteral("cadence targets above %1 rpm were limited").arg(kMaxCadence));
            return kMaxCadence;
        }
        return int(std::lround(v));
    }

    int rpe(double v)
    {
        if (!std::isfinite(v))
            return -1;
        const int r = int(std::lround(v));
        if (r < -1 || r > 5) {
            warn(QStringLiteral("rpe"), QStringLiteral("invalid RPE zones were reset"));
            return -1;
        }
        return r;
    }

private:
    QStringList *m_warnings;
    QString m_label;
    QSet<QString> m_seen;
};

QString matchCategory(const QString &raw)
{
    QString t = raw.toLower();
    t.remove(QRegularExpression(QStringLiteral("[^a-z]")));
    if (t.isEmpty())
        return {};
    if (t == QLatin1String("endurance") || t == QLatin1String("base") || t == QLatin1String("tempo"))
        return QStringLiteral("Endurance");
    if (t == QLatin1String("intervals") || t == QLatin1String("interval"))
        return QStringLiteral("Intervals");
    if (t == QLatin1String("recovery"))
        return QStringLiteral("Recovery");
    if (t == QLatin1String("ftptests") || t == QLatin1String("ftptest") || t == QLatin1String("ftp"))
        return QStringLiteral("FTP Tests");
    return {};
}

// A sensible category for workouts without a (known) one, from intensity.
QString guessCategory(const Workout &w)
{
    const double ifactor = w.intensityFactor();
    if (ifactor < 0.62)
        return QStringLiteral("Recovery");
    if (ifactor < 0.80)
        return QStringLiteral("Endurance");
    return QStringLiteral("Intervals");
}

// Common final checks; returns false when the workout must be skipped.
bool finishWorkout(Workout &w, const QString &rawCategory, Cleaner &cleaner, QStringList *warnings)
{
    w.id = -1;
    w.builtIn = false;
    w.favorite = false;
    w.timesUsed = 0;
    w.name = limited(w.name, kMaxNameLength);
    if (w.name.isEmpty())
        w.name = defaultWorkoutName();
    w.focus = limited(w.focus, kMaxFocusLength);
    w.description = limited(w.description, kMaxDescriptionLength);

    if (w.intervals.isEmpty()) {
        warnings->append(QStringLiteral("\"%1\" has no intervals and was skipped.").arg(w.name));
        return false;
    }
    if (w.totalDurationSec() > kMaxWorkoutSec) {
        warnings->append(QStringLiteral("\"%1\" is longer than 12 hours and was skipped.").arg(w.name));
        return false;
    }
    const QString cat = matchCategory(rawCategory);
    if (!cat.isEmpty()) {
        w.category = cat;
    } else {
        w.category = guessCategory(w);
        if (!rawCategory.trimmed().isEmpty())
            cleaner.warn(QStringLiteral("cat"), QStringLiteral("unknown category \"%1\" was replaced by \"%2\"")
                                                    .arg(limited(rawCategory, 40), w.category));
    }
    return true;
}

// --- JSON ------------------------------------------------------------------
double jsonNumber(const QJsonValue &v, double def, bool *found = nullptr)
{
    bool ok = false;
    double d = def;
    if (v.isDouble()) {
        d = v.toDouble();
        ok = true;
    } else if (v.isString()) {
        d = v.toString().trimmed().toDouble(&ok);
        if (!ok)
            d = def;
    }
    if (found)
        *found = ok;
    return d;
}

Interval intervalFromJson(const QJsonObject &o, Cleaner &c)
{
    Interval i;
    const QString typeText = o.value(QStringLiteral("type")).toString();
    bool hasStart = false, hasEnd = false, hasDuration = false;
    const double rawStart = jsonNumber(o.value(QStringLiteral("start")), 0.6, &hasStart);
    const double rawEnd = jsonNumber(o.value(QStringLiteral("end")), rawStart, &hasEnd);
    const double rawDuration = jsonNumber(o.value(QStringLiteral("duration")), 300, &hasDuration);
    if (!hasDuration)
        c.warn(QStringLiteral("noDur"), QStringLiteral("intervals without a duration were set to 5 minutes"));
    if (!hasStart && !hasEnd)
        c.warn(QStringLiteral("noPow"), QStringLiteral("intervals without a power target were set to 60 % FTP"));

    i.durationSec = c.duration(rawDuration);
    i.startPercent = c.fraction(hasStart ? rawStart : (hasEnd ? rawEnd : 0.6));
    i.endPercent = c.fraction(hasEnd ? rawEnd : i.startPercent);
    if (!hasStart && hasEnd)
        i.startPercent = i.endPercent;
    if (typeText.trimmed().isEmpty())
        i.type = i.isRamp() ? BlockType::Ramp : BlockType::Steady;
    else
        i.type = blockTypeFromString(typeText);
    i.name = limited(o.value(QStringLiteral("name")).toString(), kMaxIntervalNameLength);
    if (i.name.isEmpty())
        i.name = blockTypeDisplayName(i.type);
    i.cadence = c.cadence(jsonNumber(o.value(QStringLiteral("cadence")), 0));
    i.rpeZone = c.rpe(jsonNumber(o.value(QStringLiteral("rpe")), -1));
    return i;
}

// --- XML helpers ---------------------------------------------------------------
using AttrMap = QHash<QString, QString>;

AttrMap lowerAttributes(const QXmlStreamReader &xml)
{
    AttrMap m;
    const auto attrs = xml.attributes();
    for (const QXmlStreamAttribute &a : attrs)
        m.insert(a.name().toString().toLower(), a.value().toString().trimmed());
    return m;
}

double attrNumber(const AttrMap &m, const char *key, double def, bool *found = nullptr)
{
    bool ok = false;
    const auto it = m.constFind(QLatin1String(key));
    double d = def;
    if (it != m.constEnd()) {
        d = it.value().toDouble(&ok);
        if (!ok)
            d = def;
    }
    if (found)
        *found = ok;
    return d;
}

bool hasAttr(const AttrMap &m, const char *key)
{
    bool found = false;
    attrNumber(m, key, 0, &found);
    return found;
}

BlockType steadyTypeFor(double fraction)
{
    if (fraction < 0.60)
        return BlockType::Recovery;
    if (fraction >= 0.88)
        return BlockType::Interval;
    return BlockType::Steady;
}

Interval makeInterval(BlockType type, const QString &name, int duration, double start, double end, int cadence)
{
    Interval i;
    i.type = type;
    i.name = name.isEmpty() ? blockTypeDisplayName(type) : name;
    i.durationSec = duration;
    i.startPercent = start;
    i.endPercent = end;
    i.cadence = cadence;
    return i;
}

} // namespace

// ---------------------------------------------------------------------------
QStringList allowedCategories()
{
    return { QStringLiteral("Endurance"), QStringLiteral("Intervals"), QStringLiteral("Recovery"),
             QStringLiteral("FTP Tests") };
}

Format formatForFileName(const QString &fileName)
{
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if (suffix == QLatin1String("zwo") || suffix == QLatin1String("xml"))
        return Format::Zwo;
    return Format::Awt;
}

QString safeFileName(const QString &name)
{
    QString s = name;
    s.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|\\x00-\\x1F]")), QStringLiteral(" "));
    s = s.simplified();
    while (s.startsWith(QLatin1Char('.')))
        s.remove(0, 1);
    while (s.endsWith(QLatin1Char('.')))
        s.chop(1);
    s = s.left(80).trimmed();
    return s.isEmpty() ? QStringLiteral("Workout") : s;
}

bool writeFile(const QString &path, const QByteArray &data, QString *error)
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    if (f.write(data) != data.size() || !f.commit()) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return true;
}

// --- export ------------------------------------------------------------------
QByteArray toAwt(const QVector<Workout> &workouts, const QString &appVersion, const QString &appName)
{
    QJsonArray list;
    for (const Workout &w : workouts) {
        QJsonObject o;
        o.insert(QStringLiteral("name"), w.name);
        o.insert(QStringLiteral("category"), w.category);
        o.insert(QStringLiteral("focus"), w.focus);
        o.insert(QStringLiteral("description"), w.description);
        o.insert(QStringLiteral("sport"), sport::normalize(w.sport));
        o.insert(QStringLiteral("intervals"), w.intervalsToJson());
        list.append(o);
    }
    QJsonObject root;
    root.insert(QStringLiteral("format"), QString::fromLatin1(kFormatId));
    root.insert(QStringLiteral("version"), kFormatVersion);
    root.insert(QStringLiteral("exportedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    QString app = appName.isEmpty() ? QStringLiteral("Artilody Watts Trainer") : appName;
    if (!appVersion.isEmpty())
        app += QLatin1Char(' ') + appVersion;
    root.insert(QStringLiteral("app"), app);
    root.insert(QStringLiteral("workouts"), list);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

QByteArray toZwo(const Workout &w, const QString &appName)
{
    QByteArray out;
    QXmlStreamWriter xml(&out);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(4);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("workout_file"));
    xml.writeTextElement(QStringLiteral("author"),
                        appName.isEmpty() ? QStringLiteral("Artilody Watts Trainer") : appName);
    xml.writeTextElement(QStringLiteral("name"), w.name);
    xml.writeTextElement(QStringLiteral("description"), w.description);
    // Zwift has no rowing sport type; see WorkoutIO.h.
    xml.writeTextElement(QStringLiteral("sportType"), QStringLiteral("bike"));
    xml.writeStartElement(QStringLiteral("tags"));
    xml.writeEmptyElement(QStringLiteral("tag"));
    xml.writeAttribute(QStringLiteral("name"), w.category);
    xml.writeEndElement();   // tags
    xml.writeStartElement(QStringLiteral("workout"));
    for (const Interval &i : w.intervals) {
        QString element;
        if (i.type == BlockType::WarmUp)
            element = QStringLiteral("Warmup");
        else if (i.type == BlockType::CoolDown)
            element = QStringLiteral("Cooldown");
        else if (i.isRamp())
            element = QStringLiteral("Ramp");
        else
            element = QStringLiteral("SteadyState");
        xml.writeStartElement(element);
        xml.writeAttribute(QStringLiteral("Duration"), QString::number(i.durationSec));
        if (element == QLatin1String("SteadyState")) {
            xml.writeAttribute(QStringLiteral("Power"), fractionText(i.startPercent));
        } else {
            // Zwift: PowerLow is the power at the start, PowerHigh at the end
            // (also for Cooldown, where PowerLow is the higher value).
            xml.writeAttribute(QStringLiteral("PowerLow"), fractionText(i.startPercent));
            xml.writeAttribute(QStringLiteral("PowerHigh"), fractionText(i.endPercent));
        }
        if (i.cadence > 0)
            xml.writeAttribute(QStringLiteral("Cadence"), QString::number(i.cadence));
        // The block name is shown as an on-screen message at the start of the block.
        if (!i.name.isEmpty()) {
            xml.writeEmptyElement(QStringLiteral("textevent"));
            xml.writeAttribute(QStringLiteral("timeoffset"), QStringLiteral("0"));
            xml.writeAttribute(QStringLiteral("message"), i.name);
        }
        xml.writeEndElement();
    }
    xml.writeEndElement();   // workout
    xml.writeEndElement();   // workout_file
    xml.writeEndDocument();
    return out;
}

// --- import: native JSON -------------------------------------------------------
ImportResult parseAwt(const QByteArray &rawData)
{
    ImportResult r;
    if (rawData.size() > kMaxFileBytes) {
        r.error = tooLargeError();
        return r;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(stripBom(rawData), &pe);
    if (pe.error != QJsonParseError::NoError) {
        r.error = QStringLiteral("This is not a valid workout file (JSON error at position %1: %2).")
                      .arg(pe.offset)
                      .arg(pe.errorString());
        return r;
    }

    QJsonArray list;
    if (doc.isObject()) {
        const QJsonObject root = doc.object();
        if (root.contains(QStringLiteral("format"))) {
            const QString fmt = root.value(QStringLiteral("format")).toString();
            if (fmt != QLatin1String(kFormatId)) {
                r.error = QStringLiteral("This is not an Artilody workout file (unknown format \"%1\").")
                              .arg(limited(fmt, 60));
                return r;
            }
        }
        const double version = jsonNumber(root.value(QStringLiteral("version")), kFormatVersion);
        if (version > kFormatVersion) {
            r.error = QStringLiteral("This file was made with a newer version of Artilody Watts Trainer "
                                     "(format version %1). Please update the app to import it.")
                          .arg(version);
            return r;
        }
        if (root.value(QStringLiteral("workouts")).isArray())
            list = root.value(QStringLiteral("workouts")).toArray();
        else if (root.contains(QStringLiteral("intervals")))
            list.append(root);   // a bare single workout
        else {
            r.error = QStringLiteral("The file does not contain any workouts.");
            return r;
        }
    } else if (doc.isArray()) {
        list = doc.array();
    } else {
        r.error = QStringLiteral("The file does not contain any workouts.");
        return r;
    }

    if (list.size() > kMaxWorkouts) {
        r.error = QStringLiteral("The file contains more than %1 workouts – it was not imported.").arg(kMaxWorkouts);
        return r;
    }

    Cleaner cleaner(&r.warnings);
    int index = 0;
    for (const QJsonValue &v : list) {
        ++index;
        if (!v.isObject()) {
            r.warnings.append(QStringLiteral("Entry %1 is not a workout and was skipped.").arg(index));
            continue;
        }
        const QJsonObject o = v.toObject();
        Workout w;
        w.name = o.value(QStringLiteral("name")).toString();
        w.focus = o.value(QStringLiteral("focus")).toString();
        w.description = o.value(QStringLiteral("description")).toString();
        w.sport = sport::normalize(o.value(QStringLiteral("sport")).toString());
        const QString label = limited(w.name, kMaxNameLength).isEmpty() ? defaultWorkoutName()
                                                                          : limited(w.name, kMaxNameLength);
        cleaner.begin(list.size() > 1 ? label : QString());
        const QJsonArray intervals = o.value(QStringLiteral("intervals")).toArray();
        if (intervals.size() > kMaxIntervals) {
            r.workouts.clear();
            r.error = tooManyIntervalsError(label);
            return r;
        }
        for (const QJsonValue &iv : intervals) {
            if (iv.isObject())
                w.intervals.append(intervalFromJson(iv.toObject(), cleaner));
            else
                cleaner.warn(QStringLiteral("badInterval"), QStringLiteral("invalid interval entries were skipped"));
        }
        if (finishWorkout(w, o.value(QStringLiteral("category")).toString(), cleaner, &r.warnings))
            r.workouts.append(w);
    }
    if (r.workouts.isEmpty())
        r.error = QStringLiteral("The file contains no usable workouts.");
    return r;
}

// --- import: Zwift .zwo -----------------------------------------------------------
ImportResult parseZwo(const QByteArray &data)
{
    ImportResult r;
    if (data.size() > kMaxFileBytes) {
        r.error = tooLargeError();
        return r;
    }

    QXmlStreamReader xml(data);
    Cleaner cleaner(&r.warnings);
    cleaner.begin(QString());
    Workout w;
    QString rawCategory;
    QString sport;
    QStringList unknown;
    bool sawWorkout = false;
    bool inWorkout = false;
    bool freeRide = false;
    bool maxEffort = false;
    int blockDepth = 0;        // 0 = directly inside <workout>
    int blockFirst = -1;       // index of the first interval produced by the current block
    int blockCount = 0;        // number of intervals produced by the current block
    bool blockNamed = false;

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType tok = xml.readNext();
        if (tok == QXmlStreamReader::EndElement) {
            if (inWorkout) {
                if (blockDepth > 0)
                    --blockDepth;
                else
                    inWorkout = false;
            }
            continue;
        }
        if (tok != QXmlStreamReader::StartElement)
            continue;

        const QString name = xml.name().toString().toLower();
        if (!inWorkout) {
            if (name == QLatin1String("name")) {
                w.name = xml.readElementText(QXmlStreamReader::IncludeChildElements);
            } else if (name == QLatin1String("description")) {
                w.description = xml.readElementText(QXmlStreamReader::IncludeChildElements);
            } else if (name == QLatin1String("sporttype")) {
                sport = xml.readElementText(QXmlStreamReader::IncludeChildElements).trimmed();
            } else if (name == QLatin1String("category")) {
                const QString c = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                if (rawCategory.isEmpty() || matchCategory(rawCategory).isEmpty())
                    rawCategory = c;
            } else if (name == QLatin1String("tag")) {
                const QString c = lowerAttributes(xml).value(QStringLiteral("name"));
                if (!matchCategory(c).isEmpty() && matchCategory(rawCategory).isEmpty())
                    rawCategory = c;
            } else if (name == QLatin1String("workout")) {
                sawWorkout = true;
                inWorkout = true;
                blockDepth = 0;
            }
            continue;
        }

        const AttrMap a = lowerAttributes(xml);
        if (blockDepth > 0) {
            // child of a block: a text event at the block start names the block
            if (blockDepth == 1 && name == QLatin1String("textevent") && blockCount == 1 && !blockNamed) {
                const QString msg = a.value(QStringLiteral("message")).simplified();
                const double offset = attrNumber(a, "timeoffset", 0);
                if (!msg.isEmpty() && msg.size() <= 40 && offset <= 0.5) {
                    w.intervals[blockFirst].name = msg;
                    blockNamed = true;
                }
            }
            ++blockDepth;
            continue;
        }

        blockDepth = 1;
        blockFirst = w.intervals.size();
        blockNamed = false;
        const int cadence = cleaner.cadence(attrNumber(a, "cadence", 0));
        bool hasDuration = false;
        const double rawDuration = attrNumber(a, "duration", 0, &hasDuration);

        if (name == QLatin1String("warmup") || name == QLatin1String("cooldown") || name == QLatin1String("ramp")) {
            if (!hasDuration) {
                cleaner.warn(QStringLiteral("noDur"), QStringLiteral("blocks without a duration were skipped"));
            } else {
                double low = attrNumber(a, "powerlow", -1);
                double high = attrNumber(a, "powerhigh", -1);
                const double power = attrNumber(a, "power", 0.6);
                if (low < 0 && high < 0)
                    low = high = power;
                else if (low < 0)
                    low = high;
                else if (high < 0)
                    high = low;
                const BlockType type = name == QLatin1String("warmup") ? BlockType::WarmUp
                                     : name == QLatin1String("cooldown") ? BlockType::CoolDown
                                                                          : BlockType::Ramp;
                w.intervals.append(makeInterval(type, QString(), cleaner.duration(rawDuration),
                                                cleaner.fraction(low), cleaner.fraction(high), cadence));
            }
        } else if (name == QLatin1String("steadystate")) {
            if (!hasDuration) {
                cleaner.warn(QStringLiteral("noDur"), QStringLiteral("blocks without a duration were skipped"));
            } else {
                double power = 0.6;
                if (hasAttr(a, "power"))
                    power = attrNumber(a, "power", 0.6);
                else if (hasAttr(a, "powerlow") || hasAttr(a, "powerhigh")) {
                    const double low = attrNumber(a, "powerlow", attrNumber(a, "powerhigh", 0.6));
                    const double high = attrNumber(a, "powerhigh", low);
                    power = (low + high) / 2.0;
                }
                const double f = cleaner.fraction(power);
                w.intervals.append(makeInterval(steadyTypeFor(f), QString(), cleaner.duration(rawDuration), f, f, cadence));
            }
        } else if (name == QLatin1String("intervalst")) {
            const double rawRepeat = attrNumber(a, "repeat", 1);
            int repeat = int(std::lround(std::isfinite(rawRepeat) ? rawRepeat : 1.0));
            if (repeat < 1 || repeat > kMaxRepeat) {
                cleaner.warn(QStringLiteral("repeat"), QStringLiteral("interval repeat counts were limited to 1–%1").arg(kMaxRepeat));
                repeat = std::clamp(repeat, 1, kMaxRepeat);
            }
            const int onSec = cleaner.duration(attrNumber(a, "onduration", 60));
            const int offSec = cleaner.duration(attrNumber(a, "offduration", 60));
            const double onLow = attrNumber(a, "onpower", attrNumber(a, "poweronlow", attrNumber(a, "poweronhigh", 1.0)));
            const double onHigh = hasAttr(a, "onpower") ? onLow : attrNumber(a, "poweronhigh", onLow);
            const double offLow = attrNumber(a, "offpower", attrNumber(a, "powerofflow", attrNumber(a, "poweroffhigh", 0.5)));
            const double offHigh = hasAttr(a, "offpower") ? offLow : attrNumber(a, "poweroffhigh", offLow);
            const int restCadence = cleaner.cadence(attrNumber(a, "cadenceresting", 0));
            const Interval on = makeInterval(BlockType::Interval, QString(), onSec, cleaner.fraction(onLow),
                                             cleaner.fraction(onHigh), cadence);
            const Interval off = makeInterval(BlockType::Recovery, QString(), offSec, cleaner.fraction(offLow),
                                              cleaner.fraction(offHigh), restCadence);
            if (w.intervals.size() + 2 * repeat > kMaxIntervals) {
                r.error = tooManyIntervalsError(w.name.trimmed().isEmpty() ? defaultWorkoutName() : w.name.trimmed());
                return r;
            }
            for (int n = 0; n < repeat; ++n) {
                w.intervals.append(on);
                w.intervals.append(off);
            }
        } else if (name == QLatin1String("freeride")) {
            freeRide = true;
            w.intervals.append(makeInterval(BlockType::Steady, QStringLiteral("Free Ride"),
                                            cleaner.duration(hasDuration ? rawDuration : 600), kFreeRideFraction,
                                            kFreeRideFraction, cadence));
        } else if (name == QLatin1String("maxeffort")) {
            maxEffort = true;
            w.intervals.append(makeInterval(BlockType::Interval, QStringLiteral("Max Effort"),
                                            cleaner.duration(hasDuration ? rawDuration : 30), kMaxEffortFraction,
                                            kMaxEffortFraction, cadence));
        } else if (name == QLatin1String("textevent") || name == QLatin1String("textnotification")) {
            // coaching messages outside a block – ignored
        } else {
            const QString original = xml.name().toString();
            if (!unknown.contains(original))
                unknown.append(original);
        }
        blockCount = w.intervals.size() - blockFirst;
        if (w.intervals.size() > kMaxIntervals) {
            r.error = tooManyIntervalsError(w.name.trimmed().isEmpty() ? defaultWorkoutName() : w.name.trimmed());
            return r;
        }
    }

    if (xml.hasError()) {
        r.error = QStringLiteral("This is not a valid Zwift workout file (XML error on line %1: %2).")
                      .arg(xml.lineNumber())
                      .arg(xml.errorString());
        return r;
    }
    if (!sawWorkout) {
        r.error = QStringLiteral("No <workout> section found – this does not look like a Zwift workout (.zwo) file.");
        return r;
    }
    if (!sport.isEmpty() && sport.compare(QLatin1String("bike"), Qt::CaseInsensitive) != 0)
        r.warnings.append(QStringLiteral("This workout was made for \"%1\"; it was imported as a cycling workout.").arg(limited(sport, 30)));
    if (freeRide)
        r.warnings.append(QStringLiteral("Free ride sections were converted to steady blocks at 60 % FTP."));
    if (maxEffort)
        r.warnings.append(QStringLiteral("Max effort sections were converted to blocks at 150 % FTP."));
    if (!unknown.isEmpty())
        r.warnings.append(QStringLiteral("Ignored unsupported elements: %1.").arg(unknown.join(QStringLiteral(", "))));

    w.sport = sport::cycling();
    if (finishWorkout(w, rawCategory, cleaner, &r.warnings))
        r.workouts.append(w);
    else
        r.error = QStringLiteral("The Zwift workout contains no usable intervals.");
    return r;
}

// --- detection -------------------------------------------------------------------
ImportResult parseData(const QByteArray &data, const QString &fileName)
{
    if (data.size() > kMaxFileBytes) {
        ImportResult r;
        r.error = tooLargeError();
        return r;
    }
    const QByteArray body = stripBom(data);
    for (char ch : body) {
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
            continue;
        if (ch == '{' || ch == '[')
            return parseAwt(body);
        if (ch == '<')
            return parseZwo(body);
        break;
    }
    if (body.trimmed().isEmpty()) {
        ImportResult r;
        r.error = QStringLiteral("The file is empty.");
        return r;
    }
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if (suffix == QLatin1String("zwo") || suffix == QLatin1String("xml"))
        return parseZwo(body);
    if (suffix == QLatin1String("awt") || suffix == QLatin1String("json"))
        return parseAwt(body);
    ImportResult r;
    r.error = QStringLiteral("Unrecognised file format. Supported are Artilody workouts (.awt) and Zwift workouts (.zwo).");
    return r;
}

ImportResult parseFile(const QString &path)
{
    ImportResult r;
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        r.error = QStringLiteral("File not found.");
        return r;
    }
    if (info.size() > kMaxFileBytes) {
        r.error = tooLargeError();
        return r;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        r.error = f.errorString();
        return r;
    }
    const QByteArray data = f.read(kMaxFileBytes + 1);
    return parseData(data, path);
}

} // namespace watt::workoutio
