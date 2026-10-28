#ifndef BANKING_SYSTEM_COMMON_DATE_UTILS_HPP
#define BANKING_SYSTEM_COMMON_DATE_UTILS_HPP

#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace BankingSystem {

class DateUtils {
public:
    using TimePoint = std::chrono::system_clock::time_point;

    // 1. Current Time Strings
    static std::string nowISO8601();  // e.g. "2026-08-09 16:37:10"
    static std::string currentDate(); // e.g. "2026-08-09"

    // 2. Conversion between TimePoint and String
    static std::string timePointToISO8601(TimePoint tp);
    static std::string timePointToDate(TimePoint tp);
    static TimePoint parseISO8601(const std::string& isoStr);
    static TimePoint parseDate(const std::string& dateStr); // "YYYY-MM-DD"

    // 3. Elapsed Time & Date Calculations
    static int64_t calculateElapsedDays(TimePoint start, TimePoint end);
    static int64_t calculateElapsedDays(const std::string& startIso, const std::string& endIso);
    
    static TimePoint addDays(TimePoint tp, int days);
    static std::string addDays(const std::string& isoOrDateStr, int days);

    static TimePoint addMonths(TimePoint tp, int months);
    static std::string addMonths(const std::string& isoOrDateStr, int months);

    static TimePoint addYears(TimePoint tp, int years);
    static std::string addYears(const std::string& isoOrDateStr, int years);

    // 4. Utility / Formatting Helpers
    static std::string formatTimePoint(TimePoint tp, const std::string& formatStr);
    static bool isValidISO8601(const std::string& str);
};

} // namespace BankingSystem

#endif // BANKING_SYSTEM_COMMON_DATE_UTILS_HPP
