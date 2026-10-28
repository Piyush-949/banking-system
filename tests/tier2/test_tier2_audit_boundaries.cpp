#include "TestHarness.hpp"
#include "TestUtils.hpp"
#include "auth.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"
#include <filesystem>

using namespace BankingSystem;

static void setupAuditTestEnv(Storage& store, AuthManager& auth) {
    E2ETestHarness::TempEnvironment::cleanupDataDir("data");
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    auth.reloadUsers();
}

TEST_CASE(test_tier2_audit_special_characters_newlines, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupAuditTestEnv(store, auth);

    std::string specialAction = "ACTION|WITH|PIPES_AND_\"QUOTES\"";
    std::string specialDetails = "Line1\nLine2\tTabbed|Piped";

    auth.logAction(specialAction, "BNK001", specialDetails);

    const auto& logs = auth.getAuditLogs();
    ASSERT_FALSE(logs.empty());

    AuditLog lastLog = logs.back();
    std::string serialized = lastLog.serialize();
    AuditLog deserialized = AuditLog::deserialize(serialized);

    ASSERT_FALSE(serialized.empty());
    ASSERT_STREQ(deserialized.logId, lastLog.logId);
}

TEST_CASE(test_tier2_audit_long_actor_id, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupAuditTestEnv(store, auth);

    std::string longActorId(500, 'X');
    Staff staff;
    staff.staffId = longActorId;
    staff.name = "Long ID Staff";
    staff.role = Role::TELLER;
    staff.passwordHash = Utils::hashPassword("pass");

    auth.addStaffMember(staff);
    auth.loginStaff(longActorId, "pass");

    auth.logAction("LONG_ACTOR_ACTION", "BNK001", "Testing 500-char actor ID");

    const auto& logs = auth.getAuditLogs();
    ASSERT_FALSE(logs.empty());
    ASSERT_STREQ(logs.back().actorId, longActorId);
}

TEST_CASE(test_tier2_audit_high_volume_logging, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupAuditTestEnv(store, auth);

    // Perform 100 sequential log actions
    for (int i = 0; i < 100; ++i) {
        auth.logAction("BULK_ACTION_" + std::to_string(i), "BNK001", "Details for action " + std::to_string(i));
    }

    const auto& logs = auth.getAuditLogs();
    ASSERT_TRUE(logs.size() >= 100);

    // Verify storage file reload
    Storage store2("data");
    AuthManager auth2(store2);
    ASSERT_TRUE(auth2.getAuditLogs().size() >= 100);
}

TEST_CASE(test_tier2_audit_deletion_recovery, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupAuditTestEnv(store, auth);

    auth.logAction("BEFORE_DELETE", "BNK001", "Testing file deletion recovery");

    // Remove audit file
    std::error_code ec;
    std::filesystem::remove("data/audit.dat", ec);
    ASSERT_FALSE(std::filesystem::exists("data/audit.dat"));

    // Log new action
    auth.logAction("AFTER_DELETE", "BNK001", "Recovered after file deletion");

    ASSERT_TRUE(std::filesystem::exists("data/audit.dat"));
}

TEST_CASE(test_tier2_audit_atomic_append_safety, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupAuditTestEnv(store, auth);

    AuditLog log1;
    log1.logId = "LOG001";
    log1.timestamp = Utils::getCurrentTimestamp();
    log1.actorId = "ACTOR1";
    log1.actorRole = Role::BRANCH_MANAGER;
    log1.action = "TEST_APPEND_1";
    log1.bankId = "BNK001";
    log1.details = "Test Details 1";

    std::string line1 = log1.serialize();
    AuditLog d1 = AuditLog::deserialize(line1);

    ASSERT_STREQ(d1.logId, log1.logId);
    ASSERT_STREQ(d1.actorId, log1.actorId);
    ASSERT_EQ(static_cast<int>(d1.actorRole), static_cast<int>(Role::BRANCH_MANAGER));
}
