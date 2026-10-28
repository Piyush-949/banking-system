#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <limits>
#include <fstream>
#include <filesystem>

#include "common/Types.hpp"
#include "common/Money.hpp"
#include "common/DateUtils.hpp"
#include "common/TableFormatter.hpp"
#include "auth/RoleTier.hpp"
#include "auth/User.hpp"
#include "auth/PermissionManager.hpp"
#include "audit/AuditLogger.hpp"

using namespace BankingSystem;
using namespace BankingSystem::Auth;

int g_passCount = 0;
int g_failCount = 0;

#define STRESS_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            g_passCount++; \
        } else { \
            g_failCount++; \
            std::cerr << "  [FAIL] Line " << __LINE__ << ": " << msg << std::endl; \
        } \
    } while(0)

void stressTestMoney() {
    std::cout << "[STRESS] 1. Money Edge Cases & Adversarial Parsing..." << std::endl;

    // 1.1 Parsing edge cases
    Money p1 = Money::parse(".99");
    STRESS_ASSERT(p1.getPaise() == 99, "Parse .99 failed");

    Money p2 = Money::parse("100.");
    STRESS_ASSERT(p2.getPaise() == 10000, "Parse 100. failed");

    Money p3 = Money::parse("  -₹  1,23,456.78  ");
    STRESS_ASSERT(p3.getPaise() == -12345678LL, "Parse negative formatted rupees failed");

    Money p4 = Money::parse("-0.50");
    STRESS_ASSERT(p4.getPaise() == -50, "Parse -0.50 failed");

    Money p5 = Money::parse("0");
    STRESS_ASSERT(p5.getPaise() == 0, "Parse 0 failed");

    Money p6 = Money::parse("");
    STRESS_ASSERT(p6.getPaise() == 0, "Parse empty string failed");

    Money p7 = Money::parse("abc");
    STRESS_ASSERT(p7.getPaise() == 0, "Parse non-numeric string failed");

    // 1.2 stoll exception safety on overflow input
    bool stollExceptionCaught = false;
    try {
        Money p8 = Money::parse("999999999999999999999999999999999999");
        (void)p8;
    } catch (const std::out_of_range&) {
        stollExceptionCaught = true;
    } catch (...) {
        stollExceptionCaught = true;
    }
    STRESS_ASSERT(stollExceptionCaught, "Money::parse stoll overflow did not catch exception");

    // 1.3 Formatting boundary tests
    Money zero = Money::fromRupees(0.0);
    STRESS_ASSERT(zero.format() == "₹0.00", "Zero formatting failed");
    STRESS_ASSERT(zero.formatPlain() == "0.00", "Zero plain formatting failed");

    Money negCent = Money::fromPaise(-5);
    STRESS_ASSERT(negCent.format() == "-₹0.05", "Negative paise formatting failed");
    STRESS_ASSERT(negCent.formatPlain() == "-0.05", "Negative paise plain formatting failed");

    Money largeMoney = Money::fromRupees(9876543210.99); // ₹9,87,65,43,210.99
    STRESS_ASSERT(largeMoney.format() == "₹9,87,65,43,210.99", "Large Indian currency formatting failed: " + largeMoney.format());

    // 1.4 Division and Modulo by Zero exceptions
    bool divZeroDoubleHandled = false;
    try {
        Money m = Money::fromRupees(100.0) / 0.0;
        (void)m;
    } catch (const std::invalid_argument&) {
        divZeroDoubleHandled = true;
    }
    STRESS_ASSERT(divZeroDoubleHandled, "Division by 0.0 double didn't throw invalid_argument");

    bool divZeroMoneyHandled = false;
    try {
        int64_t ratio = Money::fromRupees(100.0) / Money::fromRupees(0.0);
        (void)ratio;
    } catch (const std::invalid_argument&) {
        divZeroMoneyHandled = true;
    }
    STRESS_ASSERT(divZeroMoneyHandled, "Division by zero Money didn't throw invalid_argument");

    bool modZeroHandled = false;
    try {
        Money rem = Money::fromRupees(100.0) % 0;
        (void)rem;
    } catch (const std::invalid_argument&) {
        modZeroHandled = true;
    }
    STRESS_ASSERT(modZeroHandled, "Modulo by 0 didn't throw invalid_argument");

    // 1.5 Arithmetic operations
    Money a(1000LL); // ₹10.00
    Money b(500LL);  // ₹5.00
    STRESS_ASSERT((a + b).getPaise() == 1500, "Money addition failed");
    STRESS_ASSERT((a - b).getPaise() == 500, "Money subtraction failed");
    STRESS_ASSERT((a * 2.5).getPaise() == 2500, "Money multiplication failed");
    STRESS_ASSERT((a / 2.0).getPaise() == 500, "Money division failed");
    STRESS_ASSERT((a % 3).getPaise() == 1, "Money modulo failed");
    STRESS_ASSERT((-a).getPaise() == -1000, "Money unary minus failed");
}

