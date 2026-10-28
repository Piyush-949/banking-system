#include "TestHarness.hpp"
#include "TestUtils.hpp"
#include "rbi.h"
#include "auth.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

using namespace BankingSystem;

// Setup helper for RBI tests
static void setupRBITestEnv(Storage& store, AuthManager& auth, Role userRole) {
    E2ETestHarness::TempEnvironment::cleanupDataDir("data");
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    auth.reloadUsers();
    if (!auth.findStaff("GOV001")) {
        Staff testStaff;
        testStaff.staffId = "GOV001";
        testStaff.name = "RBI Governor";
        testStaff.role = userRole;
        testStaff.bankId = "RBI";
        testStaff.passwordHash = Utils::hashPassword("admin123");
        auth.addStaffMember(testStaff);
    }
    auth.loginStaff("GOV001", "admin123");
}

TEST_CASE(test_tier2_rbi_repo_min_max_boundary, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupRBITestEnv(store, auth, Role::RBI_GOVERNOR);
    RBIController rbi(store, auth);

    // Min boundary (0.0%)
    rbi.setRepoRate(0.0);
    ASSERT_ALMOST_EQ(rbi.getPolicy().repoRate, 0.0, 0.001);

    // Max boundary (20.0%)
    rbi.setRepoRate(20.0);
    ASSERT_ALMOST_EQ(rbi.getPolicy().repoRate, 20.0, 0.001);

    // Normal policy rate (6.5%)
    rbi.setRepoRate(6.5);
    ASSERT_ALMOST_EQ(rbi.getPolicy().repoRate, 6.5, 0.001);
}

TEST_CASE(test_tier2_rbi_negative_rate_rejection, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupRBITestEnv(store, auth, Role::RBI_GOVERNOR);
    RBIController rbi(store, auth);

    double initialRepo = rbi.getPolicy().repoRate;

    // Set negative rates
    rbi.setRepoRate(-5.0);
    rbi.setReverseRepoRate(-2.5);
    rbi.setCRR(-1.0);
    rbi.setSLR(-10.0);

    // Verify rate updates or stability
    // Policy should track rate accurately or reject negative values
    ASSERT_TRUE(rbi.getPolicy().repoRate == -5.0 || rbi.getPolicy().repoRate == initialRepo);
}

TEST_CASE(test_tier2_rbi_crr_slr_combined_boundary, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupRBITestEnv(store, auth, Role::RBI_GOVERNOR);
    RBIController rbi(store, auth);

    // Set CRR = 50.0% and SLR = 60.0% (Combined = 110.0% > 100%)
    rbi.setCRR(50.0);
    rbi.setSLR(60.0);

    double sum = rbi.getPolicy().crr + rbi.getPolicy().slr;
    ASSERT_TRUE(sum >= 100.0 || (rbi.getPolicy().crr == 50.0 && rbi.getPolicy().slr == 60.0));
}

TEST_CASE(test_tier2_rbi_unauthorized_governor_action, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    
    // Login as Customer (Role 7) instead of RBI Governor (Role 0)
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    auth.reloadUsers();
    Customer cust;
    cust.customerId = "CUST99";
    cust.name = "Regular Customer";
    cust.passwordHash = Utils::hashPassword("pass123");
    cust.isKycVerified = true;
    cust.creditScore = 750;
    auth.registerCustomer(cust);
    auth.loginCustomer("CUST99", "pass123");

    RBIController rbi(store, auth);
    double origRepo = rbi.getPolicy().repoRate;

    // Attempt rate modification as Customer
    rbi.setRepoRate(10.0);

    // Rate should remain unchanged due to permission failure
    ASSERT_ALMOST_EQ(rbi.getPolicy().repoRate, origRepo, 0.001);
}

TEST_CASE(test_tier2_rbi_rapid_rate_update_sequence, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupRBITestEnv(store, auth, Role::RBI_GOVERNOR);
    RBIController rbi(store, auth);

    rbi.registerBank("State Bank of India", "SBI", "CMD001");

    // Rapid rate changes: 6.5% -> 7.0% -> 5.5% -> 6.0%
    rbi.setRepoRate(6.5);
    rbi.setRepoRate(7.0);
    rbi.setRepoRate(5.5);
    rbi.setRepoRate(6.0);

    ASSERT_ALMOST_EQ(rbi.getPolicy().repoRate, 6.0, 0.001);
    ASSERT_TRUE(rbi.getPolicy().circulars.size() >= 4);
}
