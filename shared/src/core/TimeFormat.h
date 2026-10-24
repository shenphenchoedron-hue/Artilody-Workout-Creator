#pragma once

#include <QString>

namespace watt {

// "4:05", "24:18" or "1:02:03"
QString formatClock(int seconds);
// "21h 35m", "45m", "0h 20m" (when forceHours)
QString formatHoursMinutes(int seconds, bool forceHours = false);
// Parses "12:00", "1:02:03" or "90" (seconds). Returns -1 on error.
int parseClock(const QString &text);

} // namespace watt