void stressTestDateUtils() {
    std::cout << "[STRESS] 2. DateUtils Leap Year, Boundary & Negative Arithmetic..." << std::endl;

    // 2.1 Leap Year addition (2024 is leap year: Feb 29 + 1 Year -> Feb 28, 2025)
    std::string leapDate = "2024-02-29";
    std::string nextYearLeap = DateUtils::addYears(leapDate, 1);
    STRESS_ASSERT(nextYearLeap == "2025-02-28", "Leap year Feb 29 + 1 yr failed: expected 2025-02-28, got " + nextYearLeap);

    // 2.2 Negative month arithmetic (Jan 2026 - 5 months -> Aug 2025)
    std::string startJan = "2026-01-15";
    std::string minus5Mon = DateUtils::addMonths(startJan, -5);
    STRESS_ASSERT(minus5Mon == "2025-08-15", "Jan 2026 - 5 months failed: expected 2025-08-15, got " + minus5Mon);

    // 2.3 Month end truncation (Mar 31 - 1 month -> Feb 28 in non-leap year 2026)
    std::string mar31 = "2026-03-31";
    std::string feb28 = DateUtils::addMonths(mar31, -1);
    STRESS_ASSERT(feb28 == "2026-02-28", "Mar 31 - 1 month failed: expected 2026-02-28, got " + feb28);

    // 2.4 Negative day arithmetic (Jan 5, 2026 - 10 days -> Dec 26, 2025)
    std::string jan5 = "2026-01-05";
    std::string dec26 = DateUtils::addDays(jan5, -10);
    STRESS_ASSERT(dec26 == "2025-12-26", "Jan 5 - 10 days failed: expected 2025-12-26, got " + dec26);

    // 2.5 Elapsed days calculation with reverse order
    int64_t elapsedPositive = DateUtils::calculateElapsedDays("2026-01-01", "2026-01-11");
    STRESS_ASSERT(elapsedPositive == 10, "Elapsed days positive failed");

    int64_t elapsedNegative = DateUtils::calculateElapsedDays("2026-01-11", "2026-01-01");
    STRESS_ASSERT(elapsedNegative == -10, "Elapsed days negative failed");

    // 2.6 ISO8601 Validation tests
    STRESS_ASSERT(DateUtils::isValidISO8601("2026-08-09"), "Valid date check failed");
    STRESS_ASSERT(DateUtils::isValidISO8601("2026-08-09 16:30:00"), "Valid datetime check failed");
    STRESS_ASSERT(!DateUtils::isValidISO8601(""), "Empty date validation failed");
    STRESS_ASSERT(!DateUtils::isValidISO8601("invalid-date-string"), "Invalid string date check failed");
}

void stressTestTableFormatter() {
    std::cout << "[STRESS] 3. TableFormatter Alignment, UTF-8 & Layout Hardening..." << std::endl;

    TableFormatter table("STRESS TEST TABLE WITH LONG TITLE FOR OVERFLOW CHECKING");
    table.addColumn("Col 1", 10, TableFormatter::Alignment::LEFT);
    table.addColumn("Col 2", 15, TableFormatter::Alignment::CENTER);
    table.addColumn("Col 3", 12, TableFormatter::Alignment::RIGHT);

    // Row with exact match
    table.addRow({"Data 1", "Center Data", "100.00"});
    // Row with shorter data
    table.addRow({"A", "B", "C"});
    // Row with Money formatted string containing ₹ symbol (UTF-8)
    Money m = Money::fromRupees(1234567.89);
    table.addRow({"Balance", "Current", m.format()});
    // Row with excess cells (4 cells when only 3 columns defined)
    table.addRow({"Cell 1", "Cell 2", "Cell 3", "Cell 4 IGNORED"});

    std::string rendered = table.render();
    STRESS_ASSERT(!rendered.empty(), "Table rendering failed");
    STRESS_ASSERT(rendered.find("Data 1") != std::string::npos, "Cell content missing");
    STRESS_ASSERT(rendered.find(m.format()) != std::string::npos, "Money formatted string missing");

    // Test clear
    table.clear();
    STRESS_ASSERT(table.render().empty(), "Cleared table rendering should be empty");
}

