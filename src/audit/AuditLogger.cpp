#include "audit/AuditLogger.hpp"
#include "common/TableFormatter.hpp"

#include <filesystem>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iomanip>

namespace BankingSystem {

namespace {
std::string redactSensitiveDetails(const std::string& details) {
    std::string lower = details;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower.find("password") != std::string::npos || lower.find("cvv") != std::string::npos || lower.find("otp") != std::string::npos) {
        return "[REDACTED: sensitive detail omitted]";
    }
    return details;
}
} // namespace

AuditLogger& AuditLogger::getInstance() {
    static AuditLogger instance;
    return instance;
}

AuditLogger::AuditLogger() {
    initializeStorage();
}

AuditLogger::~AuditLogger() {
    // Flush buffered items before shutdown
    flushToDisk();
}

void AuditLogger::initializeStorage() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_buffer.clear();

    try {
        std::filesystem::path path(m_filePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
    } catch (...) {}

    std::ifstream file(m_filePath);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    uint64_t maxSeq = 10000;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        AuditEntry entry = AuditEntry::deserialize(line);
        if (!entry.logId.empty()) {
            m_buffer.push_back(entry);
            if (entry.logId.rfind("LOG_", 0) == 0) {
                try {
                    uint64_t seq = std::stoull(entry.logId.substr(4));
                    if (seq > maxSeq) maxSeq = seq;
                } catch (...) {}
            }
        }
    }
    m_sequenceCounter = maxSeq + 1;
}

std::string AuditLogger::generateLogId() {
    // Note: Called with lock held
    std::stringstream ss;
    ss << "LOG_" << m_sequenceCounter++;
    return ss.str();
}

std::string AuditLogger::getCurrentTimestampISO() const {
    return DateUtils::nowISO8601();
}

std::string AuditLogger::logAction(
    const std::string& actorId,
    RoleTier role,
    const std::string& bankCode,
    const std::string& actionType,
    const std::string& targetId,
    AuditStatus status,
    const std::string& details
) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    AuditEntry entry;
    entry.logId = generateLogId();
    entry.timestamp = getCurrentTimestampISO();
    entry.actorId = actorId.empty() ? "SYSTEM" : actorId;
    entry.actorRole = role;
    entry.bankCode = bankCode.empty() ? "GENERAL" : bankCode;
    entry.actionType = actionType;
    entry.targetId = targetId.empty() ? "NONE" : targetId;
    entry.status = status;
    entry.details = redactSensitiveDetails(details);
    // Free-text details must never carry record separators or line breaks
    // into the pipe-delimited audit log, even in memory.
    for (char &dc : entry.details)
    {
        if (dc == '|' || dc == '\n' || dc == '\r')
            dc = ' ';
    }

    m_buffer.push_back(entry);

    // Immediate append to disk for persistence
    try {
        std::ofstream file(m_filePath, std::ios::app);
        if (file.is_open()) {
            file << entry.serialize() << "\n";
            file.flush();
        }
    } catch (...) {}

    return entry.logId;
}

std::string AuditLogger::logAction(
    const std::string& actorId,
    RoleTier role,
    const std::string& bankCode,
    const std::string& actionType,
    const std::string& targetId,
    const std::string& statusStr,
    const std::string& details
) {
    return logAction(actorId, role, bankCode, actionType, targetId, stringToAuditStatus(statusStr), details);
}

std::string AuditLogger::logAction(
    const std::string& actorId,
    RoleTier role,
    const std::string& bankCode,
    const std::string& actionType,
    const std::string& targetId,
    const std::string& details
) {
    return logAction(actorId, role, bankCode, actionType, targetId, AuditStatus::SUCCESS, details);
}

