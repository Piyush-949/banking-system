#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <cmath>
#include "include/common/Money.hpp"

namespace BankingSystem
{

    namespace Persistence
    {

        inline std::vector<std::string> split(const std::string &line)
        {
            std::vector<std::string> fields;
            std::string field;
            bool escaped = false;
            for (char c : line)
            {
                if (escaped)
                {
                    switch (c)
                    {
                    case 'p':
                        field.push_back('|');
                        break;
                    case 'n':
                        field.push_back('\n');
                        break;
                    case 'r':
                        field.push_back('\r');
                        break;
                    default:
                        field.push_back(c);
                        break;
                    }
                    escaped = false;
                }
                else if (c == '\\')
                {
                    escaped = true;
                }
                else if (c == '|')
                {
                    fields.push_back(field);
                    field.clear();
                }
                else
                {
                    field.push_back(c);
                }
            }
            if (escaped)
                return {};
            fields.push_back(field); // Preserve trailing empty fields.
            return fields;
        }

        inline std::string escape(const std::string &value)
        {
            std::string result;
            result.reserve(value.size());
            for (char c : value)
            {
                switch (c)
                {
                case '\\':
                    result += "\\\\";
                    break;
                case '|':
                    result += "\\p";
                    break;
                case '\n':
                    result += "\\n";
                    break;
                case '\r':
                    result += "\\r";
                    break;
                default:
                    result.push_back(c);
                    break;
                }
            }
            return result;
        }

        inline bool parseInt(const std::string &text, int &value) noexcept
        {
            char *end = nullptr;
            errno = 0;
            const long parsed = std::strtol(text.c_str(), &end, 10);
            if (errno == ERANGE || end == text.c_str() || *end != '\0' ||
                parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max())
                return false;
            value = static_cast<int>(parsed);
            return true;
        }

        inline bool parseDouble(const std::string &text, double &value) noexcept
        {
            char *end = nullptr;
            errno = 0;
            const double parsed = std::strtod(text.c_str(), &end);
            if (errno == ERANGE || end == text.c_str() || *end != '\0' || !std::isfinite(parsed))
                return false;
            value = parsed;
            return true;
        }

        inline bool parseBool(const std::string &text, bool &value) noexcept
        {
            if (text == "1")
            {
                value = true;
                return true;
            }
            if (text == "0")
            {
                value = false;
                return true;
            }
            return false;
        }

    } // namespace Persistence

    // ==========================================
    // ENUMS
    // ==========================================

    enum class Role
    {
        RBI_GOVERNOR = 0,
        BANK_CMD = 1,
        ZONAL_MANAGER = 2,
        BRANCH_MANAGER = 3,
        LOAN_OFFICER = 4,
        RELATIONSHIP_MANAGER = 5,
        TELLER = 6,
        CUSTOMER = 7,
        PUBLIC_GUEST = 8
    };

    inline std::string roleToString(Role role)
    {
        switch (role)
        {
        case Role::RBI_GOVERNOR:
            return "RBI Governor";
        case Role::BANK_CMD:
            return "Bank CMD / CEO";
        case Role::ZONAL_MANAGER:
            return "Zonal Manager";
        case Role::BRANCH_MANAGER:
            return "Branch Manager";
        case Role::LOAN_OFFICER:
            return "Loan Officer";
        case Role::RELATIONSHIP_MANAGER:
            return "Relationship Manager";
        case Role::TELLER:
            return "Teller / Cashier";
        case Role::CUSTOMER:
            return "Customer";
        default:
            return "Guest / Public";
        }
    }

    enum class AccountType
    {
        SAVINGS = 0,
        CURRENT = 1,
        NRI = 2,
        JOINT = 3
    };

    inline std::string accountTypeToString(AccountType t)
    {
        switch (t)
        {
        case AccountType::SAVINGS:
            return "Savings Account";
        case AccountType::CURRENT:
            return "Current Account (Overdraft)";
        case AccountType::NRI:
            return "NRI Account (NRE/NRO)";
        case AccountType::JOINT:
            return "Joint Savings Account";
        default:
            return "Unknown";
        }
    }

