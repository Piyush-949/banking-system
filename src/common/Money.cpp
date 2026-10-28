#include "common/Money.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace BankingSystem
{

    namespace
    {
        int64_t checkedFromLongDouble(long double value)
        {
            if (!std::isfinite(value) || value > static_cast<long double>(std::numeric_limits<int64_t>::max()) ||
                value < static_cast<long double>(std::numeric_limits<int64_t>::min()))
            {
                throw std::overflow_error("Money amount is outside the supported paise range");
            }
            return static_cast<int64_t>(std::round(value));
        }

        uint64_t magnitude(int64_t value) noexcept
        {
            // Avoid undefined behaviour for INT64_MIN.
            return value < 0 ? static_cast<uint64_t>(-(value + 1)) + 1U : static_cast<uint64_t>(value);
        }
    } // namespace

    Money Money::fromRupees(double rupees)
    {
        return Money(checkedFromLongDouble(static_cast<long double>(rupees) * 100.0L));
    }

    std::string Money::format() const
    {
        const uint64_t absPaise = magnitude(m_paise);
        const uint64_t rupees = absPaise / 100U;
        const uint64_t paise = absPaise % 100U;

        std::string paiseStr = (paise < 10 ? "0" : "") + std::to_string(paise);
        std::string rupeesStr = std::to_string(rupees);
        std::string formattedRupees;

        if (rupeesStr.length() <= 3)
        {
            formattedRupees = rupeesStr;
        }
        else
        {
            std::string last3 = rupeesStr.substr(rupeesStr.length() - 3);
            std::string rem = rupeesStr.substr(0, rupeesStr.length() - 3);

            std::string remFormatted = "";
            int len = static_cast<int>(rem.length());
            int firstGroup = len % 2;
            if (firstGroup > 0)
            {
                remFormatted += rem.substr(0, firstGroup);
            }
            for (int i = firstGroup; i < len; i += 2)
            {
                if (!remFormatted.empty())
                {
                    remFormatted += ",";
                }
                remFormatted += rem.substr(i, 2);
            }
            formattedRupees = remFormatted + "," + last3;
        }

        std::string result = "₹" + formattedRupees + "." + paiseStr;
        if (m_paise < 0)
        {
            result = "-" + result;
        }
        return result;
    }

    std::string Money::formatPlain() const
    {
        const uint64_t absPaise = magnitude(m_paise);
        const uint64_t rupees = absPaise / 100U;
        const uint64_t paise = absPaise % 100U;

        std::string paiseStr = (paise < 10 ? "0" : "") + std::to_string(paise);
        std::string result = std::to_string(rupees) + "." + paiseStr;
        if (m_paise < 0)
        {
            result = "-" + result;
        }
        return result;
    }

    Money Money::parse(const std::string &str)
    {
        const size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos)
            return Money{};
        std::string s = str.substr(start, str.find_last_not_of(" \t\n\r") - start + 1);
        bool negative = false;
        if (s.front() == '-' || s.front() == '+')
        {
            negative = s.front() == '-';
            s.erase(0, 1);
        }
        if (s.rfind("₹", 0) == 0)
            s.erase(0, std::string("₹").size());
        else if (s.rfind("â‚¹", 0) == 0)
            s.erase(0, std::string("â‚¹").size());
        const size_t innerStart = s.find_first_not_of(" \t\n\r");
        if (innerStart == std::string::npos)
            return Money{};
        const size_t innerEnd = s.find_last_not_of(" \t\n\r");
        s = s.substr(innerStart, innerEnd - innerStart + 1);
        if (s.empty())
            return Money{};

        std::string whole;
        std::string fraction;
        bool seenDot = false;
        for (char c : s)
        {
            if (c == ',')
                continue;
            if (c == '.')
            {
                if (seenDot)
                    return Money{};
                seenDot = true;
                continue;
            }
            if (!std::isdigit(static_cast<unsigned char>(c)))
                return Money{};
            (seenDot ? fraction : whole).push_back(c);
        }
        if ((whole.empty() && fraction.empty()) || fraction.size() > 2)
            return Money{};
        if (fraction.size() == 1)
            fraction.push_back('0');
        while (fraction.size() < 2)
            fraction.push_back('0');
        try
        {
            const uint64_t rupees = whole.empty() ? 0ULL : std::stoull(whole);
            const uint64_t paise = static_cast<uint64_t>(std::stoul(fraction));
            const uint64_t limit = negative ? (static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1U)
                                            : static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
            if (rupees > (limit - paise) / 100U)
                return Money{};
            const uint64_t total = rupees * 100U + paise;
            if (negative && total == limit)
                return Money(std::numeric_limits<int64_t>::min());
            return Money(negative ? -static_cast<int64_t>(total) : static_cast<int64_t>(total));
        }
        catch (const std::out_of_range &)
        {
            throw;
        }
        catch (const std::exception &)
        {
            return Money{};
        }
    }

    Money Money::operator+(const Money &other) const
    {
        if ((other.m_paise > 0 && m_paise > std::numeric_limits<int64_t>::max() - other.m_paise) ||
            (other.m_paise < 0 && m_paise < std::numeric_limits<int64_t>::min() - other.m_paise))
        {
            throw std::overflow_error("Money addition overflow");
        }
        return Money(m_paise + other.m_paise);
    }

    Money Money::operator-(const Money &other) const
    {
        if (other.m_paise == std::numeric_limits<int64_t>::min())
        {
            return *this + Money::fromPaise(std::numeric_limits<int64_t>::max()) + Money::fromPaise(1);
        }
        return *this + Money::fromPaise(-other.m_paise);
    }

    Money Money::operator*(double factor) const
    {
        return Money(checkedFromLongDouble(static_cast<long double>(m_paise) * static_cast<long double>(factor)));
    }

    Money Money::operator/(double divisor) const
    {
        if (!std::isfinite(divisor) || divisor == 0.0)
            throw std::invalid_argument("Money division by zero or non-finite divisor");
        return Money(checkedFromLongDouble(static_cast<long double>(m_paise) / static_cast<long double>(divisor)));
    }

    Money Money::operator-() const
    {
        if (m_paise == std::numeric_limits<int64_t>::min())
            throw std::overflow_error("Money negation overflow");
        return Money(-m_paise);
    }

} // namespace BankingSystem
