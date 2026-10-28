#ifndef BANKING_SYSTEM_COMMON_TYPES_HPP
#define BANKING_SYSTEM_COMMON_TYPES_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <iostream>

namespace BankingSystem {

// ==========================================
// 1. Role & Authority Tier (R2)
// ==========================================
enum class RoleTier : uint8_t {
    RBI_GOVERNOR         = 0,
    BANK_CMD             = 1,
    ZONAL_MANAGER        = 2,
    BRANCH_MANAGER       = 3,
    LOAN_OFFICER         = 4,
    RELATIONSHIP_MANAGER = 5,
    TELLER               = 6,
    CUSTOMER             = 7,
    PUBLIC_GUEST         = 8
};

// Convenience type alias
using Role = RoleTier;

std::string roleTierToString(RoleTier role);
RoleTier stringToRoleTier(const std::string& str);

// ==========================================
// 2. Account Types & Statuses (R3)
// ==========================================
enum class AccountType : uint8_t {
    SAVINGS = 0,
    CURRENT = 1, // Overdraft enabled
    NRI     = 2, // NRE / NRO
    JOINT   = 3  // Either or Survivor / Jointly
};

std::string accountTypeToString(AccountType type);
AccountType stringToAccountType(const std::string& str);

enum class AccountStatus : uint8_t {
    ACTIVE  = 0,
    FROZEN  = 1,
    DORMANT = 2,
    CLOSED  = 3
};

std::string accountStatusToString(AccountStatus status);
AccountStatus stringToAccountStatus(const std::string& str);

// ==========================================
// 3. Transaction Types (R2, R3)
// ==========================================
enum class TransactionType : uint8_t {
    DEPOSIT           = 0,
    WITHDRAWAL        = 1,
    TRANSFER_IN       = 2,
    TRANSFER_OUT      = 3,
    INTEREST_CREDIT   = 4,
    TDS_DEDUCTION     = 5,
    LOAN_DISBURSAL    = 6,
    LOAN_REPAYMENT    = 7,
    FEE_DEDUCTION     = 8,
    CARD_PAYMENT      = 9,
    INSURANCE_PREMIUM = 10
};

std::string transactionTypeToString(TransactionType type);
TransactionType stringToTransactionType(const std::string& str);

// ==========================================
// 4. Loan Categories & Approval Workflow Status (R2, R3)
// ==========================================
enum class LoanType : uint8_t {
    PERSONAL  = 0,
    HOME      = 1,
    VEHICLE   = 2,
    BUSINESS  = 3,
    EDUCATION = 4
};

std::string loanTypeToString(LoanType type);
LoanType stringToLoanType(const std::string& str);

enum class LoanStatus : uint8_t {
    APPLIED                  = 0,
    BRANCH_MANAGER_APPROVED  = 1,
    ZONAL_MANAGER_APPROVED   = 2,
    CMD_APPROVED             = 3,
    REJECTED                 = 4,
    DISBURSED                = 5,
    CLOSED                   = 6,
    DEFAULTED                = 7
};

std::string loanStatusToString(LoanStatus status);
LoanStatus stringToLoanStatus(const std::string& str);

// ==========================================
// 5. Term Deposit Types (R3)
// ==========================================
enum class TermDepositType : uint8_t {
    FIXED_DEPOSIT     = 0,
    RECURRING_DEPOSIT = 1
};

std::string termDepositTypeToString(TermDepositType type);
TermDepositType stringToTermDepositType(const std::string& str);

// ==========================================
// 6. Credit Card Tiers (R3)
// ==========================================
enum class CreditCardTier : uint8_t {
    CLASSIC       = 0,
    GOLD          = 1,
    PLATINUM      = 2,
    SUPER_PREMIUM = 3
};

std::string creditCardTierToString(CreditCardTier tier);
CreditCardTier stringToCreditCardTier(const std::string& str);

// ==========================================
// 7. Insurance Products (R3)
// ==========================================
enum class InsuranceType : uint8_t {
    LIFE    = 0,
    HEALTH  = 1,
    VEHICLE = 2
};

std::string insuranceTypeToString(InsuranceType type);
InsuranceType stringToInsuranceType(const std::string& str);

enum class InsuranceStatus : uint8_t {
    ACTIVE     = 0,
    LAPSED     = 1,
    CLAIMED    = 2,
    TERMINATED = 3
};

std::string insuranceStatusToString(InsuranceStatus status);
InsuranceStatus stringToInsuranceStatus(const std::string& str);

// Stream output overloads
std::ostream& operator<<(std::ostream& os, RoleTier role);
std::ostream& operator<<(std::ostream& os, AccountType type);
std::ostream& operator<<(std::ostream& os, AccountStatus status);
std::ostream& operator<<(std::ostream& os, TransactionType type);
std::ostream& operator<<(std::ostream& os, LoanType type);
std::ostream& operator<<(std::ostream& os, LoanStatus status);
std::ostream& operator<<(std::ostream& os, TermDepositType type);
std::ostream& operator<<(std::ostream& os, CreditCardTier tier);
std::ostream& operator<<(std::ostream& os, InsuranceType type);
std::ostream& operator<<(std::ostream& os, InsuranceStatus status);

} // namespace BankingSystem

#endif // BANKING_SYSTEM_COMMON_TYPES_HPP