    enum class LoanType
    {
        HOME = 0,
        PERSONAL = 1,
        CAR = 2,
        EDUCATION = 3,
        BUSINESS = 4,
        GOLD = 5,
        AGRI = 6
    };

    inline std::string loanTypeToString(LoanType t)
    {
        switch (t)
        {
        case LoanType::HOME:
            return "Home Loan";
        case LoanType::PERSONAL:
            return "Personal Loan";
        case LoanType::CAR:
            return "Car Loan";
        case LoanType::EDUCATION:
            return "Education Loan";
        case LoanType::BUSINESS:
            return "Business Loan";
        case LoanType::GOLD:
            return "Gold Loan";
        case LoanType::AGRI:
            return "Agriculture Loan";
        default:
            return "General Loan";
        }
    }

    enum class LoanStatus
    {
        APPLIED = 0,
        UNDER_LOAN_OFFICER = 1,
        UNDER_BRANCH_MANAGER = 2,
        UNDER_ZONAL_MANAGER = 3,
        UNDER_CMD = 4,
        APPROVED = 5,
        DISBURSED = 6,
        REJECTED = 7,
        CLOSED = 8,
        NPA_DEFAULT = 9
    };

    inline std::string loanStatusToString(LoanStatus s)
    {
        switch (s)
        {
        case LoanStatus::APPLIED:
            return "Submitted";
        case LoanStatus::UNDER_LOAN_OFFICER:
            return "Under Review (Loan Officer)";
        case LoanStatus::UNDER_BRANCH_MANAGER:
            return "Pending BM Approval";
        case LoanStatus::UNDER_ZONAL_MANAGER:
            return "Pending Zonal Manager Approval";
        case LoanStatus::UNDER_CMD:
            return "Pending CMD Approval";
        case LoanStatus::APPROVED:
            return "Approved (Pending Disbursal)";
        case LoanStatus::DISBURSED:
            return "Active / Disbursed";
        case LoanStatus::REJECTED:
            return "Rejected";
        case LoanStatus::CLOSED:
            return "Fully Paid & Closed";
        case LoanStatus::NPA_DEFAULT:
            return "Non-Performing Asset (NPA)";
        default:
            return "Unknown";
        }
    }

    enum class InsuranceType
    {
        LIFE_TERM = 0,
        LIFE_ENDOWMENT = 1,
        HEALTH_MEDICLAIM = 2,
        HEALTH_CRITICAL = 3,
        VEHICLE_COMPREHENSIVE = 4,
        HOME_LOAN_PROTECT = 5
    };

    inline std::string insuranceTypeToString(InsuranceType t)
    {
        switch (t)
        {
        case InsuranceType::LIFE_TERM:
            return "Life Term Plan";
        case InsuranceType::LIFE_ENDOWMENT:
            return "Life Endowment Plan";
        case InsuranceType::HEALTH_MEDICLAIM:
            return "Health Mediclaim";
        case InsuranceType::HEALTH_CRITICAL:
            return "Critical Illness Health";
        case InsuranceType::VEHICLE_COMPREHENSIVE:
            return "Vehicle Comprehensive";
        case InsuranceType::HOME_LOAN_PROTECT:
            return "Home Loan Protector";
        default:
            return "General Insurance";
        }
    }

    enum class CreditCardType
    {
        CLASSIC = 0,
        GOLD = 1,
        PLATINUM = 2,
        SUPER_PREMIUM = 3
    };

    inline std::string creditCardTypeToString(CreditCardType t)
    {
        switch (t)
        {
        case CreditCardType::CLASSIC:
            return "Silver Classic";
        case CreditCardType::GOLD:
            return "Gold Rewards";
        case CreditCardType::PLATINUM:
            return "Platinum Select";
        case CreditCardType::SUPER_PREMIUM:
            return "Super Premium Royale";
        default:
            return "Standard Card";
        }
    }

    // ==========================================
    // DATA STRUCTURES
    // ==========================================

    struct AuditLog
    {
        std::string logId;
        std::string timestamp;
        std::string actorId;
        Role actorRole{Role::PUBLIC_GUEST};
        std::string action;
        std::string bankId;
        std::string details;