void stressTestPermissionManager() {
    std::cout << "[STRESS] 4. PermissionManager Loan Threshold & 8-Tier Authorization..." << std::endl;

    // 4.1 Boundary Testing for Loan Approvals
    // Exact ₹25,00,000 (25 Lakhs)
    STRESS_ASSERT(PermissionManager::canApproveLoanAmount(RoleTier::BRANCH_MANAGER, 2500000.0), "BM should approve exact 25L");
    STRESS_ASSERT(PermissionManager::canApproveLoanAmount(RoleTier::ZONAL_MANAGER, 2500000.0), "ZM should approve exact 25L");
    STRESS_ASSERT(PermissionManager::canApproveLoanAmount(RoleTier::BANK_CMD, 2500000.0), "CMD should approve exact 25L");
    STRESS_ASSERT(!PermissionManager::canApproveLoanAmount(RoleTier::LOAN_OFFICER, 2500000.0), "LO should NOT approve 25L");

    // ₹25,00,000.01 (25 Lakhs + 1 paisa) -> Escalated to Zonal Manager
    STRESS_ASSERT(!PermissionManager::canApproveLoanAmount(RoleTier::BRANCH_MANAGER, 2500000.01), "BM should NOT approve 25L + 1p");
    STRESS_ASSERT(PermissionManager::canApproveLoanAmount(RoleTier::ZONAL_MANAGER, 2500000.01), "ZM should approve 25L + 1p");
    STRESS_ASSERT(PermissionManager::canApproveLoanAmount(RoleTier::BANK_CMD, 2500000.01), "CMD should approve 25L + 1p");

    // Exact ₹1,00,00,000 (1 Crore) -> Zonal Manager can approve
    STRESS_ASSERT(!PermissionManager::canApproveLoanAmount(RoleTier::BRANCH_MANAGER, 10000000.0), "BM should NOT approve 1Cr");
    STRESS_ASSERT(PermissionManager::canApproveLoanAmount(RoleTier::ZONAL_MANAGER, 10000000.0), "ZM should approve exact 1Cr");
    STRESS_ASSERT(PermissionManager::canApproveLoanAmount(RoleTier::BANK_CMD, 10000000.0), "CMD should approve exact 1Cr");

    // ₹1,00,00,000.01 (1 Crore + 1 paisa) -> Escalated to CMD
    STRESS_ASSERT(!PermissionManager::canApproveLoanAmount(RoleTier::BRANCH_MANAGER, 10000000.01), "BM should NOT approve >1Cr");
    STRESS_ASSERT(!PermissionManager::canApproveLoanAmount(RoleTier::ZONAL_MANAGER, 10000000.01), "ZM should NOT approve >1Cr");
    STRESS_ASSERT(PermissionManager::canApproveLoanAmount(RoleTier::BANK_CMD, 10000000.01), "CMD should approve >1Cr");
    STRESS_ASSERT(PermissionManager::canApproveLoanAmount(RoleTier::RBI_GOVERNOR, 10000000.01), "Governor should approve >1Cr");

    // Negative loan amount
    STRESS_ASSERT(!PermissionManager::canApproveLoanAmount(RoleTier::BANK_CMD, -100.0), "Negative loan amount approval should fail");
    STRESS_ASSERT(!PermissionManager::canApproveLoanAmount(RoleTier::RBI_GOVERNOR, 0.0), "Zero loan amount approval should fail");

    // 4.2 Inactive User checks
    User activeUser("U01", "Alice", "pass", RoleTier::BRANCH_MANAGER, "SBI", "BR01", true, 0);
    User inactiveUser("U02", "Bob", "pass", RoleTier::BRANCH_MANAGER, "SBI", "BR01", false, 0);

    STRESS_ASSERT(PermissionManager::hasPermission(activeUser, Permission::APPROVE_SMALL_LOAN), "Active BM should have small loan permission");
    STRESS_ASSERT(!PermissionManager::hasPermission(inactiveUser, Permission::APPROVE_SMALL_LOAN), "Inactive BM must NOT have permissions");

    // 4.3 Permission decoding vector length test
    auto govPerms = PermissionManager::getPermissionListForRole(RoleTier::RBI_GOVERNOR);
    STRESS_ASSERT(govPerms.size() == 64, "Governor permission list should contain all 64 flags");

    auto custPerms = PermissionManager::getPermissionListForRole(RoleTier::CUSTOMER);
    STRESS_ASSERT(custPerms.size() == 6, "Customer permission list count mismatch");
}

