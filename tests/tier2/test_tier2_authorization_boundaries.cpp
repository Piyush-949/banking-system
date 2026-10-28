#include "TestHarness.hpp"
#include "TestUtils.hpp"
#include "auth.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

using namespace BankingSystem;

static void setupAuthTestEnv(Storage& store, AuthManager& auth) {
    E2ETestHarness::TempEnvironment::cleanupDataDir("data");
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    auth.reloadUsers();
}

TEST_CASE(test_tier2_auth_role_escalation_violation, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupAuthTestEnv(store, auth);

    Customer cust;
    cust.customerId = "CUST100";
    cust.name = "John Customer";
    cust.passwordHash = Utils::hashPassword("pass123");
    auth.registerCustomer(cust);
    auth.loginCustomer("CUST100", "pass123");

    // Customer role is 7
    ASSERT_EQ(static_cast<int>(auth.getSession().role), static_cast<int>(Role::CUSTOMER));

    // Attempting Governor (Role 0) permissions
    ASSERT_FALSE(auth.checkPermission(Role::RBI_GOVERNOR));
    // Attempting CMD (Role 1) permissions
    ASSERT_FALSE(auth.checkPermission(Role::BANK_CMD));
    // Attempting Branch Manager (Role 3) permissions
    ASSERT_FALSE(auth.checkPermission(Role::BRANCH_MANAGER));

    // Customer permission check
    ASSERT_TRUE(auth.checkPermission(Role::CUSTOMER));
}

TEST_CASE(test_tier2_auth_session_expiration_edge_cases, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupAuthTestEnv(store, auth);

    // Initial state: not logged in
    ASSERT_FALSE(auth.getSession().isLoggedIn);
    ASSERT_FALSE(auth.checkPermission(Role::CUSTOMER));

    // Login Staff
    Staff staff;
    staff.staffId = "BM001";
    staff.name = "Branch Mgr";
    staff.role = Role::BRANCH_MANAGER;
    staff.passwordHash = Utils::hashPassword("pass123");
    auth.addStaffMember(staff);

    bool loginSuccess = auth.loginStaff("BM001", "pass123");
    ASSERT_TRUE(loginSuccess);
    ASSERT_TRUE(auth.getSession().isLoggedIn);
    ASSERT_STREQ(auth.getSession().userId, "BM001");

    // Logout session
    auth.logout();
    ASSERT_FALSE(auth.getSession().isLoggedIn);
    ASSERT_FALSE(auth.checkPermission(Role::BRANCH_MANAGER));
}

TEST_CASE(test_tier2_auth_invalid_role_tier_ids, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupAuthTestEnv(store, auth);

    // Test role enum casting boundaries (-1, 8)
    Role invalidLow = static_cast<Role>(-1);
    Role invalidHigh = static_cast<Role>(8);

    ASSERT_STREQ(roleToString(invalidLow), "Guest / Public");
    ASSERT_STREQ(roleToString(invalidHigh), "Guest / Public");
    ASSERT_STREQ(roleToString(Role::PUBLIC_GUEST), "Guest / Public");
}

TEST_CASE(test_tier2_auth_permission_manager_boundaries, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupAuthTestEnv(store, auth);

    // Test permission hierarchy: lower enum value = higher authority
    Staff govStaff;
    govStaff.staffId = "GOV002";
    govStaff.role = Role::RBI_GOVERNOR;
    govStaff.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(govStaff);
    auth.loginStaff("GOV002", "pass");

    // Governor (0) should have permission for all roles 0..7
    for (int r = 0; r <= 7; ++r) {
        ASSERT_TRUE(auth.checkPermission(static_cast<Role>(r)));
    }

    auth.logout();

    // Test Teller (Role 6)
    Staff tellerStaff;
    tellerStaff.staffId = "TEL001";
    tellerStaff.role = Role::TELLER;
    tellerStaff.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(tellerStaff);
    auth.loginStaff("TEL001", "pass");

    // Teller (6) should NOT have permission for roles 0..5
    ASSERT_FALSE(auth.checkPermission(Role::RBI_GOVERNOR));
    ASSERT_FALSE(auth.checkPermission(Role::BRANCH_MANAGER));
    ASSERT_FALSE(auth.checkPermission(Role::LOAN_OFFICER));
    ASSERT_TRUE(auth.checkPermission(Role::TELLER));
    ASSERT_TRUE(auth.checkPermission(Role::CUSTOMER));
}

TEST_CASE(test_tier2_auth_unauthorized_loan_review_attempt, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupAuthTestEnv(store, auth);

    Staff bmStaff;
    bmStaff.staffId = "BM002";
    bmStaff.role = Role::BRANCH_MANAGER;
    bmStaff.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(bmStaff);
    auth.loginStaff("BM002", "pass");

    // Branch Manager (Role 3) cannot approve CMD level requirements (Role 1)
    ASSERT_FALSE(auth.checkPermission(Role::BANK_CMD));
    ASSERT_FALSE(auth.checkPermission(Role::ZONAL_MANAGER));
    ASSERT_TRUE(auth.checkPermission(Role::BRANCH_MANAGER));
    ASSERT_TRUE(auth.checkPermission(Role::LOAN_OFFICER));
}
