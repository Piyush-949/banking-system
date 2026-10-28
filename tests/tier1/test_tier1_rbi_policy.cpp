#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../auth.h"
#include "../../rbi.h"
#include "../../utils.h"

using namespace BankingSystem;

TEST_CASE(test_rbi_policy_default_rates, "tier1") {
    std::string testDir = "_test_data_rbi_1";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);

    const RBIPolicy& policy = rbi.getPolicy();
    ASSERT_ALMOST_EQ(policy.repoRate, 6.50, 0.001);
    ASSERT_ALMOST_EQ(policy.reverseRepoRate, 6.25, 0.001);
    ASSERT_ALMOST_EQ(policy.crr, 4.50, 0.001);
    ASSERT_ALMOST_EQ(policy.slr, 18.00, 0.001);
    ASSERT_FALSE(policy.lastUpdatedDate.empty());

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_rbi_repo_rate_update, "tier1") {
    std::string testDir = "_test_data_rbi_2";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    Staff governor;
    governor.staffId = "GOV001";
    governor.name = "RBI Governor";
    governor.role = Role::RBI_GOVERNOR;
    governor.bankId = "RBI";
    governor.passwordHash = Utils::hashPassword("gov123");
    auth.addStaffMember(governor);
    auth.loginStaff("GOV001", "gov123");

    RBIController rbi(store, auth);
    rbi.setRepoRate(6.75);

    ASSERT_ALMOST_EQ(rbi.getPolicy().repoRate, 6.75, 0.001);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_rbi_rate_cascading, "tier1") {
    std::string testDir = "_test_data_rbi_3";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    Staff governor;
    governor.staffId = "GOV001";
    governor.name = "RBI Governor";
    governor.role = Role::RBI_GOVERNOR;
    governor.bankId = "RBI";
    governor.passwordHash = Utils::hashPassword("gov123");
    auth.addStaffMember(governor);
    auth.loginStaff("GOV001", "gov123");

    RBIController rbi(store, auth);
    rbi.registerBank("State Bank of India", "SBI", "CMD001");

    BankInfo* sbi = rbi.findBankByCode("SBI");
    ASSERT_TRUE(sbi != nullptr);
    double initialHomeRate = sbi->policies.homeLoanBaseRate;
    double initialPersonalRate = sbi->policies.personalLoanBaseRate;

    // Repo rate increases by +0.50% (from 6.50 to 7.00)
    rbi.setRepoRate(7.00);

    // Rate cascading expected: base rate += delta * 0.8 = 0.50 * 0.8 = 0.40%
    BankInfo* sbiAfter = rbi.findBankByCode("SBI");
    ASSERT_TRUE(sbiAfter != nullptr);
    ASSERT_ALMOST_EQ(sbiAfter->policies.homeLoanBaseRate, initialHomeRate + 0.40, 0.01);
    ASSERT_ALMOST_EQ(sbiAfter->policies.personalLoanBaseRate, initialPersonalRate + 0.40, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_rbi_governor_authority_enforcement, "tier1") {
    std::string testDir = "_test_data_rbi_4";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    // Customer user (Role 7)
    Customer cust;
    cust.customerId = "CUST100";
    cust.name = "John Doe";
    cust.passwordHash = Utils::hashPassword("pass123");
    cust.bankId = "SBI";
    auth.registerCustomer(cust);
    auth.loginCustomer("CUST100", "pass123");

    RBIController rbi(store, auth);
    double origRepo = rbi.getPolicy().repoRate;

    // Customer attempt to change rate should fail (permission check fails)
    rbi.setRepoRate(9.99);
    ASSERT_ALMOST_EQ(rbi.getPolicy().repoRate, origRepo, 0.001);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_rbi_monetary_metrics_and_circulars, "tier1") {
    std::string testDir = "_test_data_rbi_5";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);

    Staff governor;
    governor.staffId = "GOV001";
    governor.name = "RBI Governor";
    governor.role = Role::RBI_GOVERNOR;
    governor.bankId = "RBI";
    governor.passwordHash = Utils::hashPassword("gov123");
    auth.addStaffMember(governor);
    auth.loginStaff("GOV001", "gov123");

    RBIController rbi(store, auth);
    rbi.setReverseRepoRate(6.00);
    rbi.setCRR(4.75);
    rbi.setSLR(18.50);

    const RBIPolicy& policy = rbi.getPolicy();
    ASSERT_ALMOST_EQ(policy.reverseRepoRate, 6.00, 0.001);
    ASSERT_ALMOST_EQ(policy.crr, 4.75, 0.001);
    ASSERT_ALMOST_EQ(policy.slr, 18.50, 0.001);

    // Check that circulars contain CRR and SLR broadcast messages
    bool foundCRR = false, foundSLR = false;
    for (const auto& c : policy.circulars) {
        if (c.find("CRR updated to 4.750000%") != std::string::npos || c.find("CRR updated") != std::string::npos) foundCRR = true;
        if (c.find("SLR updated to 18.500000%") != std::string::npos || c.find("SLR updated") != std::string::npos) foundSLR = true;
    }
    ASSERT_TRUE(foundCRR);
    ASSERT_TRUE(foundSLR);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
