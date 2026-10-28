#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include "TestHarness.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace E2ETestUtils {

// Launch RBI Banking System Executable
inline E2ETestHarness::CLIExecutionResult launchRBIExecutable(
    const std::string& stdinInput = "",
    const std::string& execPath = "./rbi_banking_system.exe",
    const std::string& workingDir = "."
) {
    return E2ETestHarness::executeCLI(execPath, stdinInput, workingDir);
}

// Convert Rupees (double) to Paise (int64_t)
inline int64_t rupeesToPaise(double rupees) {
    return static_cast<int64_t>(std::round(rupees * 100.0));
}

// Convert Paise (int64_t) to Rupees (double)
inline double paiseToRupees(int64_t paise) {
    return static_cast<double>(paise) / 100.0;
}

// Format Paise as string e.g. "1234.56"
inline std::string paiseToRupeesString(int64_t paise) {
    double r = paiseToRupees(paise);
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << r;
    return ss.str();
}

// Parsed Data Structures for System State Verification
struct ParsedAccount {
    std::string accNo;
    std::string bankCode;
    std::string accType;
    int64_t balancePaise = 0;
    std::string ownerId;
    std::string status;
};

struct ParsedLoan {
    std::string loanId;
    std::string applicantId;
    int64_t amountPaise = 0;
    std::string status;
    std::string currentApproverRole;
};

struct ParsedFD {
    std::string fdId;
    std::string accNo;
    int64_t principalPaise = 0;
    double interestRate = 0.0;
    int tenureMonths = 0;
    bool isSenior = false;
};

struct ParsedCreditCard {
    std::string cardNo;
    std::string customerId;
    std::string tier;
    int64_t limitPaise = 0;
    int64_t balancePaise = 0;
    int cibilScore = 0;
};

struct ParsedAuditLog {
    std::string timestamp;
    std::string actorId;
    std::string role;
    std::string action;
    std::string details;
};

struct ParsedBank {
    std::string code;
    std::string name;
    double repoRate = 0.0;
    double baseRate = 0.0;
    double savingsRate = 0.0;
};

// Parser Helpers for .dat / .csv persistence files
inline std::vector<ParsedAccount> parseAccountsFile(const std::string& filePath = "data/accounts.dat") {
    std::vector<ParsedAccount> list;
    auto rows = E2ETestHarness::PersistenceValidator::parsePipeDelimited(filePath, '|');
    for (const auto& row : rows) {
        if (row.size() >= 5) {
            ParsedAccount acc;
            acc.accNo = row[0];
            acc.bankCode = row[1];
            acc.accType = row[2];
            try { acc.balancePaise = std::stoll(row[3]); } catch (...) { acc.balancePaise = 0; }
            acc.ownerId = row[4];
            if (row.size() >= 6) acc.status = row[5];
            list.push_back(acc);
        }
    }
    return list;
}

inline std::vector<ParsedLoan> parseLoansFile(const std::string& filePath = "data/loans.dat") {
    std::vector<ParsedLoan> list;
    auto rows = E2ETestHarness::PersistenceValidator::parsePipeDelimited(filePath, '|');
    for (const auto& row : rows) {
        if (row.size() >= 5) {
            ParsedLoan loan;
            loan.loanId = row[0];
            loan.applicantId = row[1];
            try { loan.amountPaise = std::stoll(row[2]); } catch (...) { loan.amountPaise = 0; }
            loan.status = row[3];
            loan.currentApproverRole = row[4];
            list.push_back(loan);
        }
    }
    return list;
}

inline std::vector<ParsedFD> parseFDsFile(const std::string& filePath = "data/fd_rd.dat") {
    std::vector<ParsedFD> list;
    auto rows = E2ETestHarness::PersistenceValidator::parsePipeDelimited(filePath, '|');
    for (const auto& row : rows) {
        if (row.size() >= 6) {
            ParsedFD fd;
            fd.fdId = row[0];
            fd.accNo = row[1];
            try { fd.principalPaise = std::stoll(row[2]); } catch (...) { fd.principalPaise = 0; }
            try { fd.interestRate = std::stod(row[3]); } catch (...) { fd.interestRate = 0.0; }
            try { fd.tenureMonths = std::stoi(row[4]); } catch (...) { fd.tenureMonths = 0; }
            fd.isSenior = (row[5] == "1" || row[5] == "true" || row[5] == "TRUE");
            list.push_back(fd);
        }
    }
    return list;
}

inline std::vector<ParsedCreditCard> parseCreditCardsFile(const std::string& filePath = "data/credit_cards.dat") {
    std::vector<ParsedCreditCard> list;
    auto rows = E2ETestHarness::PersistenceValidator::parsePipeDelimited(filePath, '|');
    for (const auto& row : rows) {
        if (row.size() >= 6) {
            ParsedCreditCard cc;
            cc.cardNo = row[0];
            cc.customerId = row[1];
            cc.tier = row[2];
            try { cc.limitPaise = std::stoll(row[3]); } catch (...) { cc.limitPaise = 0; }
            try { cc.balancePaise = std::stoll(row[4]); } catch (...) { cc.balancePaise = 0; }
            try { cc.cibilScore = std::stoi(row[5]); } catch (...) { cc.cibilScore = 0; }
            list.push_back(cc);
        }
    }
    return list;
}

