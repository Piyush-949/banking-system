#ifndef BANKING_SYSTEM_COMMON_MONEY_HPP
#define BANKING_SYSTEM_COMMON_MONEY_HPP

#include <cstdint>
#include <string>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <limits>

namespace BankingSystem
{

    class Money
    {
    private:
        int64_t m_paise{0}; // Internal storage in Paise (1 INR = 100 Paise)

    public:
        // Constructors
        constexpr Money() noexcept : m_paise(0) {}
        constexpr explicit Money(int64_t paise) noexcept : m_paise(paise) {}
        // Transitional boundary adapter for the legacy CLI. Domain storage remains paise.
        explicit Money(double rupees) : m_paise(fromRupees(rupees).m_paise) {}
        Money &operator=(double rupees)
        {
            m_paise = fromRupees(rupees).m_paise;
            return *this;
        }

        // Factory static methods
        static Money fromPaise(int64_t paise) noexcept { return Money(paise); }
        static Money fromRupees(double rupees);
        static Money parse(const std::string &str);

        // Getters & Converters
        constexpr int64_t getPaise() const noexcept { return m_paise; }
        double toRupees() const noexcept { return static_cast<double>(m_paise) / 100.0; }
        operator double() const noexcept { return toRupees(); }

        // Indian Currency Formatting (₹XX,XX,XXX.YY)
        std::string format() const;
        std::string formatPlain() const; // Plain "XXXXX.YY" without currency symbol or commas

        // Arithmetic Operators
        Money operator+(const Money &other) const;
        Money operator-(const Money &other) const;
        Money operator*(double factor) const;
        Money operator/(double divisor) const;
        int64_t operator/(const Money &other) const
        {
            if (other.m_paise == 0)
                throw std::invalid_argument("Money division by zero Money");
            return m_paise / other.m_paise;
        }
        Money operator%(int64_t divisor) const
        {
            if (divisor == 0)
                throw std::invalid_argument("Money modulo by zero");
            return Money(m_paise % divisor);
        }
        Money operator-() const;

        // Compound Assignment Operators
        Money &operator+=(const Money &other) { return *this = *this + other; }
        Money &operator-=(const Money &other) { return *this = *this - other; }
        Money &operator*=(double factor) { return *this = *this * factor; }
        Money &operator/=(double divisor) { return *this = *this / divisor; }
        Money &operator+=(double rupees) { return *this += Money(rupees); }
        Money &operator-=(double rupees) { return *this -= Money(rupees); }

        // Comparison Operators
        constexpr bool operator==(const Money &other) const noexcept { return m_paise == other.m_paise; }
        constexpr bool operator!=(const Money &other) const noexcept { return m_paise != other.m_paise; }
        constexpr bool operator<(const Money &other) const noexcept { return m_paise < other.m_paise; }
        constexpr bool operator<=(const Money &other) const noexcept { return m_paise <= other.m_paise; }
        constexpr bool operator>(const Money &other) const noexcept { return m_paise > other.m_paise; }
        constexpr bool operator>=(const Money &other) const noexcept { return m_paise >= other.m_paise; }

        // Predicates
        constexpr bool isZero() const noexcept { return m_paise == 0; }
        constexpr bool isPositive() const noexcept { return m_paise > 0; }
        constexpr bool isNegative() const noexcept { return m_paise < 0; }

        // Stream Operators
        friend std::ostream &operator<<(std::ostream &os, const Money &m)
        {
            return os << m.format();
        }
    };

} // namespace BankingSystem

#endif // BANKING_SYSTEM_COMMON_MONEY_HPP