        std::string serialize() const
        {
            std::stringstream ss;
            ss << logId << "|" << timestamp << "|" << actorId << "|" << static_cast<int>(actorRole)
               << "|" << action << "|" << bankId << "|" << details;
            return ss.str();
        }

        static AuditLog deserialize(const std::string &line)
        {
            AuditLog log;
            const auto tokens = Persistence::split(line);
            if (tokens.size() >= 7)
            {
                log.logId = tokens[0];
                log.timestamp = tokens[1];
                log.actorId = tokens[2];
                log.actorRole = static_cast<Role>(std::stoi(tokens[3]));
                log.action = tokens[4];
                log.bankId = tokens[5];
                log.details = tokens[6];
            }
            return log;
        }
    };

    struct Transaction
    {
        std::string txnId;
        std::string timestamp;
        std::string accountNo;
        std::string txnType; // DEPOSIT, WITHDRAW, TRANSFER_IN, TRANSFER_OUT, INTEREST, EMI_PAYMENT, FD_CREATION
        Money amount{};
        Money balanceAfter{};
        std::string narration;
        std::string processedByStaffId;

        std::string serialize() const
        {
            std::stringstream ss;
            ss << txnId << "|" << timestamp << "|" << accountNo << "|" << txnType << "|"
               << amount << "|" << balanceAfter << "|" << narration << "|" << processedByStaffId;
            return ss.str();
        }

        static Transaction deserialize(const std::string &line)
        {
            Transaction t;
            const auto tokens = Persistence::split(line);
            if (tokens.size() >= 8)
            {
                t.txnId = tokens[0];
                t.timestamp = tokens[1];
                t.accountNo = tokens[2];
                t.txnType = tokens[3];
                t.amount = Money::parse(tokens[4]);
                t.balanceAfter = Money::parse(tokens[5]);
                t.narration = tokens[6];
                t.processedByStaffId = tokens[7];
            }
            return t;
        }
    };

    struct Customer
    {
        std::string customerId;
        std::string name;
        std::string dob;
        std::string pan;
        std::string aadhaar;
        std::string address;
        std::string phone;
        std::string email;
        int creditScore{0}; // 300 to 900
        bool isKycVerified{false};
        bool isSeniorCitizen{false};
        std::string passwordHash;
        std::string bankId; // Registered primary bank ID

        std::string serialize() const
        {
            std::stringstream ss;
            ss << customerId << "|" << name << "|" << dob << "|" << pan << "|" << aadhaar << "|"
               << address << "|" << phone << "|" << email << "|" << creditScore << "|"
               << (isKycVerified ? 1 : 0) << "|" << (isSeniorCitizen ? 1 : 0) << "|"
               << passwordHash << "|" << bankId;
            return ss.str();
        }

        static Customer deserialize(const std::string &line)
        {
            Customer c;
            const auto tokens = Persistence::split(line);
            if (tokens.size() >= 13)
            {
                c.customerId = tokens[0];
                c.name = tokens[1];
                c.dob = tokens[2];
                c.pan = tokens[3];
                c.aadhaar = tokens[4];
                c.address = tokens[5];
                c.phone = tokens[6];
                c.email = tokens[7];
                c.creditScore = std::stoi(tokens[8]);
                c.isKycVerified = (tokens[9] == "1");
                c.isSeniorCitizen = (tokens[10] == "1");
                c.passwordHash = tokens[11];
                c.bankId = tokens[12];
            }
            return c;
        }
    };

    struct Staff
    {
        std::string staffId;
        std::string name;
        Role role{Role::PUBLIC_GUEST};
        std::string bankId;
        std::string branchCode;
        std::string email;
        std::string passwordHash;

        std::string serialize() const
        {
            std::stringstream ss;
            ss << staffId << "|" << name << "|" << static_cast<int>(role) << "|" << bankId
               << "|" << branchCode << "|" << email << "|" << passwordHash;
            return ss.str();
        }

        static Staff deserialize(const std::string &line)
        {
            Staff s;
            const auto tokens = Persistence::split(line);
            if (tokens.size() >= 7)
            {
                s.staffId = tokens[0];
                s.name = tokens[1];
                s.role = static_cast<Role>(std::stoi(tokens[2]));
                s.bankId = tokens[3];
                s.branchCode = tokens[4];
                s.email = tokens[5];
                s.passwordHash = tokens[6];
            }
            return s;
        }
    };

