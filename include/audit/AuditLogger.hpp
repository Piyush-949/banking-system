#ifndef BANKING_SYSTEM_AUDIT_LOGGER_HPP
#define BANKING_SYSTEM_AUDIT_LOGGER_HPP

#include "common/Types.hpp"
#include "common/DateUtils.hpp"
#include "common/SafeSerialization.hpp"
#include "auth/RoleTier.hpp"

#include <string>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <memory>

namespace BankingSystem {

enum class AuditStatus : uint8_t {
    SUCCESS   = 0,
    FAILED    = 1,
    PENDING   = 2,
    ESCALATED = 3,
    REJECTED  = 4
};

inline std::string auditStatusToString(AuditStatus s) {
    switch (s) {
        case AuditStatus::SUCCESS:   return "SUCCESS";
        case AuditStatus::FAILED:    return "FAILED";
        case AuditStatus::PENDING:   return "PENDING";
        case AuditStatus::ESCALATED: return "ESCALATED";
        case AuditStatus::REJECTED:  return "REJECTED";
        default:                     return "UNKNOWN";
    }
}

inline AuditStatus stringToAuditStatus(const std::string& str) {
    if (str == "SUCCESS" || str == "0") return AuditStatus::SUCCESS;
    if (str == "FAILED" || str == "1") return AuditStatus::FAILED;
    if (str == "PENDING" || str == "2") return AuditStatus::PENDING;
    if (str == "ESCALATED" || str == "3") return AuditStatus::ESCALATED;
    if (str == "REJECTED" || str == "4") return AuditStatus::REJECTED;
    return AuditStatus::SUCCESS;
}

struct AuditEntry {
    std::string logId;        // Unique identifier (e.g., "LOG_10001")
    std::string timestamp;    // ISO-8601 formatted timestamp ("YYYY-MM-DD HH:MM:SS")
    std::string actorId;      // ID of user performing action ("GOV001", "BM_SBI_01", "CUST101")
    RoleTier actorRole;       // 8-Tier Role enum
    std::string bankCode;     // Bank code ("RBI", "SBI", "HDFC", "ICICI", "AXIS", "PNB")
    std::string actionType;   // Operation type ("REPO_RATE_CHANGE", "LOAN_APPROVE", "DEPOSIT", "WITHDRAW")
    std::string targetId;     // Target entity ID ("ACCOUNT_1001", "LOAN_5001", "BANK_SBI")
    AuditStatus status;       // SUCCESS, FAILED, PENDING, ESCALATED, REJECTED
    std::string details;      // Contextual description

    // Escaped, pipe-delimited serialization retained for backward-compatible files.
    std::string serialize() const {
        std::stringstream ss;
        ss << RecordFormat::escape(logId) << "|"
           << RecordFormat::escape(timestamp) << "|"
           << RecordFormat::escape(actorId) << "|"
           << static_cast<int>(actorRole) << "|"
           << RecordFormat::escape(bankCode) << "|"
           << RecordFormat::escape(actionType) << "|"
           << RecordFormat::escape(targetId) << "|"
           << auditStatusToString(status) << "|"
           << RecordFormat::escape(details);
        return ss.str();
    }

    static AuditEntry deserialize(const std::string& line) {
        AuditEntry entry;
        if (line.empty()) return entry;

        std::vector<std::string> tokens;
        if (RecordFormat::split(line, tokens) && tokens.size() == 9) {
            entry.logId = tokens[0];
            entry.timestamp = tokens[1];
            entry.actorId = tokens[2];
            try {
            const int value = std::stoi(tokens[3]);
            if (value < static_cast<int>(RoleTier::RBI_GOVERNOR) || value > static_cast<int>(RoleTier::PUBLIC_GUEST)) return AuditEntry{};
            entry.actorRole = static_cast<RoleTier>(value);
            } catch (...) {
                entry.actorRole = RoleTier::CUSTOMER;
            }
            entry.bankCode = tokens[4];
            entry.actionType = tokens[5];
            entry.targetId = tokens[6];
            const AuditStatus parsedStatus = stringToAuditStatus(tokens[7]);
            if (auditStatusToString(parsedStatus) != tokens[7] && tokens[7].size() != 1) return AuditEntry{};
            entry.status = parsedStatus;
            entry.details = tokens[8];
        }
        return entry;
    }
};

// Filter struct for composite queries
struct AuditQueryFilter {
    std::string actorId;
    std::string targetId;
    std::string bankCode;
    std::string startDate; // "YYYY-MM-DD" or "YYYY-MM-DD HH:MM:SS"
    std::string endDate;   // "YYYY-MM-DD" or "YYYY-MM-DD HH:MM:SS"
    std::string actionType;
    int roleFilter{-1};    // -1 = any role, otherwise static_cast<int>(RoleTier)
    int statusFilter{-1};  // -1 = any status
};

class AuditLogger {
public:
    // C++17 Meyers' Singleton instance
    static AuditLogger& getInstance();

    // Prevent copies and moves
    AuditLogger(const AuditLogger&) = delete;
    AuditLogger& operator=(const AuditLogger&) = delete;
    AuditLogger(AuditLogger&&) = delete;
    AuditLogger& operator=(AuditLogger&&) = delete;

    // Primary logging interfaces
    std::string logAction(
        const std::string& actorId,
        RoleTier role,
        const std::string& bankCode,
        const std::string& actionType,
        const std::string& targetId,
        AuditStatus status,
        const std::string& details
    );

    std::string logAction(
        const std::string& actorId,
        RoleTier role,
        const std::string& bankCode,
        const std::string& actionType,
        const std::string& targetId,
        const std::string& statusStr,
        const std::string& details
    );

    std::string logAction(
        const std::string& actorId,
        RoleTier role,
        const std::string& bankCode,
        const std::string& actionType,
        const std::string& targetId,
        const std::string& details
    );

    // Filtered Query Interfaces (Thread-Safe Shared Readers)
    std::vector<AuditEntry> getLogsByActor(const std::string& actorId) const;
    std::vector<AuditEntry> getLogsByTarget(const std::string& targetId) const;
    std::vector<AuditEntry> getLogsByBank(const std::string& bankCode) const;
    std::vector<AuditEntry> getLogsByDateRange(const std::string& startDate, const std::string& endDate) const;
    std::vector<AuditEntry> getLogsByRole(RoleTier role) const;
    std::vector<AuditEntry> getLogsByActionType(const std::string& actionType) const;
    std::vector<AuditEntry> queryLogs(const AuditQueryFilter& filter) const;
    std::vector<AuditEntry> getAllLogs() const;

    // Buffer Persistence Management
    bool flushToDisk();
    bool reloadFromFile();
    void setLogFilePath(const std::string& path);
    std::string getLogFilePath() const;
    size_t getLogCount() const;
    void clearLogs();

    // Reporting & Display Output
    void printAuditReport(const std::vector<AuditEntry>& entries, std::ostream& out = std::cout) const;

private:
    AuditLogger();
    ~AuditLogger();

    void initializeStorage();
    std::string generateLogId();
    std::string getCurrentTimestampISO() const;

    mutable std::shared_mutex m_mutex;
    std::vector<AuditEntry> m_buffer;
    std::string m_filePath{"data/audit_logs.dat"};
    uint64_t m_sequenceCounter{10001};
};

} // namespace BankingSystem

#endif // BANKING_SYSTEM_AUDIT_LOGGER_HPP
