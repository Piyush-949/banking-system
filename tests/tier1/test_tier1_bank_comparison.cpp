#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../auth.h"
#include "../../rbi.h"
#include "../../bank.h"
#include "../../utils.h"

using namespace BankingSystem;

TEST_CASE(test_bank_preseeded_list, "tier1") {
    std::string testDir = "_test_data_bank_1";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);

    Staff governor;
    governor.staffId = "GOV001";
    governor.role = Role::RBI_GOVERNOR;
    governor.passwordHash = Utils::hashPassword("gov");
    auth.addStaffMember(governor);
    auth.loginStaff("GOV001", "gov");

    rbi.registerBank("State Bank of India", "SBI", "CMD_SBI");
    rbi.registerBank("HDFC Bank", "HDFC", "CMD_HDFC");
    rbi.registerBank("ICICI Bank", "ICICI", "CMD_ICICI");
    rbi.registerBank("Axis Bank", "AXIS", "CMD_AXIS");
    rbi.registerBank("Punjab National Bank", "PNB", "CMD_PNB");

    ASSERT_TRUE(rbi.findBankByCode("SBI") != nullptr);
    ASSERT_TRUE(rbi.findBankByCode("HDFC") != nullptr);
    ASSERT_TRUE(rbi.findBankByCode("ICICI") != nullptr);
    ASSERT_TRUE(rbi.findBankByCode("AXIS") != nullptr);
    ASSERT_TRUE(rbi.findBankByCode("PNB") != nullptr);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_bank_interest_rate_schedules, "tier1") {
    std::string testDir = "_test_data_bank_2";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);

    Staff governor;
    governor.staffId = "GOV001";
    governor.role = Role::RBI_GOVERNOR;
    governor.passwordHash = Utils::hashPassword("gov");
    auth.addStaffMember(governor);
    auth.loginStaff("GOV001", "gov");

    rbi.registerBank("State Bank of India", "SBI", "CMD_SBI");
    BankInfo* sbi = rbi.findBankByCode("SBI");
    ASSERT_TRUE(sbi != nullptr);

    ASSERT_ALMOST_EQ(sbi->policies.minSavingsBalance, 1000.0, 0.01);
    ASSERT_ALMOST_EQ(sbi->policies.savingsInterestRate, 3.50, 0.01);
    ASSERT_ALMOST_EQ(sbi->policies.homeLoanBaseRate, 8.50, 0.01);
    ASSERT_ALMOST_EQ(sbi->policies.personalLoanBaseRate, 12.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_bank_fd_yield_tiers, "tier1") {
    std::string testDir = "_test_data_bank_3";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);

    Staff governor;
    governor.staffId = "GOV001";
    governor.role = Role::RBI_GOVERNOR;
    governor.passwordHash = Utils::hashPassword("gov");
    auth.addStaffMember(governor);
    auth.loginStaff("GOV001", "gov");

    rbi.registerBank("HDFC Bank", "HDFC", "CMD_HDFC");
    BankInfo* hdfc = rbi.findBankByCode("HDFC");
    ASSERT_TRUE(hdfc != nullptr);

    ASSERT_ALMOST_EQ(hdfc->policies.fd1YrRate, 6.75, 0.01);
    ASSERT_ALMOST_EQ(hdfc->policies.fd3YrRate, 7.10, 0.01);
    ASSERT_ALMOST_EQ(hdfc->policies.fd5YrRate, 7.50, 0.01);
    ASSERT_ALMOST_EQ(hdfc->policies.seniorCitizenBonus, 0.50, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_bank_cmd_rate_updates, "tier1") {
    std::string testDir = "_test_data_bank_4";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);

    Staff governor;
    governor.staffId = "GOV001";
    governor.role = Role::RBI_GOVERNOR;
    governor.passwordHash = Utils::hashPassword("gov");
    auth.addStaffMember(governor);
    auth.loginStaff("GOV001", "gov");

    rbi.registerBank("ICICI Bank", "ICICI", "CMD_ICICI");
    BankInfo* icici = rbi.findBankByCode("ICICI");
    ASSERT_TRUE(icici != nullptr);

    BankEngine engine(store, auth, rbi);
    engine.updateBankInterestRates(icici->bankId, 4.00, 8.25, 11.50, 7.00);

    BankInfo* iciciAfter = rbi.findBank(icici->bankId);
    ASSERT_TRUE(iciciAfter != nullptr);
    ASSERT_ALMOST_EQ(iciciAfter->policies.savingsInterestRate, 4.00, 0.01);
    ASSERT_ALMOST_EQ(iciciAfter->policies.homeLoanBaseRate, 8.25, 0.01);
    ASSERT_ALMOST_EQ(iciciAfter->policies.personalLoanBaseRate, 11.50, 0.01);
    ASSERT_ALMOST_EQ(iciciAfter->policies.fd1YrRate, 7.00, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_bank_comparison_and_brochure, "tier1") {
    std::string testDir = "_test_data_bank_5";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);

    Staff governor;
    governor.staffId = "GOV001";
    governor.role = Role::RBI_GOVERNOR;
    governor.passwordHash = Utils::hashPassword("gov");
    auth.addStaffMember(governor);
    auth.loginStaff("GOV001", "gov");

    rbi.registerBank("Axis Bank", "AXIS", "CMD_AXIS");
    BankInfo* axis = rbi.findBankByCode("AXIS");
    ASSERT_TRUE(axis != nullptr);

    BankEngine engine(store, auth, rbi);

    // Call display methods and verify they complete without exception
    engine.displayBankComparisonTable();
    engine.displayProductBrochure(axis->bankId);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