    struct Account
    {
        std::string accountNo;
        std::string customerId;
        std::string bankId;
        std::string branchCode;
        AccountType type{AccountType::SAVINGS};
        Money balance{};
        Money overdraftLimit{}; // For CURRENT accounts
        bool isActive{false};
        bool hasDebitCard{false};
        bool hasChequeBook{false};
        std::string createdDate;

        std::string serialize() const
        {
            std::stringstream ss;
            ss << accountNo << "|" << customerId << "|" << bankId << "|" << branchCode << "|"
               << static_cast<int>(type) << "|" << balance << "|" << overdraftLimit << "|"
               << (isActive ? 1 : 0) << "|" << (hasDebitCard ? 1 : 0) << "|"
               << (hasChequeBook ? 1 : 0) << "|" << createdDate;
            return ss.str();
        }

        static Account deserialize(const std::string &line)
        {
            Account a;
            const auto tokens = Persistence::split(line);
            if (tokens.size() >= 11)
            {
                a.accountNo = tokens[0];
                a.customerId = tokens[1];
                a.bankId = tokens[2];
                a.branchCode = tokens[3];
                a.type = static_cast<AccountType>(std::stoi(tokens[4]));
                a.balance = Money::parse(tokens[5]);
                a.overdraftLimit = Money::parse(tokens[6]);
                a.isActive = (tokens[7] == "1");
                a.hasDebitCard = (tokens[8] == "1");
                a.hasChequeBook = (tokens[9] == "1");
                a.createdDate = tokens[10];
            }
            return a;
        }
    };

    struct FixedDeposit
    {
        std::string fdNo;
        std::string customerId;
        std::string bankId;
        std::string linkedAccountNo;
        Money principal{};
        int tenureMonths{0};
        double interestRate{0.0};
        Money maturityAmount{};
        std::string startDate;
        std::string maturityDate;
        bool isSeniorCitizen{false};
        bool isClosed{false};
        Money tdsDeducted{};

        std::string serialize() const
        {
            std::stringstream ss;
            ss << fdNo << "|" << customerId << "|" << bankId << "|" << linkedAccountNo << "|"
               << principal << "|" << tenureMonths << "|" << interestRate << "|"
               << maturityAmount << "|" << startDate << "|" << maturityDate << "|"
               << (isSeniorCitizen ? 1 : 0) << "|" << (isClosed ? 1 : 0) << "|" << tdsDeducted;
            return ss.str();
        }

        static FixedDeposit deserialize(const std::string &line)
        {
            FixedDeposit fd;
            const auto tokens = Persistence::split(line);
            if (tokens.size() >= 13)
            {
                fd.fdNo = tokens[0];
                fd.customerId = tokens[1];
                fd.bankId = tokens[2];
                fd.linkedAccountNo = tokens[3];
                fd.principal = Money::parse(tokens[4]);
                fd.tenureMonths = std::stoi(tokens[5]);
                fd.interestRate = std::stod(tokens[6]);
                fd.maturityAmount = Money::parse(tokens[7]);
                fd.startDate = tokens[8];
                fd.maturityDate = tokens[9];
                fd.isSeniorCitizen = (tokens[10] == "1");
                fd.isClosed = (tokens[11] == "1");
                fd.tdsDeducted = Money::parse(tokens[12]);
            }
            return fd;
        }
    };

    struct RecurringDeposit
    {
        std::string rdNo;
        std::string customerId;
        std::string bankId;
        std::string linkedAccountNo;
        Money monthlyInstallment{};
        int tenureMonths{0};
        int monthsPaid{0};
        double interestRate{0.0};
        Money maturityAmount{};
        std::string startDate;
        bool isClosed{false};

        std::string serialize() const
        {
            std::stringstream ss;
            ss << rdNo << "|" << customerId << "|" << bankId << "|" << linkedAccountNo << "|"
               << monthlyInstallment << "|" << tenureMonths << "|" << monthsPaid << "|"
               << interestRate << "|" << maturityAmount << "|" << startDate << "|"
               << (isClosed ? 1 : 0);
            return ss.str();
        }

