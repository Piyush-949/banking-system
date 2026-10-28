#include "audit/AuditLogger.hpp"
#include "auth/RoleTier.hpp"
#include "common/DateUtils.hpp"

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <set>
#include <cassert>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace BankingSystem;

void runSingletonStressTest() {
    std::cout << "\n[STRESS TEST 1] Verifying Singleton Multi-Threaded Initialization..." << std::endl;
    constexpr int NUM_THREADS = 16;
    std::vector<std::thread> threads;
    std::atomic<bool> startFlag{false};
    std::vector<AuditLogger*> instances(NUM_THREADS, nullptr);

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([i, &startFlag, &instances]() {
            while (!startFlag.load()) {
                std::this_thread::yield();
            }
            instances[i] = &AuditLogger::getInstance();
        });
    }

    startFlag.store(true);
    for (auto& t : threads) {
        t.join();
    }

    AuditLogger* firstInstance = instances[0];
    assert(firstInstance != nullptr && "Instance should not be null");
    for (int i = 1; i < NUM_THREADS; ++i) {
        assert(instances[i] == firstInstance && "All threads must retrieve identical Singleton instance pointer!");
    }
    std::cout << "  -> PASSED: All " << NUM_THREADS << " threads received identical Singleton instance pointer (" 
              << static_cast<void*>(firstInstance) << ")." << std::endl;
}