inline std::vector<ParsedAuditLog> parseAuditLogFile(const std::string& filePath = "data/audit.log") {
    std::vector<ParsedAuditLog> list;
    auto rows = E2ETestHarness::PersistenceValidator::parsePipeDelimited(filePath, '|');
    for (const auto& row : rows) {
        if (row.size() >= 5) {
            ParsedAuditLog log;
            log.timestamp = row[0];
            log.actorId = row[1];
            log.role = row[2];
            log.action = row[3];
            log.details = row[4];
            list.push_back(log);
        }
    }
    return list;
}

inline std::vector<ParsedBank> parseBanksFile(const std::string& filePath = "data/banks.dat") {
    std::vector<ParsedBank> list;
    auto rows = E2ETestHarness::PersistenceValidator::parsePipeDelimited(filePath, '|');
    for (const auto& row : rows) {
        if (row.size() >= 5) {
            ParsedBank bank;
            bank.code = row[0];
            bank.name = row[1];
            try { bank.repoRate = std::stod(row[2]); } catch (...) { bank.repoRate = 0.0; }
            try { bank.baseRate = std::stod(row[3]); } catch (...) { bank.baseRate = 0.0; }
            try { bank.savingsRate = std::stod(row[4]); } catch (...) { bank.savingsRate = 0.0; }
            list.push_back(bank);
        }
    }
    return list;
}

// Interest & Financial Math Calculation Helpers

// Calculate expected compound interest for FD (Quarterly Compounding)
inline int64_t calculateFDInterestPaise(int64_t principalPaise, double annualRatePct, double tenureYears, bool isSenior = false) {
    double effectiveRate = annualRatePct + (isSenior ? 0.50 : 0.0);
    double r = effectiveRate / 100.0;
    int n = 4; // Quarterly compounding
    double compoundFactor = std::pow(1.0 + (r / n), n * tenureYears);
    double totalAmount = static_cast<double>(principalPaise) * compoundFactor;
    double interest = totalAmount - static_cast<double>(principalPaise);
    return static_cast<int64_t>(std::round(interest));
}

// Calculate expected FD total maturity amount
inline int64_t calculateFDTotalAmountPaise(int64_t principalPaise, double annualRatePct, double tenureYears, bool isSenior = false) {
    return principalPaise + calculateFDInterestPaise(principalPaise, annualRatePct, tenureYears, isSenior);
}

// Calculate Section 194A TDS Deduction (10% on interest if > threshold)
// Threshold: ₹40,000 for regular (4,000,000 Paise), ₹50,000 for senior citizen (5,000,000 Paise)
inline int64_t calculateTDS(int64_t totalInterestPaise, bool isSenior = false) {
    int64_t thresholdPaise = isSenior ? 5000000LL : 4000000LL;
    if (totalInterestPaise > thresholdPaise) {
        return static_cast<int64_t>(std::round(static_cast<double>(totalInterestPaise) * 0.10));
    }
    return 0LL;
}

// Calculate Loan Escalation Required Approval Role
inline std::string getRequiredLoanApprovalRole(int64_t amountPaise) {
    int64_t rupees25Lakhs = 250000000LL;  // ₹25,00,000 in Paise
    int64_t rupees1Crore  = 1000000000LL; // ₹1,00,00,000 in Paise

    if (amountPaise < rupees25Lakhs) {
        return "Branch Manager";
    } else if (amountPaise <= rupees1Crore) {
        return "Zonal Manager";
    } else {
        return "CMD";
    }
}

// Get full loan escalation approval path
inline std::vector<std::string> getLoanEscalationPath(int64_t amountPaise) {
    int64_t rupees25Lakhs = 250000000LL;
    int64_t rupees1Crore  = 1000000000LL;

    std::vector<std::string> path;
    path.push_back("Loan Officer");
    path.push_back("Branch Manager");

    if (amountPaise >= rupees25Lakhs) {
        path.push_back("Zonal Manager");
    }
    if (amountPaise > rupees1Crore) {
        path.push_back("CMD");
    }
    return path;
}

// Daily APR Finance Charge helper for Credit Cards
inline int64_t calculateDailyFinanceCharge(int64_t balancePaise, double annualRatePct, int days) {
    double dailyRate = (annualRatePct / 100.0) / 365.0;
    double charge = static_cast<double>(balancePaise) * dailyRate * days;
    return static_cast<int64_t>(std::round(charge));
}

} // namespace E2ETestUtils

// Custom Assertion Helpers for Paise & Currency
#define ASSERT_PAISE_EQ(actualPaise, expectedPaise) \
    do { \
        int64_t _act = (actualPaise); \
        int64_t _exp = (expectedPaise); \
        if (_act != _exp) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_PAISE_EQ failed: Actual Paise [" << _act << " (" << E2ETestUtils::paiseToRupeesString(_act) \
                 << " INR)], Expected Paise [" << _exp << " (" << E2ETestUtils::paiseToRupeesString(_exp) << " INR)]"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_RUPEES_EQ(actualRupees, expectedRupees) \
    do { \
        double _actR = static_cast<double>(actualRupees); \
        double _expR = static_cast<double>(expectedRupees); \
        if (std::abs(_actR - _expR) > 0.009) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_RUPEES_EQ failed: Actual ₹" << std::fixed << std::setprecision(2) << _actR \
                 << ", Expected ₹" << _expR; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#endif // TEST_UTILS_HPP
