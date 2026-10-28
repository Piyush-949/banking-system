#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../auth.h"
#include "../../account.h"
#include "../../utils.h"

using namespace BankingSystem;

TEST_CASE(test_audit_log_action_recording, "tier1") {
    std::string testDir = "_test_data_audit_1";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    size_t countBefore = auth.getAuditLogs().size();
    auth.logAction("TEST_ACTION", "SBI", "Testing audit logger functionality");

    const auto& logs = auth.getAuditLogs();
    ASSERT_EQ(logs.size(), countBefore + 1);
    ASSERT_STREQ(logs.back().action, "TEST_ACTION");
    ASSERT_STREQ(logs.back().bankId, "SBI");
    ASSERT_STREQ(logs.back().details, "Testing audit logger functionality");

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_audit_log_actor_and_role_tracking, "tier1") {
    std::string testDir = "_test_data_audit_2";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    // 1. Guest action (no active login session)
    auth.logAction("GUEST_ACTION", "SYSTEM", "Guest performing operation");
    AuditLog log1 = auth.getAuditLogs().back();
    ASSERT_STREQ(log1.actorId, "GUEST");
    ASSERT_EQ(static_cast<int>(log1.actorRole), static_cast<int>(Role::PUBLIC_GUEST));

    // 2. Staff action with login session
    Staff s;
    s.staffId = "TELLER_01";
    s.name = "John Cashier";
    s.role = Role::TELLER;
    s.bankId = "HDFC";
    s.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(s);

    auth.loginStaff("TELLER_01", "pass");
    auth.logAction("TELLER_ACTION", "HDFC", "Teller cash count");
    AuditLog log2 = auth.getAuditLogs().back();

    ASSERT_STREQ(log2.actorId, "TELLER_01");
    ASSERT_EQ(static_cast<int>(log2.actorRole), static_cast<int>(Role::TELLER));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_audit_log_timestamp_format, "tier1") {
    std::string testDir = "_test_data_audit_3";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    auth.logAction("TIMESTAMP_CHECK", "RBI", "Checking timestamp string");
    AuditLog log = auth.getAuditLogs().back();

    ASSERT_FALSE(log.timestamp.empty());
    ASSERT_FALSE(log.logId.empty());

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_audit_log_financial_operations, "tier1") {
    std::string testDir = "_test_data_audit_4";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);

    Customer c;
    c.customerId = "CUST_AUD";
    c.name = "Audit Test User";
    c.isKycVerified = true;
    c.bankId = "SBI";
    c.passwordHash = Utils::hashPassword("pwd");
    auth.registerCustomer(c);

    Account* userAcc = acc.openAccount("CUST_AUD", "SBI", "BR001", AccountType::SAVINGS, 5000.0);
    ASSERT_TRUE(userAcc != nullptr);
    std::string accNo = userAcc->accountNo;

    acc.deposit(accNo, 2000.0, "STAFF01");
    acc.withdraw(accNo, 1000.0, "STAFF01");

    bool foundDeposit = false;
    bool foundWithdraw = false;
    for (const auto& l : auth.getAuditLogs()) {
        if (l.action == "CASH_DEPOSIT") foundDeposit = true;
        if (l.action == "CASH_WITHDRAWAL") foundWithdraw = true;
    }

    ASSERT_TRUE(foundDeposit);
    ASSERT_TRUE(foundWithdraw);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_audit_log_serialization_and_storage, "tier1") {
    std::string testDir = "_test_data_audit_5";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    auth.logAction("PERSIST_ACTION_1", "SBI", "Detail line 1");
    auth.logAction("PERSIST_ACTION_2", "HDFC", "Detail line 2");

    std::vector<AuditLog> loadedLogs = store.loadAuditLogs();
    ASSERT_TRUE(loadedLogs.size() >= 2);

    bool found1 = false, found2 = false;
    for (const auto& l : loadedLogs) {
        if (l.action == "PERSIST_ACTION_1" && l.details == "Detail line 1") found1 = true;
        if (l.action == "PERSIST_ACTION_2" && l.details == "Detail line 2") found2 = true;
    }

    ASSERT_TRUE(found1);
    ASSERT_TRUE(found2);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
