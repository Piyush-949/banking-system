#ifndef RBI_BANKING_SYSTEM_PERMISSION_MANAGER_HPP
#define RBI_BANKING_SYSTEM_PERMISSION_MANAGER_HPP

#include "auth/RoleTier.hpp"
#include "auth/User.hpp"
#include <cstdint>
#include <vector>

namespace BankingSystem {
namespace Auth {

/**
 * @brief Static permission evaluation manager enforcing 8-tier security and loan approval escalation rules.
 */
class PermissionManager {
public:
    // Loan Approval Threshold Constants in Paise (1 INR = 100 Paise)
    static constexpr int64_t SMALL_LOAN_MAX_PAISE  = 250000000LL;   // ₹25,00,000 (25 Lakhs)
    static constexpr int64_t MEDIUM_LOAN_MAX_PAISE = 1000000000LL;  // ₹1,00,00,000 (1 Crore)

    // Loan Approval Threshold Constants in Rupees (double)
    static constexpr double SMALL_LOAN_MAX_RUPEES  = 2500000.0;    // ₹25 Lakhs
    static constexpr double MEDIUM_LOAN_MAX_RUPEES = 10000000.0;   // ₹1 Crore

    /**
     * @brief Computes the default 64-bit permission bitmask for a given RoleTier.
     */
    static constexpr uint64_t getRolePermissions(RoleTier tier) {
        switch (tier) {
            case RoleTier::RBI_GOVERNOR:
                return static_cast<uint64_t>(Permission::ALL);

            case RoleTier::BANK_CMD:
                return static_cast<uint64_t>(
                    Permission::SET_BANK_POLICIES | Permission::MANAGE_BRANCHES |
                    Permission::MANAGE_STAFF | Permission::VIEW_BANK_ANALYTICS |
                    Permission::CREATE_CUSTOMER_ACCOUNT | Permission::MODIFY_ACCOUNT_STATUS |
                    Permission::ISSUE_CREDIT_CARD | Permission::ISSUE_INSURANCE_POLICY |
                    Permission::PROCESS_INSURANCE_CLAIM | Permission::APPROVE_SMALL_LOAN |
                    Permission::APPROVE_MEDIUM_LOAN | Permission::APPROVE_LARGE_LOAN |
                    Permission::DISBURSE_LOAN | Permission::REJECT_LOAN |
                    Permission::OVERRIDE_DAILY_LIMIT | Permission::VIEW_BRANCH_VAULT |
                    Permission::VIEW_AUDIT_LOGS | Permission::EXPORT_AUDIT_TRAIL
                );

            case RoleTier::ZONAL_MANAGER:
                return static_cast<uint64_t>(
                    Permission::MANAGE_STAFF | Permission::VIEW_BANK_ANALYTICS |
                    Permission::CREATE_CUSTOMER_ACCOUNT | Permission::MODIFY_ACCOUNT_STATUS |
                    Permission::ISSUE_CREDIT_CARD | Permission::ISSUE_INSURANCE_POLICY |
                    Permission::PROCESS_INSURANCE_CLAIM | Permission::APPROVE_SMALL_LOAN |
                    Permission::APPROVE_MEDIUM_LOAN | Permission::DISBURSE_LOAN |
                    Permission::REJECT_LOAN | Permission::OVERRIDE_DAILY_LIMIT |
                    Permission::VIEW_BRANCH_VAULT | Permission::VIEW_AUDIT_LOGS
                );

            case RoleTier::BRANCH_MANAGER:
                return static_cast<uint64_t>(
                    Permission::CREATE_CUSTOMER_ACCOUNT | Permission::MODIFY_ACCOUNT_STATUS |
                    Permission::ISSUE_CREDIT_CARD | Permission::ISSUE_INSURANCE_POLICY |
                    Permission::PROCESS_INSURANCE_CLAIM | Permission::APPROVE_SMALL_LOAN |
                    Permission::DISBURSE_LOAN | Permission::REJECT_LOAN |
                    Permission::PERFORM_CASH_DEPOSIT | Permission::PERFORM_CASH_WITHDRAWAL |
                    Permission::PERFORM_FUNDS_TRANSFER | Permission::OVERRIDE_DAILY_LIMIT |
                    Permission::VIEW_BRANCH_VAULT | Permission::VIEW_AUDIT_LOGS
                );

            case RoleTier::LOAN_OFFICER:
                return static_cast<uint64_t>(
                    Permission::APPLY_LOAN | Permission::REJECT_LOAN |
                    Permission::CREATE_CUSTOMER_ACCOUNT | Permission::VIEW_OWN_PROFILE
                );

            case RoleTier::RELATIONSHIP_MANAGER:
                return static_cast<uint64_t>(
                    Permission::CREATE_CUSTOMER_ACCOUNT | Permission::ISSUE_CREDIT_CARD |
                    Permission::ISSUE_INSURANCE_POLICY | Permission::APPLY_LOAN |
                    Permission::CREATE_TERM_DEPOSIT | Permission::VIEW_OWN_PROFILE
                );

            case RoleTier::TELLER:
                return static_cast<uint64_t>(
                    Permission::PERFORM_CASH_DEPOSIT | Permission::PERFORM_CASH_WITHDRAWAL |
                    Permission::PERFORM_FUNDS_TRANSFER | Permission::CREATE_CUSTOMER_ACCOUNT |
                    Permission::VIEW_OWN_PROFILE
                );

            case RoleTier::CUSTOMER:
                return static_cast<uint64_t>(
                    Permission::VIEW_OWN_PROFILE | Permission::VIEW_OWN_TRANSACTIONS |
                    Permission::SELF_TRANSFER | Permission::CREATE_TERM_DEPOSIT |
                    Permission::BREAK_TERM_DEPOSIT | Permission::APPLY_LOAN
                );

            default:
                return 0ULL;
        }
    }

    /**
     * @brief Evaluates whether a given RoleTier possesses a specific permission.
     */
    static bool hasPermission(RoleTier tier, Permission perm);

    /**
     * @brief Evaluates whether an active user entity possesses a specific permission.
     */
    static bool hasPermission(const User& user, Permission perm);

    /**
     * @brief Checks if an actor role tier has higher or equal authority compared to a target role tier.
     * Note: Lower numerical tier enum value = Higher authority level.
     */
    static bool isHigherOrEqualAuthority(RoleTier actor, RoleTier target);

    /**
     * @brief Evaluates whether a role tier is authorized to approve a loan of the given amount (in Paise).
     */
    static bool canApproveLoanAmount(RoleTier tier, int64_t loanAmountPaise);

    /**
     * @brief Convenience double overload (Rupees) for loan approval evaluation.
     */
    static bool canApproveLoanAmount(RoleTier tier, double loanAmountRupees);

    /**
     * @brief Decodes the bitmask permissions of a role tier into a vector of individual Permission flags.
     */
    static std::vector<Permission> getPermissionListForRole(RoleTier tier);
};

} // namespace Auth
} // namespace BankingSystem

#endif // RBI_BANKING_SYSTEM_PERMISSION_MANAGER_HPP
