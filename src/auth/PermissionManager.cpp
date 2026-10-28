#include "auth/PermissionManager.hpp"
#include <cmath>

namespace BankingSystem {
namespace Auth {

bool PermissionManager::hasPermission(RoleTier tier, Permission perm) {
    uint64_t mask = getRolePermissions(tier);
    uint64_t p = static_cast<uint64_t>(perm);
    return (mask & p) == p;
}

bool PermissionManager::hasPermission(const User& user, Permission perm) {
    if (!user.isActive) return false;
    return hasPermission(user.roleTier, perm);
}

bool PermissionManager::isHigherOrEqualAuthority(RoleTier actor, RoleTier target) {
    return static_cast<uint8_t>(actor) <= static_cast<uint8_t>(target);
}

bool PermissionManager::canApproveLoanAmount(RoleTier tier, int64_t loanAmountPaise) {
    if (loanAmountPaise <= 0) return false;

    if (loanAmountPaise <= SMALL_LOAN_MAX_PAISE) {
        // <= ₹25L: Tier 3 (Branch Manager) or higher
        return isHigherOrEqualAuthority(tier, RoleTier::BRANCH_MANAGER);
    } else if (loanAmountPaise <= MEDIUM_LOAN_MAX_PAISE) {
        // ₹25L–₹1Cr: Tier 2 (Zonal Manager) or higher
        return isHigherOrEqualAuthority(tier, RoleTier::ZONAL_MANAGER);
    } else {
        // > ₹1Cr: Tier 1 (CMD) or higher (Governor)
        return isHigherOrEqualAuthority(tier, RoleTier::BANK_CMD);
    }
}

bool PermissionManager::canApproveLoanAmount(RoleTier tier, double loanAmountRupees) {
    int64_t paise = static_cast<int64_t>(std::round(loanAmountRupees * 100.0));
    return canApproveLoanAmount(tier, paise);
}

std::vector<Permission> PermissionManager::getPermissionListForRole(RoleTier tier) {
    uint64_t mask = getRolePermissions(tier);
    std::vector<Permission> list;
    for (int i = 0; i < 64; ++i) {
        uint64_t bit = 1ULL << i;
        if (mask & bit) {
            list.push_back(static_cast<Permission>(bit));
        }
    }
    return list;
}

} // namespace Auth
} // namespace BankingSystem
