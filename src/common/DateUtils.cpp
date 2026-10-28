#include "common/DateUtils.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace BankingSystem {

namespace {
    std::tm getLocalTime(const std::time_t& timer) {
        std::tm bt{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&bt, &timer);
#else
        localtime_r(&timer, &bt);
#endif
        return bt;
    }

    int daysInMonth(int year, int month) {
        // month is 1..12
        static const int days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        if (month == 2) {
            bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            return isLeap ? 29 : 28;
        }
        if (month >= 1 && month <= 12) {
            return days[month];
        }
        return 30;
    }
}

std::string DateUtils::nowISO8601() {
    return timePointToISO8601(std::chrono::system_clock::now());
}

std::string DateUtils::currentDate() {
    return timePointToDate(std::chrono::system_clock::now());
}

std::string DateUtils::timePointToISO8601(TimePoint tp) {
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = getLocalTime(tt);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

std::string DateUtils::timePointToDate(TimePoint tp) {
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = getLocalTime(tt);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return std::string(buf);
}

DateUtils::TimePoint DateUtils::parseISO8601(const std::string& isoStr) {
    if (isoStr.empty()) return std::chrono::system_clock::now();

    int year = 1970, month = 1, day = 1, hour = 0, min = 0, sec = 0;
    // Handle formats: "YYYY-MM-DD HH:MM:SS", "YYYY-MM-DDTHH:MM:SSZ", "YYYY-MM-DD"
    if (std::sscanf(isoStr.c_str(), "%d-%d-%d%*c%d:%d:%d", &year, &month, &day, &hour, &min, &sec) >= 3 ||
        std::sscanf(isoStr.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
        std::tm tm{};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;
        tm.tm_isdst = -1;
        std::time_t tt = std::mktime(&tm);
        if (tt != -1) {
            return std::chrono::system_clock::from_time_t(tt);
        }
    }
    return std::chrono::system_clock::now();
}

DateUtils::TimePoint DateUtils::parseDate(const std::string& dateStr) {
    return parseISO8601(dateStr);
}

int64_t DateUtils::calculateElapsedDays(TimePoint start, TimePoint end) {
    auto hours = std::chrono::duration_cast<std::chrono::hours>(end - start).count();
    return hours / 24;
}

int64_t DateUtils::calculateElapsedDays(const std::string& startIso, const std::string& endIso) {
    return calculateElapsedDays(parseISO8601(startIso), parseISO8601(endIso));
}

DateUtils::TimePoint DateUtils::addDays(TimePoint tp, int days) {
    return tp + std::chrono::hours(24 * static_cast<int64_t>(days));
}

std::string DateUtils::addDays(const std::string& isoOrDateStr, int days) {
    TimePoint tp = parseISO8601(isoOrDateStr);
    TimePoint resultTp = addDays(tp, days);
    if (isoOrDateStr.length() <= 10) {
        return timePointToDate(resultTp);
    }
    return timePointToISO8601(resultTp);
}

DateUtils::TimePoint DateUtils::addMonths(TimePoint tp, int months) {
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = getLocalTime(tt);

    int totalMonths = (tm.tm_year + 1900) * 12 + tm.tm_mon + months;
    int newYear = totalMonths / 12;
    int newMon = totalMonths % 12;
    if (newMon < 0) {
        newMon += 12;
        newYear -= 1;
    }

    int maxDays = daysInMonth(newYear, newMon + 1);
    int newDay = std::min(tm.tm_mday, maxDays);

    tm.tm_year = newYear - 1900;
    tm.tm_mon = newMon;
    tm.tm_mday = newDay;
    tm.tm_isdst = -1;

    std::time_t newTt = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(newTt);
}

std::string DateUtils::addMonths(const std::string& isoOrDateStr, int months) {
    TimePoint tp = parseISO8601(isoOrDateStr);
    TimePoint resultTp = addMonths(tp, months);
    if (isoOrDateStr.length() <= 10) {
        return timePointToDate(resultTp);
    }
    return timePointToISO8601(resultTp);
}

DateUtils::TimePoint DateUtils::addYears(TimePoint tp, int years) {
    return addMonths(tp, years * 12);
}

std::string DateUtils::addYears(const std::string& isoOrDateStr, int years) {
    return addMonths(isoOrDateStr, years * 12);
}

std::string DateUtils::formatTimePoint(TimePoint tp, const std::string& formatStr) {
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = getLocalTime(tt);
    char buf[128];
    if (std::strftime(buf, sizeof(buf), formatStr.c_str(), &tm)) {
        return std::string(buf);
    }
    return timePointToISO8601(tp);
}

bool DateUtils::isValidISO8601(const std::string& str) {
    if (str.empty()) return false;
    int year = 0, month = 0, day = 0;
    if (std::sscanf(str.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
        if (year >= 1900 && year <= 2100 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
            return true;
        }
    }
    return false;
}

} // namespace BankingSystem
