#include "common/Types.hpp"
#include <algorithm>
#include <cctype>

namespace BankingSystem {

namespace {
    std::string toUpperTrim(const std::string& str) {
        std::string s = str;
        // Trim
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\n\r");
        s = s.substr(start, end - start + 1);
        // Uppercase
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
        return s;
    }
}

// ------------------------------------------
// RoleTier
// ------------------------------------------
std::string roleTierToString(RoleTier role) {
    switch (role) {
        case RoleTier::RBI_GOVERNOR:         return "RBI Governor";
        case RoleTier::BANK_CMD:             return "Bank CMD / CEO";
        case RoleTier::ZONAL_MANAGER:        return "Zonal Manager";
        case RoleTier::BRANCH_MANAGER:       return "Branch Manager";
        case RoleTier::LOAN_OFFICER:         return "Loan Officer";
        case RoleTier::RELATIONSHIP_MANAGER: return "Relationship Manager";
        case RoleTier::TELLER:               return "Teller / Cashier";
        case RoleTier::CUSTOMER:             return "Customer";
        case RoleTier::PUBLIC_GUEST:         return "Public Guest";
        default:                             return "Unknown Role";
    }
}

RoleTier stringToRoleTier(const std::string& str) {
    std::string s = toUpperTrim(str);
    if (s == "RBI GOVERNOR" || s == "RBI_GOVERNOR" || s == "GOVERNOR" || s == "0") return RoleTier::RBI_GOVERNOR;
    if (s == "BANK CMD / CEO" || s == "BANK_CMD" || s == "CMD" || s == "1") return RoleTier::BANK_CMD;
    if (s == "ZONAL MANAGER" || s == "ZONAL_MANAGER" || s == "2") return RoleTier::ZONAL_MANAGER;
    if (s == "BRANCH MANAGER" || s == "BRANCH_MANAGER" || s == "3") return RoleTier::BRANCH_MANAGER;
    if (s == "LOAN OFFICER" || s == "LOAN_OFFICER" || s == "4") return RoleTier::LOAN_OFFICER;
    if (s == "RELATIONSHIP MANAGER" || s == "RELATIONSHIP_MANAGER" || s == "5") return RoleTier::RELATIONSHIP_MANAGER;
    if (s == "TELLER / CASHIER" || s == "TELLER" || s == "CASHIER" || s == "6") return RoleTier::TELLER;
    if (s == "CUSTOMER" || s == "7") return RoleTier::CUSTOMER;
    if (s == "PUBLIC GUEST" || s == "PUBLIC_GUEST" || s == "GUEST" || s == "8") return RoleTier::PUBLIC_GUEST;
    return RoleTier::CUSTOMER;
}

// ------------------------------------------
// AccountType
// ------------------------------------------
std::string accountTypeToString(AccountType type) {
    switch (type) {
        case AccountType::SAVINGS: return "Savings";
        case AccountType::CURRENT: return "Current";
        case AccountType::NRI:     return "NRI";
        case AccountType::JOINT:   return "Joint";
        default:                   return "Unknown";
    }
}

AccountType stringToAccountType(const std::string& str) {
    std::string s = toUpperTrim(str);
    if (s == "SAVINGS" || s == "SAVINGS ACCOUNT" || s == "0") return AccountType::SAVINGS;
    if (s == "CURRENT" || s == "CURRENT ACCOUNT" || s == "1") return AccountType::CURRENT;
    if (s == "NRI" || s == "NRI ACCOUNT" || s == "2") return AccountType::NRI;
    if (s == "JOINT" || s == "JOINT ACCOUNT" || s == "3") return AccountType::JOINT;
    return AccountType::SAVINGS;
}

// ------------------------------------------
// AccountStatus
// ------------------------------------------
std::string accountStatusToString(AccountStatus status) {
    switch (status) {
        case AccountStatus::ACTIVE:  return "ACTIVE";
        case AccountStatus::FROZEN:  return "FROZEN";
        case AccountStatus::DORMANT: return "DORMANT";
        case AccountStatus::CLOSED:  return "CLOSED";
        default:                     return "UNKNOWN";
    }
}

AccountStatus stringToAccountStatus(const std::string& str) {
    std::string s = toUpperTrim(str);
    if (s == "ACTIVE" || s == "0") return AccountStatus::ACTIVE;
    if (s == "FROZEN" || s == "1") return AccountStatus::FROZEN;
    if (s == "DORMANT" || s == "2") return AccountStatus::DORMANT;
    if (s == "CLOSED" || s == "3") return AccountStatus::CLOSED;
    return AccountStatus::ACTIVE;
}

// ------------------------------------------
// TransactionType
// ------------------------------------------
std::string transactionTypeToString(TransactionType type) {
    switch (type) {
        case TransactionType::DEPOSIT:           return "DEPOSIT";
        case TransactionType::WITHDRAWAL:        return "WITHDRAWAL";
        case TransactionType::TRANSFER_IN:       return "TRANSFER_IN";
        case TransactionType::TRANSFER_OUT:      return "TRANSFER_OUT";
        case TransactionType::INTEREST_CREDIT:   return "INTEREST_CREDIT";
        case TransactionType::TDS_DEDUCTION:     return "TDS_DEDUCTION";
        case TransactionType::LOAN_DISBURSAL:    return "LOAN_DISBURSAL";
        case TransactionType::LOAN_REPAYMENT:    return "LOAN_REPAYMENT";
        case TransactionType::FEE_DEDUCTION:     return "FEE_DEDUCTION";
        case TransactionType::CARD_PAYMENT:      return "CARD_PAYMENT";
        case TransactionType::INSURANCE_PREMIUM: return "INSURANCE_PREMIUM";
        default:                                 return "UNKNOWN";
    }
}

TransactionType stringToTransactionType(const std::string& str) {
    std::string s = toUpperTrim(str);
    if (s == "DEPOSIT" || s == "0") return TransactionType::DEPOSIT;
    if (s == "WITHDRAWAL" || s == "1") return TransactionType::WITHDRAWAL;
    if (s == "TRANSFER_IN" || s == "TRANSFER IN" || s == "2") return TransactionType::TRANSFER_IN;
    if (s == "TRANSFER_OUT" || s == "TRANSFER OUT" || s == "3") return TransactionType::TRANSFER_OUT;
    if (s == "INTEREST_CREDIT" || s == "INTEREST CREDIT" || s == "4") return TransactionType::INTEREST_CREDIT;
    if (s == "TDS_DEDUCTION" || s == "TDS DEDUCTION" || s == "5") return TransactionType::TDS_DEDUCTION;
    if (s == "LOAN_DISBURSAL" || s == "LOAN DISBURSAL" || s == "6") return TransactionType::LOAN_DISBURSAL;
    if (s == "LOAN_REPAYMENT" || s == "LOAN REPAYMENT" || s == "7") return TransactionType::LOAN_REPAYMENT;
    if (s == "FEE_DEDUCTION" || s == "FEE DEDUCTION" || s == "8") return TransactionType::FEE_DEDUCTION;
    if (s == "CARD_PAYMENT" || s == "CARD PAYMENT" || s == "9") return TransactionType::CARD_PAYMENT;
    if (s == "INSURANCE_PREMIUM" || s == "INSURANCE PREMIUM" || s == "10") return TransactionType::INSURANCE_PREMIUM;
    return TransactionType::DEPOSIT;
}

// ------------------------------------------
// LoanType
// ------------------------------------------
std::string loanTypeToString(LoanType type) {
    switch (type) {
        case LoanType::PERSONAL:  return "Personal Loan";
        case LoanType::HOME:      return "Home Loan";
        case LoanType::VEHICLE:   return "Vehicle Loan";
        case LoanType::BUSINESS:  return "Business Loan";
        case LoanType::EDUCATION: return "Education Loan";
        default:                  return "General Loan";
    }
}

LoanType stringToLoanType(const std::string& str) {
    std::string s = toUpperTrim(str);
    if (s == "PERSONAL" || s == "PERSONAL LOAN" || s == "0") return LoanType::PERSONAL;
    if (s == "HOME" || s == "HOME LOAN" || s == "1") return LoanType::HOME;
    if (s == "VEHICLE" || s == "VEHICLE LOAN" || s == "CAR LOAN" || s == "2") return LoanType::VEHICLE;
    if (s == "BUSINESS" || s == "BUSINESS LOAN" || s == "3") return LoanType::BUSINESS;
    if (s == "EDUCATION" || s == "EDUCATION LOAN" || s == "4") return LoanType::EDUCATION;
    return LoanType::PERSONAL;
}

// ------------------------------------------
// LoanStatus
// ------------------------------------------
std::string loanStatusToString(LoanStatus status) {
    switch (status) {
        case LoanStatus::APPLIED:                 return "APPLIED";
        case LoanStatus::BRANCH_MANAGER_APPROVED: return "BRANCH_MANAGER_APPROVED";
        case LoanStatus::ZONAL_MANAGER_APPROVED:  return "ZONAL_MANAGER_APPROVED";
        case LoanStatus::CMD_APPROVED:            return "CMD_APPROVED";
        case LoanStatus::REJECTED:                return "REJECTED";
        case LoanStatus::DISBURSED:               return "DISBURSED";
        case LoanStatus::CLOSED:                  return "CLOSED";
        case LoanStatus::DEFAULTED:               return "DEFAULTED";
        default:                                  return "UNKNOWN";
    }
}

LoanStatus stringToLoanStatus(const std::string& str) {
    std::string s = toUpperTrim(str);
    if (s == "APPLIED" || s == "SUBMITTED" || s == "0") return LoanStatus::APPLIED;
    if (s == "BRANCH_MANAGER_APPROVED" || s == "BM_APPROVED" || s == "1") return LoanStatus::BRANCH_MANAGER_APPROVED;
    if (s == "ZONAL_MANAGER_APPROVED" || s == "ZM_APPROVED" || s == "2") return LoanStatus::ZONAL_MANAGER_APPROVED;
    if (s == "CMD_APPROVED" || s == "APPROVED" || s == "3") return LoanStatus::CMD_APPROVED;
    if (s == "REJECTED" || s == "4") return LoanStatus::REJECTED;
    if (s == "DISBURSED" || s == "5") return LoanStatus::DISBURSED;
    if (s == "CLOSED" || s == "6") return LoanStatus::CLOSED;
    if (s == "DEFAULTED" || s == "NPA_DEFAULT" || s == "7") return LoanStatus::DEFAULTED;
    return LoanStatus::APPLIED;
}

// ------------------------------------------
// TermDepositType
// ------------------------------------------
std::string termDepositTypeToString(TermDepositType type) {
    switch (type) {
        case TermDepositType::FIXED_DEPOSIT:     return "Fixed Deposit (FD)";
        case TermDepositType::RECURRING_DEPOSIT: return "Recurring Deposit (RD)";
        default:                                 return "Term Deposit";
    }
}

TermDepositType stringToTermDepositType(const std::string& str) {
    std::string s = toUpperTrim(str);
    if (s == "FIXED_DEPOSIT" || s == "FIXED DEPOSIT" || s == "FD" || s == "0") return TermDepositType::FIXED_DEPOSIT;
    if (s == "RECURRING_DEPOSIT" || s == "RECURRING DEPOSIT" || s == "RD" || s == "1") return TermDepositType::RECURRING_DEPOSIT;
    return TermDepositType::FIXED_DEPOSIT;
}

// ------------------------------------------
// CreditCardTier
// ------------------------------------------
std::string creditCardTierToString(CreditCardTier tier) {
    switch (tier) {
        case CreditCardTier::CLASSIC:       return "Classic";
        case CreditCardTier::GOLD:          return "Gold";
        case CreditCardTier::PLATINUM:      return "Platinum";
        case CreditCardTier::SUPER_PREMIUM: return "Super Premium";
        default:                            return "Standard";
    }
}

CreditCardTier stringToCreditCardTier(const std::string& str) {
    std::string s = toUpperTrim(str);
    if (s == "CLASSIC" || s == "0") return CreditCardTier::CLASSIC;
    if (s == "GOLD" || s == "1") return CreditCardTier::GOLD;
    if (s == "PLATINUM" || s == "2") return CreditCardTier::PLATINUM;
    if (s == "SUPER_PREMIUM" || s == "SUPER PREMIUM" || s == "3") return CreditCardTier::SUPER_PREMIUM;
    return CreditCardTier::CLASSIC;
}

// ------------------------------------------
// InsuranceType
// ------------------------------------------
std::string insuranceTypeToString(InsuranceType type) {
    switch (type) {
        case InsuranceType::LIFE:    return "Life Insurance";
        case InsuranceType::HEALTH:  return "Health Insurance";
        case InsuranceType::VEHICLE: return "Vehicle Insurance";
        default:                     return "General Insurance";
    }
}

InsuranceType stringToInsuranceType(const std::string& str) {
    std::string s = toUpperTrim(str);
    if (s == "LIFE" || s == "LIFE INSURANCE" || s == "0") return InsuranceType::LIFE;
    if (s == "HEALTH" || s == "HEALTH INSURANCE" || s == "1") return InsuranceType::HEALTH;
    if (s == "VEHICLE" || s == "VEHICLE INSURANCE" || s == "MOTOR" || s == "2") return InsuranceType::VEHICLE;
    return InsuranceType::LIFE;
}

// ------------------------------------------
// InsuranceStatus
// ------------------------------------------
std::string insuranceStatusToString(InsuranceStatus status) {
    switch (status) {
        case InsuranceStatus::ACTIVE:     return "ACTIVE";
        case InsuranceStatus::LAPSED:     return "LAPSED";
        case InsuranceStatus::CLAIMED:    return "CLAIMED";
        case InsuranceStatus::TERMINATED: return "TERMINATED";
        default:                          return "UNKNOWN";
    }
}

InsuranceStatus stringToInsuranceStatus(const std::string& str) {
    std::string s = toUpperTrim(str);
    if (s == "ACTIVE" || s == "0") return InsuranceStatus::ACTIVE;
    if (s == "LAPSED" || s == "1") return InsuranceStatus::LAPSED;
    if (s == "CLAIMED" || s == "2") return InsuranceStatus::CLAIMED;
    if (s == "TERMINATED" || s == "3") return InsuranceStatus::TERMINATED;
    return InsuranceStatus::ACTIVE;
}

// ------------------------------------------
// Stream output overloads
// ------------------------------------------
std::ostream& operator<<(std::ostream& os, RoleTier role) {
    return os << roleTierToString(role);
}

std::ostream& operator<<(std::ostream& os, AccountType type) {
    return os << accountTypeToString(type);
}

std::ostream& operator<<(std::ostream& os, AccountStatus status) {
    return os << accountStatusToString(status);
}

std::ostream& operator<<(std::ostream& os, TransactionType type) {
    return os << transactionTypeToString(type);
}

std::ostream& operator<<(std::ostream& os, LoanType type) {
    return os << loanTypeToString(type);
}

std::ostream& operator<<(std::ostream& os, LoanStatus status) {
    return os << loanStatusToString(status);
}

std::ostream& operator<<(std::ostream& os, TermDepositType type) {
    return os << termDepositTypeToString(type);
}

std::ostream& operator<<(std::ostream& os, CreditCardTier tier) {
    return os << creditCardTierToString(tier);
}

std::ostream& operator<<(std::ostream& os, InsuranceType type) {
    return os << insuranceTypeToString(type);
}

std::ostream& operator<<(std::ostream& os, InsuranceStatus status) {
    return os << insuranceStatusToString(status);
}

} // namespace BankingSystem
