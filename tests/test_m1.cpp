#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

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

void testTypes() {
    std::cout << "[TEST] 1. Domain Enums & Bidirectional String Conversion..." << std::endl;

    // RoleTier
    assert(roleTierToString(RoleTier::RBI_GOVERNOR) == "RBI Governor");
    assert(stringToRoleTier("RBI Governor") == RoleTier::RBI_GOVERNOR);
    assert(stringToRoleTier("0") == RoleTier::RBI_GOVERNOR);
    assert(stringToRoleTier("CUSTOMER") == RoleTier::CUSTOMER);

    // AccountType
    assert(accountTypeToString(AccountType::SAVINGS) == "Savings");
    assert(stringToAccountType("SAVINGS") == AccountType::SAVINGS);
    assert(stringToAccountType("Current") == AccountType::CURRENT);

    // AccountStatus
    assert(accountStatusToString(AccountStatus::ACTIVE) == "ACTIVE");
    assert(stringToAccountStatus("ACTIVE") == AccountStatus::ACTIVE);

    // TransactionType
    assert(transactionTypeToString(TransactionType::DEPOSIT) == "DEPOSIT");
    assert(stringToTransactionType("DEPOSIT") == TransactionType::DEPOSIT);

    // LoanType
    assert(loanTypeToString(LoanType::HOME) == "Home Loan");
    assert(stringToLoanType("HOME") == LoanType::HOME);

    // LoanStatus
    assert(loanStatusToString(LoanStatus::BRANCH_MANAGER_APPROVED) == "BRANCH_MANAGER_APPROVED");
    assert(stringToLoanStatus("BRANCH_MANAGER_APPROVED") == LoanStatus::BRANCH_MANAGER_APPROVED);

    // TermDepositType
    assert(termDepositTypeToString(TermDepositType::FIXED_DEPOSIT) == "Fixed Deposit (FD)");
    assert(stringToTermDepositType("FD") == TermDepositType::FIXED_DEPOSIT);

    // CreditCardTier
    assert(creditCardTierToString(CreditCardTier::SUPER_PREMIUM) == "Super Premium");
    assert(stringToCreditCardTier("SUPER_PREMIUM") == CreditCardTier::SUPER_PREMIUM);

    // InsuranceType
    assert(insuranceTypeToString(InsuranceType::HEALTH) == "Health Insurance");
    assert(stringToInsuranceType("HEALTH") == InsuranceType::HEALTH);

    // InsuranceStatus
    assert(insuranceStatusToString(InsuranceStatus::CLAIMED) == "CLAIMED");
    assert(stringToInsuranceStatus("CLAIMED") == InsuranceStatus::CLAIMED);

    std::cout << "  -> PASSED" << std::endl;
}

void testMoney() {
    std::cout << "[TEST] 2. Fixed-Point Money (Paise) & Indian Currency Formatting..." << std::endl;

    Money m0;
    assert(m0.getPaise() == 0);
    assert(m0.format() == "₹0.00");

    Money m1 = Money::fromRupees(1234567.89);
    assert(m1.getPaise() == 123456789LL);
    assert(m1.format() == "₹12,34,567.89");

    Money m2 = Money::fromRupees(2500000.00); // ₹25 Lakhs
    assert(m2.format() == "₹25,00,000.00");

    Money m3 = Money::fromRupees(10000000.00); // ₹1 Crore
    assert(m3.format() == "₹1,00,00,000.00");

    Money negM = Money::fromRupees(-1234567.89);
    assert(negM.format() == "-₹12,34,567.89");

    // Money Arithmetic
    Money a = Money::fromRupees(100.50);
    Money b = Money::fromRupees(50.25);
    Money sum = a + b;
    assert(sum.toRupees() == 150.75);

    Money diff = a - b;
    assert(diff.toRupees() == 50.25);

    Money mult = a * 2.0;
    assert(mult.toRupees() == 201.00);

    Money div = a / 2.0;
    assert(div.toRupees() == 50.25);

    // Parsing
    Money parsed1 = Money::parse("₹12,34,567.89");
    assert(parsed1.getPaise() == 123456789LL);

    Money parsed2 = Money::parse("-₹5,000.50");
    assert(parsed2.getPaise() == -500050LL);

    Money parsed3 = Money::parse("1000");
    assert(parsed3.getPaise() == 100000LL);

    // Exception check
    bool caughtException = false;
    try {
        Money divZero = a / 0.0;
        (void)divZero;
    } catch (const std::invalid_argument&) {
        caughtException = true;
    }
    assert(caughtException);

    std::cout << "  -> PASSED" << std::endl;
}

