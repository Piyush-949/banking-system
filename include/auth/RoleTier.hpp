#ifndef RBI_BANKING_SYSTEM_ROLE_TIER_HPP
#define RBI_BANKING_SYSTEM_ROLE_TIER_HPP

#include "common/Types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace BankingSystem {
namespace Auth {

// Reuse RoleTier enum defined in BankingSystem::RoleTier
using RoleTier = BankingSystem::RoleTier;
using Role = BankingSystem::RoleTier;

/**
 * @brief Fine-grained capability permissions represented as bit flags in a 64-bit integer.
 */
enum class Permission : uint64_t {
    NONE                     = 0ULL,

    // RBI & Regulatory (Bits 0–7)
    MANAGE_RBI_POLICY        = 1ULL << 0,
    ISSUE_RBI_CIRCULAR       = 1ULL << 1,
    INSPECT_BANK_RESERVES    = 1ULL << 2,
    REVOKE_BANK_LICENSE      = 1ULL << 3,

    // Bank Management & Governance (Bits 8–15)
    SET_BANK_POLICIES        = 1ULL << 8,
    MANAGE_BRANCHES          = 1ULL << 9,
    MANAGE_STAFF             = 1ULL << 10,
    VIEW_BANK_ANALYTICS      = 1ULL << 11,

    // Branch Admin & Customer Onboarding (Bits 16–23)
    CREATE_CUSTOMER_ACCOUNT  = 1ULL << 16,
    MODIFY_ACCOUNT_STATUS    = 1ULL << 17,
    ISSUE_CREDIT_CARD        = 1ULL << 18,
    ISSUE_INSURANCE_POLICY   = 1ULL << 19,
    PROCESS_INSURANCE_CLAIM  = 1ULL << 20,

    // Teller & Cash Operations (Bits 24–31)
    PERFORM_CASH_DEPOSIT     = 1ULL << 24,
    PERFORM_CASH_WITHDRAWAL  = 1ULL << 25,
    PERFORM_FUNDS_TRANSFER   = 1ULL << 26,
    OVERRIDE_DAILY_LIMIT     = 1ULL << 27,
    VIEW_BRANCH_VAULT        = 1ULL << 28,

    // Loan Operations & Approvals (Bits 32–39)
    APPLY_LOAN               = 1ULL << 32,
    APPROVE_SMALL_LOAN       = 1ULL << 33, // <= 25L
    APPROVE_MEDIUM_LOAN      = 1ULL << 34, // 25L to 1Cr
    APPROVE_LARGE_LOAN       = 1ULL << 35, // > 1Cr
    DISBURSE_LOAN            = 1ULL << 36,
    REJECT_LOAN              = 1ULL << 37,

    // Customer Operations (Bits 40–47)
    VIEW_OWN_PROFILE         = 1ULL << 40,
    VIEW_OWN_TRANSACTIONS    = 1ULL << 41,
    SELF_TRANSFER            = 1ULL << 42,
    CREATE_TERM_DEPOSIT      = 1ULL << 43,
    BREAK_TERM_DEPOSIT       = 1ULL << 44,

    // Audit & System Monitoring (Bits 48–55)
    VIEW_AUDIT_LOGS          = 1ULL << 48,
    EXPORT_AUDIT_TRAIL       = 1ULL << 49,

    // Composite Presets
    ALL                      = 0xFFFFFFFFFFFFFFFFULL
};

// Bitwise Operators for Permission enum class
inline constexpr Permission operator|(Permission a, Permission b) {
    return static_cast<Permission>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline constexpr Permission operator&(Permission a, Permission b) {
    return static_cast<Permission>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

inline constexpr Permission operator~(Permission a) {
    return static_cast<Permission>(~static_cast<uint64_t>(a));
}

inline Permission& operator|=(Permission& a, Permission b) {
    a = a | b;
    return a;
}

inline Permission& operator&=(Permission& a, Permission b) {
    a = a & b;
    return a;
}

inline bool isValidRoleTier(uint8_t val) {
    return val <= static_cast<uint8_t>(RoleTier::PUBLIC_GUEST);
}

inline std::string permissionToString(Permission perm) {
    switch (perm) {
        case Permission::MANAGE_RBI_POLICY:       return "MANAGE_RBI_POLICY";
        case Permission::ISSUE_RBI_CIRCULAR:      return "ISSUE_RBI_CIRCULAR";
        case Permission::INSPECT_BANK_RESERVES:   return "INSPECT_BANK_RESERVES";
        case Permission::REVOKE_BANK_LICENSE:     return "REVOKE_BANK_LICENSE";
        case Permission::SET_BANK_POLICIES:       return "SET_BANK_POLICIES";
        case Permission::MANAGE_BRANCHES:         return "MANAGE_BRANCHES";
        case Permission::MANAGE_STAFF:            return "MANAGE_STAFF";
        case Permission::VIEW_BANK_ANALYTICS:     return "VIEW_BANK_ANALYTICS";
        case Permission::CREATE_CUSTOMER_ACCOUNT: return "CREATE_CUSTOMER_ACCOUNT";
        case Permission::MODIFY_ACCOUNT_STATUS:   return "MODIFY_ACCOUNT_STATUS";
        case Permission::ISSUE_CREDIT_CARD:       return "ISSUE_CREDIT_CARD";
        case Permission::ISSUE_INSURANCE_POLICY:  return "ISSUE_INSURANCE_POLICY";
        case Permission::PROCESS_INSURANCE_CLAIM: return "PROCESS_INSURANCE_CLAIM";
        case Permission::PERFORM_CASH_DEPOSIT:    return "PERFORM_CASH_DEPOSIT";
        case Permission::PERFORM_CASH_WITHDRAWAL: return "PERFORM_CASH_WITHDRAWAL";
        case Permission::PERFORM_FUNDS_TRANSFER:  return "PERFORM_FUNDS_TRANSFER";
        case Permission::OVERRIDE_DAILY_LIMIT:    return "OVERRIDE_DAILY_LIMIT";
        case Permission::VIEW_BRANCH_VAULT:       return "VIEW_BRANCH_VAULT";
        case Permission::APPLY_LOAN:              return "APPLY_LOAN";
        case Permission::APPROVE_SMALL_LOAN:      return "APPROVE_SMALL_LOAN";
        case Permission::APPROVE_MEDIUM_LOAN:     return "APPROVE_MEDIUM_LOAN";
        case Permission::APPROVE_LARGE_LOAN:      return "APPROVE_LARGE_LOAN";
        case Permission::DISBURSE_LOAN:           return "DISBURSE_LOAN";
        case Permission::REJECT_LOAN:             return "REJECT_LOAN";
        case Permission::VIEW_OWN_PROFILE:        return "VIEW_OWN_PROFILE";
        case Permission::VIEW_OWN_TRANSACTIONS:   return "VIEW_OWN_TRANSACTIONS";
        case Permission::SELF_TRANSFER:           return "SELF_TRANSFER";
        case Permission::CREATE_TERM_DEPOSIT:     return "CREATE_TERM_DEPOSIT";
        case Permission::BREAK_TERM_DEPOSIT:      return "BREAK_TERM_DEPOSIT";
        case Permission::VIEW_AUDIT_LOGS:         return "VIEW_AUDIT_LOGS";
        case Permission::EXPORT_AUDIT_TRAIL:      return "EXPORT_AUDIT_TRAIL";
        default:                                  return "UNKNOWN_PERMISSION";
    }
}

} // namespace Auth
} // namespace BankingSystem

#endif // RBI_BANKING_SYSTEM_ROLE_TIER_HPP
