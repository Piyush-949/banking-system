#include <iostream>
#include <cassert>
#include <filesystem>
#include "audit/AuditLogger.hpp"

using namespace BankingSystem;

int main() {
    AuditLogger& logger = AuditLogger::getInstance();
    std::string testFile = "data/pipe_inject_test.dat";
    logger.setLogFilePath(testFile);
    logger.clearLogs();

    // Log action with pipe in actorId and bankCode
    logger.logAction("ACTOR|INJECT", RoleTier::RBI_GOVERNOR, "RBI|EVIL", "ACTION|HACK", "TARGET|X", AuditStatus::SUCCESS, "Normal details");
    logger.flushToDisk();
    logger.reloadFromFile();

    auto logs = logger.getAllLogs();
    std::cout << "Logs count after reload: " << logs.size() << std::endl;
    if (!logs.empty()) {
        std::cout << "Actor ID: " << logs[0].actorId << std::endl;
        std::cout << "Bank Code: " << logs[0].bankCode << std::endl;
        std::cout << "Action Type: " << logs[0].actionType << std::endl;
        std::cout << "Target ID: " << logs[0].targetId << std::endl;
        
        bool corrupted = (logs[0].actorId != "ACTOR|INJECT" || logs[0].bankCode != "RBI|EVIL");
        if (corrupted) {
            std::cout << "[FINDING] Pipe injection in actorId/bankCode corrupted deserialization!" << std::endl;
        } else {
            std::cout << "[PASS] Deserialization preserved fields." << std::endl;
        }
    }

    try { std::filesystem::remove(testFile); } catch(...) {}
    return 0;
}