void runConcurrentLoggingAndQueriesStressTest() {
    std::cout << "\n[STRESS TEST 2] 16-Thread Concurrent Heavy Write & Read Query Operations..." << std::endl;
    
    AuditLogger& logger = AuditLogger::getInstance();
    std::string testPath = "data/test_stress_audit.dat";
    logger.setLogFilePath(testPath);
    logger.clearLogs();

    constexpr int NUM_THREADS = 16;
    constexpr int WRITER_THREADS = 8;
    constexpr int READER_THREADS = 8;
    constexpr int LOGS_PER_WRITER = 500;
    constexpr int EXPECTED_TOTAL_LOGS = WRITER_THREADS * LOGS_PER_WRITER;

    std::atomic<bool> startFlag{false};
    std::atomic<bool> writersFinished{false};
    std::atomic<uint64_t> totalReadOps{0};

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    // Launch Writer Threads (0 to 7)
    for (int w = 0; w < WRITER_THREADS; ++w) {
        threads.emplace_back([w, &startFlag, &logger]() {
            while (!startFlag.load()) {
                std::this_thread::yield();
            }
            std::string actorId = "WRITER_" + std::to_string(w);
            RoleTier roles[] = { RoleTier::TELLER, RoleTier::BRANCH_MANAGER, RoleTier::ZONAL_MANAGER, RoleTier::BANK_CMD };
            std::string banks[] = { "SBI", "HDFC", "ICICI", "AXIS", "PNB" };
            std::string actions[] = { "DEPOSIT", "WITHDRAW", "LOAN_APPROVE", "FD_CREATE", "POLICY_UPDATE" };

            for (int i = 0; i < LOGS_PER_WRITER; ++i) {
                RoleTier role = roles[i % 4];
                std::string bank = banks[i % 5];
                std::string action = actions[i % 5];
                std::string target = "ACCT_" + std::to_string(w * 1000 + i);
                std::string details = "Stress log entry " + std::to_string(i) + " from thread " + std::to_string(w) + " | pipe_test & newline_test";

                logger.logAction(actorId, role, bank, action, target, AuditStatus::SUCCESS, details);
            }
        });
    }

    // Launch Reader Threads (8 to 15)
    for (int r = 0; r < READER_THREADS; ++r) {
        threads.emplace_back([r, &startFlag, &writersFinished, &totalReadOps, &logger]() {
            while (!startFlag.load()) {
                std::this_thread::yield();
            }
            std::string targetWriterActor = "WRITER_" + std::to_string(r % WRITER_THREADS);
            
            while (!writersFinished.load()) {
                auto logsActor = logger.getLogsByActor(targetWriterActor);
                auto logsBank = logger.getLogsByBank("SBI");
                auto logsRole = logger.getLogsByRole(RoleTier::BRANCH_MANAGER);
                auto logsAction = logger.getLogsByActionType("DEPOSIT");
                
                AuditQueryFilter filter;
                filter.bankCode = "HDFC";
                filter.roleFilter = static_cast<int>(RoleTier::BANK_CMD);
                auto logsFiltered = logger.queryLogs(filter);

                size_t count = logger.getLogCount();
                (void)count;
                (void)logsActor;
                (void)logsBank;
                (void)logsRole;
                (void)logsAction;
                (void)logsFiltered;

                totalReadOps.fetch_add(6, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    startFlag.store(true);

    // Join writer threads first
    for (int i = 0; i < WRITER_THREADS; ++i) {
        threads[i].join();
    }
    writersFinished.store(true);

    // Join reader threads
    for (int i = WRITER_THREADS; i < NUM_THREADS; ++i) {
        threads[i].join();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "  -> Writers and Readers completed in " << durationMs << " ms." << std::endl;
    std::cout << "  -> Total concurrent read query calls executed: " << totalReadOps.load() << std::endl;

    // Data Race & Integrity Checks
    size_t inMemoryCount = logger.getLogCount();
    std::cout << "  -> In-memory log count: " << inMemoryCount << " (Expected: " << EXPECTED_TOTAL_LOGS << ")" << std::endl;
    assert(inMemoryCount == EXPECTED_TOTAL_LOGS && "In-memory log count must exactly match total writes!");

    auto allLogs = logger.getAllLogs();
    assert(allLogs.size() == EXPECTED_TOTAL_LOGS && "getAllLogs() size must match expected total");

    std::set<std::string> uniqueLogIds;
    for (const auto& entry : allLogs) {
        assert(!entry.logId.empty() && "Log ID must not be empty");
        assert(!entry.actorId.empty() && "Actor ID must not be empty");
        assert(!entry.timestamp.empty() && "Timestamp must not be empty");
        uniqueLogIds.insert(entry.logId);
    }

    assert(uniqueLogIds.size() == EXPECTED_TOTAL_LOGS && "All generated Log IDs must be 100% unique!");
    std::cout << "  -> Unique Log IDs verified: " << uniqueLogIds.size() << " / " << EXPECTED_TOTAL_LOGS << std::endl;
    std::cout << "  -> PASSED: Zero data races, zero duplicate log IDs under 16-thread contention." << std::endl;
}

void runPersistenceAndReloadStressTest() {
    std::cout << "\n[STRESS TEST 3] Verifying Disk Persistence Integrity & File Reload..." << std::endl;

    AuditLogger& logger = AuditLogger::getInstance();
    std::string testPath = logger.getLogFilePath();

    // Verify disk file lines
    std::ifstream file(testPath);
    assert(file.is_open() && "Log persistence file must exist on disk!");

    std::string line;
    size_t lineCount = 0;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            lineCount++;
        }
    }
    file.close();

    std::cout << "  -> Disk file line count: " << lineCount << " (Expected: 4000)" << std::endl;
    assert(lineCount == 4000 && "Disk file must contain exactly 4000 serialized log entries!");

    // Test reloadFromFile without calling clearLogs (since clearLogs truncates the file)
    // Create a new logger path for reload testing
    std::string reloadPath = "data/test_stress_reload_direct.dat";
    logger.setLogFilePath(reloadPath);
    logger.clearLogs(); // Prepare empty file

    for (int i = 0; i < 500; ++i) {
        logger.logAction("RELOAD_TEST_ACTOR", RoleTier::CUSTOMER, "SBI", "DEPOSIT", "ACC_RELOAD", "Entry for reload test");
    }

    // Now reload from file
    bool reloaded = logger.reloadFromFile();
    assert(reloaded && "reloadFromFile() failed");
    assert(logger.getLogCount() == 500 && "Reloaded log count must be 500");

    std::cout << "  -> PASSED: Disk persistence and file reloads verified cleanly." << std::endl;
}

void runFlushingAndReloadingConcurrentStressTest() {
    std::cout << "\n[STRESS TEST 4] Persistence Reloading & Concurrent Flush Stress Test..." << std::endl;

    AuditLogger& logger = AuditLogger::getInstance();
    std::string testPath = "data/test_stress_reload.dat";
    logger.setLogFilePath(testPath);
    logger.clearLogs();
    
    // Seed 100 logs
    for (int i = 0; i < 100; ++i) {
        logger.logAction("SEED_USER", RoleTier::CUSTOMER, "SBI", "DEPOSIT", "ACC_1", "Initial seed");
    }

    logger.flushToDisk();
    assert(logger.getLogCount() == 100);

    // Call reloadFromFile
    bool reloadSuccess = logger.reloadFromFile();
    assert(reloadSuccess && "reloadFromFile() must return true");
    assert(logger.getLogCount() == 100 && "Reloaded log count must match saved entries (100)");

    // Test concurrent flushing during active logging
    constexpr int THREADS = 8;
    std::vector<std::thread> workers;
    std::atomic<bool> startFlag{false};

    for (int i = 0; i < THREADS; ++i) {
        workers.emplace_back([i, &startFlag, &logger]() {
            while (!startFlag.load()) std::this_thread::yield();
            for (int j = 0; j < 100; ++j) {
                logger.logAction("CONC_USER", RoleTier::TELLER, "ICICI", "WITHDRAW", "ACC_2", "Concurrent flush entry");
                if (j % 25 == 0) {
                    logger.flushToDisk();
                }
            }
        });
    }

    startFlag.store(true);
    for (auto& w : workers) {
        w.join();
    }

    size_t expectedTotal = 100 + (THREADS * 100);
    assert(logger.getLogCount() == expectedTotal && "Total count after concurrent flush must equal 900!");

    // Verify disk file match
    std::ifstream file(testPath);
    size_t diskLines = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) diskLines++;
    }
    file.close();

    std::cout << "  -> In-memory count: " << logger.getLogCount() << ", Disk count: " << diskLines << std::endl;
    assert(diskLines == expectedTotal && "Disk count must match in-memory count after concurrent flushes!");

    std::cout << "  -> PASSED: Clean persistence and reload verification complete." << std::endl;
}

int main() {
    std::cout << "====================================================" << std::endl;
    std::cout << "  AuditLogger Concurrency & Stress Verification Suite" << std::endl;
    std::cout << "====================================================" << std::endl;

    try {
        runSingletonStressTest();
        runConcurrentLoggingAndQueriesStressTest();
        runPersistenceAndReloadStressTest();
        runFlushingAndReloadingConcurrentStressTest();

        std::cout << "\n====================================================" << std::endl;
        std::cout << "  ALL AUDIT LOGGER CONCURRENCY STRESS TESTS PASSED! " << std::endl;
        std::cout << "====================================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[TEST FAILURE] Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n[TEST FAILURE] Unknown exception caught!" << std::endl;
        return 1;
    }
}