void testDateUtils() {
    std::cout << "[TEST] 3. DateUtils & ISO8601 Formatting..." << std::endl;

    std::string nowStr = DateUtils::nowISO8601();
    assert(!nowStr.empty());
    assert(DateUtils::isValidISO8601(nowStr));

    std::string dateStr = DateUtils::currentDate();
    assert(dateStr.length() == 10);
    assert(DateUtils::isValidISO8601(dateStr));

    // Date Arithmetic
    std::string start = "2026-01-01";
    std::string nextDay = DateUtils::addDays(start, 10);
    assert(nextDay == "2026-01-11");

    std::string nextMonth = DateUtils::addMonths(start, 2);
    assert(nextMonth == "2026-03-01");

    std::string nextYear = DateUtils::addYears(start, 1);
    assert(nextYear == "2027-01-01");

    int64_t elapsed = DateUtils::calculateElapsedDays("2026-01-01", "2026-01-11");
    assert(elapsed == 10);

    std::cout << "  -> PASSED" << std::endl;
}

void testTableFormatter() {
    std::cout << "[TEST] 4. TableFormatter ASCII Rendering..." << std::endl;

    TableFormatter table("SAMPLE BANK TABLE");
    table.addColumn("Bank Name", 15, TableFormatter::Alignment::LEFT);
    table.addColumn("Savings ROI", 12, TableFormatter::Alignment::RIGHT);
    table.addRow({"State Bank of India", "3.50%"});
    table.addRow({"HDFC Bank", "3.75%"});

    std::string output = table.render();
    assert(!output.empty());
    assert(output.find("SAMPLE BANK TABLE") != std::string::npos);
    assert(output.find("State Bank of India") != std::string::npos);
    assert(output.find("3.75%") != std::string::npos);

    std::cout << "  -> PASSED" << std::endl;
}

void testAuthAndPermissions() {
    std::cout << "[TEST] 5. 8-Tier Authority & PermissionManager Logic..." << std::endl;

    // Bitwise operators
    Permission p = Permission::MANAGE_RBI_POLICY | Permission::ISSUE_RBI_CIRCULAR;
    assert((p & Permission::MANAGE_RBI_POLICY) == Permission::MANAGE_RBI_POLICY);
    assert((p & Permission::SET_BANK_POLICIES) == Permission::NONE);

    // Permission checks across Tiers
    assert(PermissionManager::hasPermission(RoleTier::RBI_GOVERNOR, Permission::MANAGE_RBI_POLICY));
    assert(!PermissionManager::hasPermission(RoleTier::CUSTOMER, Permission::MANAGE_RBI_POLICY));
    assert(PermissionManager::hasPermission(RoleTier::CUSTOMER, Permission::SELF_TRANSFER));

    // User Serialization
    User user1("GOV001", "Shaktikanta Das", "hash123", RoleTier::RBI_GOVERNOR, "RBI", "HO", true, 1700000000LL);
    std::string serialized = user1.serialize();
    User deserialized = User::deserialize(serialized);
    assert(deserialized.userId == user1.userId);
    assert(deserialized.username == user1.username);
    assert(deserialized.roleTier == user1.roleTier);
    assert(deserialized.bankCode == user1.bankCode);

    // Loan Approval Thresholds (<25L BM, 25L-1Cr ZM, >1Cr CMD)
    // Small Loan (20 Lakhs)
    assert(PermissionManager::canApproveLoanAmount(RoleTier::BRANCH_MANAGER, 2000000.0));
    assert(PermissionManager::canApproveLoanAmount(RoleTier::ZONAL_MANAGER, 2000000.0));
    assert(PermissionManager::canApproveLoanAmount(RoleTier::BANK_CMD, 2000000.0));
    assert(!PermissionManager::canApproveLoanAmount(RoleTier::LOAN_OFFICER, 2000000.0));
    assert(!PermissionManager::canApproveLoanAmount(RoleTier::CUSTOMER, 2000000.0));

    // Medium Loan (50 Lakhs)
    assert(!PermissionManager::canApproveLoanAmount(RoleTier::BRANCH_MANAGER, 5000000.0));
    assert(PermissionManager::canApproveLoanAmount(RoleTier::ZONAL_MANAGER, 5000000.0));
    assert(PermissionManager::canApproveLoanAmount(RoleTier::BANK_CMD, 5000000.0));

    // Large Loan (2 Crores)
    assert(!PermissionManager::canApproveLoanAmount(RoleTier::BRANCH_MANAGER, 20000000.0));
    assert(!PermissionManager::canApproveLoanAmount(RoleTier::ZONAL_MANAGER, 20000000.0));
    assert(PermissionManager::canApproveLoanAmount(RoleTier::BANK_CMD, 20000000.0));
    assert(PermissionManager::canApproveLoanAmount(RoleTier::RBI_GOVERNOR, 20000000.0));

    std::cout << "  -> PASSED" << std::endl;
}

