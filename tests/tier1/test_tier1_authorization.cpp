#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../auth.h"
#include "../../utils.h"

using namespace BankingSystem;

TEST_CASE(test_auth_8_tier_role_hierarchy, "tier1") {
    ASSERT_EQ(static_cast<int>(Role::RBI_GOVERNOR), 0);
    ASSERT_EQ(static_cast<int>(Role::BANK_CMD), 1);
    ASSERT_EQ(static_cast<int>(Role::ZONAL_MANAGER), 2);
    ASSERT_EQ(static_cast<int>(Role::BRANCH_MANAGER), 3);
    ASSERT_EQ(static_cast<int>(Role::LOAN_OFFICER), 4);
    ASSERT_EQ(static_cast<int>(Role::RELATIONSHIP_MANAGER), 5);
    ASSERT_EQ(static_cast<int>(Role::TELLER), 6);
    ASSERT_EQ(static_cast<int>(Role::CUSTOMER), 7);
    ASSERT_EQ(static_cast<int>(Role::PUBLIC_GUEST), 8);

    ASSERT_STREQ(roleToString(Role::RBI_GOVERNOR), "RBI Governor");
    ASSERT_STREQ(roleToString(Role::BRANCH_MANAGER), "Branch Manager");
    ASSERT_STREQ(roleToString(Role::CUSTOMER), "Customer");
}

TEST_CASE(test_auth_staff_login_logout, "tier1") {
    std::string testDir = "_test_data_auth_1";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    Staff bm;
    bm.staffId = "BM101";
    bm.name = "Alice Manager";
    bm.role = Role::BRANCH_MANAGER;
    bm.bankId = "SBI";
    bm.branchCode = "BR001";
    bm.passwordHash = Utils::hashPassword("secret123");
    auth.addStaffMember(bm);

    // Invalid password login should fail
    ASSERT_FALSE(auth.loginStaff("BM101", "wrongpass"));
    ASSERT_FALSE(auth.getSession().isLoggedIn);

    // Valid login should succeed
    ASSERT_TRUE(auth.loginStaff("BM101", "secret123"));
    ASSERT_TRUE(auth.getSession().isLoggedIn);
    ASSERT_STREQ(auth.getSession().userId, "BM101");
    ASSERT_EQ(static_cast<int>(auth.getSession().role), static_cast<int>(Role::BRANCH_MANAGER));

    // Logout resets session
    auth.logout();
    ASSERT_FALSE(auth.getSession().isLoggedIn);
    ASSERT_EQ(static_cast<int>(auth.getSession().role), static_cast<int>(Role::PUBLIC_GUEST));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_auth_customer_login_logout, "tier1") {
    std::string testDir = "_test_data_auth_2";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    Customer cust;
    cust.customerId = "CUST500";
    cust.name = "Bob Smith";
    cust.pan = "ABCDE1234F";
    cust.bankId = "HDFC";
    cust.passwordHash = Utils::hashPassword("custpass");
    cust.isKycVerified = true;
    auth.registerCustomer(cust);

    // Invalid login
    ASSERT_FALSE(auth.loginCustomer("CUST500", "wrong"));
    ASSERT_FALSE(auth.getSession().isLoggedIn);

    // Valid login
    ASSERT_TRUE(auth.loginCustomer("CUST500", "custpass"));
    ASSERT_TRUE(auth.getSession().isLoggedIn);
    ASSERT_STREQ(auth.getSession().userId, "CUST500");
    ASSERT_EQ(static_cast<int>(auth.getSession().role), static_cast<int>(Role::CUSTOMER));

    auth.logout();
    ASSERT_FALSE(auth.getSession().isLoggedIn);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_auth_role_permission_hierarchy, "tier1") {
    std::string testDir = "_test_data_auth_3";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    Staff zm;
    zm.staffId = "ZM202";
    zm.role = Role::ZONAL_MANAGER; // Role enum value 2
    zm.passwordHash = Utils::hashPassword("zmpass");
    auth.addStaffMember(zm);
    auth.loginStaff("ZM202", "zmpass");

    // Zonal Manager (tier 2) should pass check for Branch Manager (tier 3), Loan Officer (tier 4), Customer (tier 7)
    ASSERT_TRUE(auth.checkPermission(Role::BRANCH_MANAGER));
    ASSERT_TRUE(auth.checkPermission(Role::LOAN_OFFICER));
    ASSERT_TRUE(auth.checkPermission(Role::CUSTOMER));

    // Zonal Manager (tier 2) should FAIL check for RBI Governor (tier 0) and CMD (tier 1)
    ASSERT_FALSE(auth.checkPermission(Role::RBI_GOVERNOR));
    ASSERT_FALSE(auth.checkPermission(Role::BANK_CMD));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_auth_security_otp_generation, "tier1") {
    std::string testDir = "_test_data_auth_4";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    std::string otp = auth.generateOTP();
    ASSERT_EQ(otp.length(), static_cast<size_t>(6));
    ASSERT_TRUE(auth.verifyOTP(otp, otp));
    ASSERT_FALSE(auth.verifyOTP("000000", "999999"));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