        static RecurringDeposit deserialize(const std::string &line)
        {
            RecurringDeposit rd;
            const auto tokens = Persistence::split(line);
            if (tokens.size() >= 11)
            {
                rd.rdNo = tokens[0];
                rd.customerId = tokens[1];
                rd.bankId = tokens[2];
                rd.linkedAccountNo = tokens[3];
                rd.monthlyInstallment = Money::parse(tokens[4]);
                rd.tenureMonths = std::stoi(tokens[5]);
                rd.monthsPaid = std::stoi(tokens[6]);
                rd.interestRate = std::stod(tokens[7]);
                rd.maturityAmount = Money::parse(tokens[8]);
                rd.startDate = tokens[9];
                rd.isClosed = (tokens[10] == "1");
            }
            return rd;
        }
    };

    struct Loan
    {
        std::string loanId;
        std::string customerId;
        std::string bankId;
        std::string linkedAccountNo;
        LoanType type{LoanType::PERSONAL};
        Money principalAmount{};
        double interestRate{0.0};
        int tenureMonths{0};
        Money emiAmount{};
        Money totalOutstanding{};
        Money totalPaid{};
        LoanStatus status{LoanStatus::APPLIED};
        std::string appliedDate;
        std::string sanctionedDate;
        std::string approvalHistory;

        std::string serialize() const
        {
            std::stringstream ss;
            ss << loanId << "|" << customerId << "|" << bankId << "|" << linkedAccountNo << "|"
               << static_cast<int>(type) << "|" << principalAmount << "|" << interestRate << "|"
               << tenureMonths << "|" << emiAmount << "|" << totalOutstanding << "|"
               << totalPaid << "|" << static_cast<int>(status) << "|" << appliedDate << "|"
               << sanctionedDate << "|" << approvalHistory;
            return ss.str();
        }

        static Loan deserialize(const std::string &line)
        {
            Loan l;
            const auto tokens = Persistence::split(line);
            if (tokens.size() >= 15)
            {
                l.loanId = tokens[0];
                l.customerId = tokens[1];
                l.bankId = tokens[2];
                l.linkedAccountNo = tokens[3];
                l.type = static_cast<LoanType>(std::stoi(tokens[4]));
                l.principalAmount = Money::parse(tokens[5]);
                l.interestRate = std::stod(tokens[6]);
                l.tenureMonths = std::stoi(tokens[7]);
                l.emiAmount = Money::parse(tokens[8]);
                l.totalOutstanding = Money::parse(tokens[9]);
                l.totalPaid = Money::parse(tokens[10]);
                l.status = static_cast<LoanStatus>(std::stoi(tokens[11]));
                l.appliedDate = tokens[12];
                l.sanctionedDate = tokens[13];
                l.approvalHistory = tokens[14];
            }
            return l;
        }
    };

    struct InsurancePolicy
    {
        std::string policyNo;
        std::string customerId;
        std::string bankId;
        InsuranceType type{InsuranceType::LIFE_TERM};
        Money coverageAmount{};
        Money annualPremium{};
        std::string nomineeName;
        std::string nomineeRelation;
        std::string startDate;
        bool isActive{false};
        bool claimPending{false};
        Money claimAmountRequested{};
        std::string claimStatus;

        std::string serialize() const
        {
            std::stringstream ss;
            ss << policyNo << "|" << customerId << "|" << bankId << "|"
               << static_cast<int>(type) << "|" << coverageAmount << "|" << annualPremium << "|"
               << nomineeName << "|" << nomineeRelation << "|" << startDate << "|"
               << (isActive ? 1 : 0) << "|" << (claimPending ? 1 : 0) << "|"
               << claimAmountRequested << "|" << claimStatus;
            return ss.str();
        }

        static InsurancePolicy deserialize(const std::string &line)
        {
            InsurancePolicy ip;
            const auto tokens = Persistence::split(line);
            if (tokens.size() >= 13)
            {
                ip.policyNo = tokens[0];
                ip.customerId = tokens[1];
                ip.bankId = tokens[2];
                ip.type = static_cast<InsuranceType>(std::stoi(tokens[3]));
                ip.coverageAmount = Money::parse(tokens[4]);
                ip.annualPremium = Money::parse(tokens[5]);
                ip.nomineeName = tokens[6];
                ip.nomineeRelation = tokens[7];
                ip.startDate = tokens[8];
                ip.isActive = (tokens[9] == "1");
                ip.claimPending = (tokens[10] == "1");
                ip.claimAmountRequested = Money::parse(tokens[11]);
                ip.claimStatus = tokens[12];
            }
            return ip;
        }
    };

