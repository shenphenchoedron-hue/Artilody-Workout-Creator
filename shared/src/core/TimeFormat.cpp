#include "TimeFormat.h"

#include <QStringList>

namespace watt {

QString formatClock(int seconds)
{
    if (seconds < 0)
        seconds = 0;
    const int h = seconds / 3600;
    const int m = (seconds % 3600) / 60;
    const int s = seconds % 60;
    if (h > 0)
        return QStringLiteral("%1:%2:%3").arg(h).arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(m).arg(s, 2, 10, QLatin1Char('0'));
}

QString formatHoursMinutes(int seconds, bool forceHours)
{
    if (seconds < 0)
        seconds = 0;
    const int h = seconds / 3600;
    const int m = (seconds % 3600) / 60;
    if (h > 0 || forceHours)
        return QStringLiteral("%1h %2m").arg(h).arg(m, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1m").arg(m);
}

int parseClock(const QString &text)
{
    const QStringList parts = text.trimmed().split(QLatin1Char(':'));
    if (parts.isEmpty() || parts.size() > 3)
        return -1;
    int total = 0;
    for (const QString &p : parts) {
        bool ok = false;
        const int v = p.trimmed().toInt(&ok);
        if (!ok || v < 0)
            return -1;
        total = total * 60 + v;
    }
    return total;
}

} // namespace watt