void testAuditLogger() {
    std::cout << "[TEST] 6. Thread-Safe AuditLogger Singleton & Disk Persistence..." << std::endl;

    AuditLogger& logger = AuditLogger::getInstance();
    logger.setLogFilePath("data/test_audit_logs.dat");
    logger.clearLogs();

    std::string logId1 = logger.logAction("GOV001", RoleTier::RBI_GOVERNOR, "RBI", "UPDATE_REPO_RATE", "POLICY_01", AuditStatus::SUCCESS, "Repo rate changed to 6.50%");
    assert(!logId1.empty());

    std::string logId2 = logger.logAction("BM001", RoleTier::BRANCH_MANAGER, "SBI", "DEPOSIT", "ACC1001", AuditStatus::SUCCESS, "Deposited INR 50000");
    assert(!logId2.empty());

    assert(logger.getLogCount() == 2);

    // Query Tests
    auto actorLogs = logger.getLogsByActor("GOV001");
    assert(actorLogs.size() == 1);
    assert(actorLogs[0].actionType == "UPDATE_REPO_RATE");

    auto bankLogs = logger.getLogsByBank("SBI");
    assert(bankLogs.size() == 1);
    assert(bankLogs[0].targetId == "ACC1001");

    // File Persistence Reload Verification
    logger.flushToDisk();
    logger.reloadFromFile();
    assert(logger.getLogCount() == 2);

    // Multi-threaded Concurrency Test
    std::vector<std::thread> threads;
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < 10; ++i) {
                AuditLogger::getInstance().logAction(
                    "USER_" + std::to_string(t),
                    RoleTier::TELLER,
                    "SBI",
                    "CONCURRENT_OP",
                    "TARGET_" + std::to_string(i),
                    AuditStatus::SUCCESS,
                    "Thread testing"
                );
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    assert(logger.getLogCount() == 102);

    std::cout << "  -> PASSED" << std::endl;
}

int main() {
    std::cout << "====================================================" << std::endl;
    std::cout << "  Milestone 1 Test Suite - RBI Banking System" << std::endl;
    std::cout << "====================================================" << std::endl;

    testTypes();
    testMoney();
    testDateUtils();
    testTableFormatter();
    testAuthAndPermissions();
    testAuditLogger();

    std::cout << "====================================================" << std::endl;
    std::cout << "  ALL MILESTONE 1 TESTS PASSED SUCCESSFULLY! (100%)" << std::endl;
    std::cout << "====================================================" << std::endl;
    return 0;
}