    struct CreditCard
    {
        std::string cardNumber;
        std::string customerId;
        std::string bankId;
        std::string linkedAccountNo;
        CreditCardType type{CreditCardType::CLASSIC};
        Money creditLimit{};
        Money availableLimit{};
        Money currentOutstanding{};
        double rewardPoints{0.0};
        int cvv{0};
        std::string expiryDate;
        bool isBlocked{false};

        std::string serialize() const
        {
            std::stringstream ss;
            ss << cardNumber << "|" << customerId << "|" << bankId << "|" << linkedAccountNo << "|"
               << static_cast<int>(type) << "|" << creditLimit << "|" << availableLimit << "|"
               << currentOutstanding << "|" << rewardPoints << "|" << 0 << "|"
               << expiryDate << "|" << (isBlocked ? 1 : 0);
            return ss.str();
        }

        static CreditCard deserialize(const std::string &line)
        {
            CreditCard cc;
            const auto tokens = Persistence::split(line);
            if (tokens.size() >= 12)
            {
                cc.cardNumber = tokens[0];
                cc.customerId = tokens[1];
                cc.bankId = tokens[2];
                cc.linkedAccountNo = tokens[3];
                cc.type = static_cast<CreditCardType>(std::stoi(tokens[4]));
                cc.creditLimit = Money::parse(tokens[5]);
                cc.availableLimit = Money::parse(tokens[6]);
                cc.currentOutstanding = Money::parse(tokens[7]);
                cc.rewardPoints = std::stod(tokens[8]);
                cc.cvv = std::stoi(tokens[9]);
                cc.expiryDate = tokens[10];
                cc.isBlocked = (tokens[11] == "1");
            }
            return cc;
        }
    };

    struct Branch
    {
        std::string branchCode;
        std::string bankId;
        std::string city;
        std::string ifscCode;
        std::string managerStaffId;
    };

    struct BankPolicies
    {
        double minSavingsBalance = 1000.0;
        double savingsInterestRate = 3.50; // % per annum
        double homeLoanBaseRate = 8.50;
        double personalLoanBaseRate = 12.0;
        double carLoanBaseRate = 9.0;
        double eduLoanBaseRate = 9.5;
        double fd1YrRate = 6.75;
        double fd3YrRate = 7.10;
        double fd5YrRate = 7.50;
        double seniorCitizenBonus = 0.50; // +0.5%
    };

    struct BankInfo
    {
        std::string bankId;
        std::string name;
        std::string code; // SBI, HDFC, etc.
        std::string cmdStaffId;
        BankPolicies policies;
        bool isLicensed = true;
        Money totalDeposits{};
        Money totalLoansDisbursed{};
        Money totalNPA{};
    };

    struct RBIPolicy
    {
        double repoRate = 6.50;        // %
        double reverseRepoRate = 6.25; // %
        double crr = 4.50;             // Cash Reserve Ratio %
        double slr = 18.00;            // Statutory Liquidity Ratio %
        std::string lastUpdatedDate;
        std::vector<std::string> circulars;
    };

    inline std::ostream &operator<<(std::ostream &os, Role r) { return os << roleToString(r); }
    inline std::ostream &operator<<(std::ostream &os, AccountType t) { return os << accountTypeToString(t); }
    inline std::ostream &operator<<(std::ostream &os, LoanType t) { return os << loanTypeToString(t); }
    inline std::ostream &operator<<(std::ostream &os, LoanStatus s) { return os << loanStatusToString(s); }
    inline std::ostream &operator<<(std::ostream &os, InsuranceType t) { return os << insuranceTypeToString(t); }
    inline std::ostream &operator<<(std::ostream &os, CreditCardType t) { return os << creditCardTypeToString(t); }

} // namespace BankingSystem

#endif // STRUCTURES_H