std::vector<AuditEntry> AuditLogger::getLogsByActor(const std::string& actorId) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<AuditEntry> result;
    for (const auto& entry : m_buffer) {
        if (entry.actorId == actorId) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<AuditEntry> AuditLogger::getLogsByTarget(const std::string& targetId) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<AuditEntry> result;
    for (const auto& entry : m_buffer) {
        if (entry.targetId == targetId) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<AuditEntry> AuditLogger::getLogsByBank(const std::string& bankCode) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<AuditEntry> result;
    for (const auto& entry : m_buffer) {
        if (entry.bankCode == bankCode) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<AuditEntry> AuditLogger::getLogsByDateRange(
    const std::string& startDate, 
    const std::string& endDate
) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<AuditEntry> result;

    std::string start = startDate;
    if (!start.empty() && start.length() == 10) start += " 00:00:00";

    std::string end = endDate;
    if (!end.empty() && end.length() == 10) end += " 23:59:59";

    for (const auto& entry : m_buffer) {
        if ((start.empty() || entry.timestamp >= start) &&
            (end.empty() || entry.timestamp <= end)) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<AuditEntry> AuditLogger::getLogsByRole(RoleTier role) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<AuditEntry> result;
    for (const auto& entry : m_buffer) {
        if (entry.actorRole == role) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<AuditEntry> AuditLogger::getLogsByActionType(const std::string& actionType) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<AuditEntry> result;
    for (const auto& entry : m_buffer) {
        if (entry.actionType == actionType) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<AuditEntry> AuditLogger::queryLogs(const AuditQueryFilter& filter) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<AuditEntry> result;

    std::string start = filter.startDate;
    if (!start.empty() && start.length() == 10) start += " 00:00:00";
    std::string end = filter.endDate;
    if (!end.empty() && end.length() == 10) end += " 23:59:59";

    for (const auto& entry : m_buffer) {
        if (!filter.actorId.empty() && entry.actorId != filter.actorId) continue;
        if (!filter.targetId.empty() && entry.targetId != filter.targetId) continue;
        if (!filter.bankCode.empty() && entry.bankCode != filter.bankCode) continue;
        if (!filter.actionType.empty() && entry.actionType != filter.actionType) continue;
        if (filter.roleFilter >= 0 && static_cast<int>(entry.actorRole) != filter.roleFilter) continue;
        if (filter.statusFilter >= 0 && static_cast<int>(entry.status) != filter.statusFilter) continue;
        if (!start.empty() && entry.timestamp < start) continue;
        if (!end.empty() && entry.timestamp > end) continue;

        result.push_back(entry);
    }
    return result;
}

std::vector<AuditEntry> AuditLogger::getAllLogs() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_buffer;
}

bool AuditLogger::flushToDisk() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    try {
        const std::string tempPath = m_filePath + ".tmp";
        std::ofstream file(tempPath, std::ios::trunc | std::ios::binary);
        if (!file.is_open()) return false;
        for (const auto& entry : m_buffer) {
            file << entry.serialize() << "\n";
        }
        file.flush();
        if (!file.good()) return false;
        file.close();
        std::error_code ec;
        std::filesystem::remove(m_filePath, ec);
        ec.clear();
        std::filesystem::rename(tempPath, m_filePath, ec);
        return !ec;
    } catch (...) {
        return false;
    }
}

bool AuditLogger::reloadFromFile() {
    initializeStorage();
    return true;
}

void AuditLogger::setLogFilePath(const std::string& path) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_filePath = path;
}

std::string AuditLogger::getLogFilePath() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_filePath;
}

size_t AuditLogger::getLogCount() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_buffer.size();
}

void AuditLogger::clearLogs() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_buffer.clear();
    std::ofstream file(m_filePath, std::ios::trunc);
}

void AuditLogger::printAuditReport(const std::vector<AuditEntry>& entries, std::ostream& out) const {
    TableFormatter table("AUDIT LOG SYSTEM TRAIL");
    table.addColumn("Log ID", 10, TableFormatter::Alignment::LEFT);
    table.addColumn("Timestamp", 19, TableFormatter::Alignment::LEFT);
    table.addColumn("Actor ID", 12, TableFormatter::Alignment::LEFT);
    table.addColumn("Role", 16, TableFormatter::Alignment::LEFT);
    table.addColumn("Bank", 8, TableFormatter::Alignment::LEFT);
    table.addColumn("Action", 20, TableFormatter::Alignment::LEFT);
    table.addColumn("Target ID", 12, TableFormatter::Alignment::LEFT);
    table.addColumn("Status", 10, TableFormatter::Alignment::LEFT);
    table.addColumn("Details", 30, TableFormatter::Alignment::LEFT);

    for (const auto& entry : entries) {
        table.addRow({
            entry.logId,
            entry.timestamp,
            entry.actorId,
            roleTierToString(entry.actorRole),
            entry.bankCode,
            entry.actionType,
            entry.targetId,
            auditStatusToString(entry.status),
            entry.details
        });
    }

    table.print(out);
}

} // namespace BankingSystem
