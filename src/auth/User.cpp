#include "auth/User.hpp"
#include "common/SafeSerialization.hpp"
#include <sstream>
#include <vector>

namespace BankingSystem {
namespace Auth {

std::string User::serialize() const {
    std::stringstream ss;
    ss << RecordFormat::escape(userId) << "|"
       << RecordFormat::escape(username) << "|"
       << RecordFormat::escape(passwordHash) << "|"
       << static_cast<int>(roleTier) << "|"
       << RecordFormat::escape(bankCode) << "|"
       << RecordFormat::escape(branchCode) << "|"
       << (isActive ? 1 : 0) << "|"
       << createdAtTimestamp;
    return ss.str();
}

User User::deserialize(const std::string& line) {
    User u;
    if (line.empty()) return u;

    std::vector<std::string> tokens;
    if (RecordFormat::split(line, tokens) && tokens.size() == 8) {
        u.userId = tokens[0];
        u.username = tokens[1];
        u.passwordHash = tokens[2];
        try {
            const int role = std::stoi(tokens[3]);
            if (role < static_cast<int>(RoleTier::RBI_GOVERNOR) || role > static_cast<int>(RoleTier::PUBLIC_GUEST)) return User{};
            u.roleTier = static_cast<RoleTier>(role);
        } catch (...) {
            u.roleTier = RoleTier::CUSTOMER;
        }
        u.bankCode = tokens[4];
        u.branchCode = tokens[5];
        u.isActive = (tokens[6] == "1");
        try {
            u.createdAtTimestamp = std::stoll(tokens[7]);
        } catch (...) {
            u.createdAtTimestamp = 0;
        }
    }
    return u;
}

} // namespace Auth
} // namespace BankingSystem