void stressTestAuditLoggerConcurrencyAndInjection() {
    std::cout << "[STRESS] 5. AuditLogger Multi-Threaded Stress & Pipe Injection Hardening..." << std::endl;

    AuditLogger& logger = AuditLogger::getInstance();
    std::string testPath = "data/stress_test_audit_logs.dat";
    logger.setLogFilePath(testPath);
    logger.clearLogs();

    // 5.1 Pipe & Newline Injection in fields
    std::string injectedActor = "ACTOR|EVIL\nROLE";
    std::string injectedBank = "SBI|INJECTED";
    std::string injectedAction = "ACTION|HACK";
    std::string injectedTarget = "ACC|999";
    std::string injectedDetails = "Details with | pipe and \n newline";

    std::string logId = logger.logAction(
        injectedActor, RoleTier::TELLER, injectedBank,
        injectedAction, injectedTarget, AuditStatus::FAILED, injectedDetails
    );
    STRESS_ASSERT(!logId.empty(), "Log action returned empty logId");

    // Persist and reload
    logger.flushToDisk();
    logger.reloadFromFile();

    auto logs = logger.getAllLogs();
    STRESS_ASSERT(logs.size() == 1, "Expected 1 log entry after reload, got " + std::to_string(logs.size()));

    if (!logs.empty()) {
        const auto& entry = logs[0];
        // Details field pipe sanitization check
        STRESS_ASSERT(entry.details.find('|') == std::string::npos, "Details should not contain unescaped pipe '|'");
        STRESS_ASSERT(entry.details.find('\n') == std::string::npos, "Details should not contain unescaped newline '\\n'");
    }

    // Clear test log
    logger.clearLogs();

    // 5.2 High Concurrency Stress Test (20 threads x 50 logs = 1,000 logs)
    const int NUM_THREADS = 20;
    const int LOGS_PER_THREAD = 50;
    std::vector<std::thread> threads;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([t, LOGS_PER_THREAD]() {
            for (int i = 0; i < LOGS_PER_THREAD; ++i) {
                AuditLogger::getInstance().logAction(
                    "STAFF_" + std::to_string(t),
                    RoleTier::TELLER,
                    "SBI",
                    "STRESS_TX",
                    "ACC_" + std::to_string(t * 100 + i),
                    AuditStatus::SUCCESS,
                    "Concurrent stress log entry " + std::to_string(i)
                );
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double durationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    STRESS_ASSERT(logger.getLogCount() == static_cast<size_t>(NUM_THREADS * LOGS_PER_THREAD), 
                  "Concurrent log count mismatch: expected 1000, got " + std::to_string(logger.getLogCount()));
    std::cout << "  [PERF] Logged 1,000 concurrent entries in " << durationMs << " ms." << std::endl;

    // Cleanup stress data file
    try {
        std::filesystem::remove(testPath);
    } catch (...) {}
}

int main() {
    std::cout << "====================================================" << std::endl;
    std::cout << "  Milestone 1 Empirical Stress Test Harness" << std::endl;
    std::cout << "====================================================" << std::endl;

    stressTestMoney();
    stressTestDateUtils();
    stressTestTableFormatter();
    stressTestPermissionManager();
    stressTestAuditLoggerConcurrencyAndInjection();

    std::cout << "====================================================" << std::endl;
    std::cout << "  STRESS TEST SUMMARY:" << std::endl;
    std::cout << "    PASSED: " << g_passCount << std::endl;
    std::cout << "    FAILED: " << g_failCount << std::endl;
    std::cout << "====================================================" << std::endl;

    return (g_failCount == 0) ? 0 : 1;
}
