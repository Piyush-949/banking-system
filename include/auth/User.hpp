#ifndef RBI_BANKING_SYSTEM_USER_HPP
#define RBI_BANKING_SYSTEM_USER_HPP

#include "auth/RoleTier.hpp"
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>

namespace BankingSystem {
namespace Auth {

/**
 * @brief User entity struct encapsulating authentication credentials, role tier, and organizational scope.
 */
struct User {
    std::string userId;
    std::string username;
    std::string passwordHash;
    RoleTier roleTier = RoleTier::CUSTOMER;
    std::string bankCode;   // "RBI", "SBI", "HDFC", "ICICI", "AXIS", "PNB"
    std::string branchCode; // "HO", "MUM01", etc.
    bool isActive = true;
    int64_t createdAtTimestamp = 0;

    User() = default;

    User(std::string id, std::string name, std::string hash, RoleTier tier,
         std::string bank, std::string branch = "", bool active = true, int64_t timestamp = 0)
        : userId(std::move(id)), username(std::move(name)), passwordHash(std::move(hash)),
          roleTier(tier), bankCode(std::move(bank)), branchCode(std::move(branch)),
          isActive(active), createdAtTimestamp(timestamp) {}

    // Helper Predicates
    bool isRBIOfficial() const {
        return bankCode == "RBI" || roleTier == RoleTier::RBI_GOVERNOR;
    }

    bool isBankStaff() const {
        return roleTier >= RoleTier::BANK_CMD && roleTier <= RoleTier::TELLER;
    }

    bool isCustomer() const {
        return roleTier == RoleTier::CUSTOMER;
    }

    std::string getRoleString() const {
        return roleTierToString(roleTier);
    }

    std::string getFullScopedIdentifier() const {
        std::stringstream ss;
        ss << bankCode;
        if (!branchCode.empty()) ss << ":" << branchCode;
        ss << ":" << userId;
        return ss.str();
    }

    // Pipe-delimited Serialization Format:
    // userId|username|passwordHash|roleTierInt|bankCode|branchCode|isActiveInt|createdAtTimestamp
    std::string serialize() const;
    static User deserialize(const std::string& line);
};

} // namespace Auth
} // namespace BankingSystem

#endif // RBI_BANKING_SYSTEM_USER_HPP
