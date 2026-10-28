#include "TestHarness.hpp"
#include "TestUtils.hpp"
#include "bank.h"
#include "rbi.h"
#include "auth.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

using namespace BankingSystem;

static void setupBankTestEnv(Storage& store, AuthManager& auth) {
    E2ETestHarness::TempEnvironment::cleanupDataDir("data");
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    auth.reloadUsers();
    if (!auth.findStaff("CMD001")) {
        Staff cmdStaff;
        cmdStaff.staffId = "CMD001";
        cmdStaff.name = "Bank CMD";
        cmdStaff.role = Role::BANK_CMD;
        cmdStaff.bankId = "BNK001";
        cmdStaff.passwordHash = Utils::hashPassword("cmd123");
        auth.addStaffMember(cmdStaff);
    }
    auth.loginStaff("CMD001", "cmd123");
}

TEST_CASE(test_tier2_bank_single_bank_comparison, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupBankTestEnv(store, auth);
    RBIController rbi(store, auth);
    BankEngine engine(store, auth, rbi);

    rbi.registerBank("Solo Bank India", "SOLO", "CMD001", true);
    ASSERT_EQ(rbi.getBanks().size(), 1);

    // Call comparison matrix with single bank
    engine.displayBankComparisonTable();
    BankInfo* b = rbi.findBankByCode("SOLO");
    ASSERT_NE(b, nullptr);
    ASSERT_STREQ(b->name, "Solo Bank India");
}

TEST_CASE(test_tier2_bank_equal_rate_tiebreaker, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupBankTestEnv(store, auth);
    RBIController rbi(store, auth);
    BankEngine engine(store, auth, rbi);

    rbi.registerBank("Alpha Bank", "ALPHA", "CMD001", true);
    rbi.registerBank("Beta Bank", "BETA", "CMD001", true);

    BankInfo* b1 = rbi.findBankByCode("ALPHA");
    BankInfo* b2 = rbi.findBankByCode("BETA");
    ASSERT_NE(b1, nullptr);
    ASSERT_NE(b2, nullptr);

    // Set identical rates
    engine.updateBankInterestRates(b1->bankId, 4.0, 8.5, 12.0, 7.0);
    engine.updateBankInterestRates(b2->bankId, 4.0, 8.5, 12.0, 7.0);

    ASSERT_ALMOST_EQ(b1->policies.savingsInterestRate, b2->policies.savingsInterestRate, 0.001);
    ASSERT_ALMOST_EQ(b1->policies.homeLoanBaseRate, b2->policies.homeLoanBaseRate, 0.001);
}

TEST_CASE(test_tier2_bank_non_existent_bank_lookup, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupBankTestEnv(store, auth);
    RBIController rbi(store, auth);

    BankInfo* b1 = rbi.findBank("BNK_NONEXISTENT_999");
    BankInfo* b2 = rbi.findBankByCode("NONEXISTENT_CODE");

    ASSERT_EQ(b1, nullptr);
    ASSERT_EQ(b2, nullptr);
}

TEST_CASE(test_tier2_bank_max_margin_spread_limits, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupBankTestEnv(store, auth);
    RBIController rbi(store, auth);
    BankEngine engine(store, auth, rbi);

    rbi.registerBank("Extreme Bank", "EXT", "CMD001", true);
    BankInfo* b = rbi.findBankByCode("EXT");
    ASSERT_NE(b, nullptr);

    // Extreme rates: Savings 50%, Home Loan 100%, Personal Loan 150%, FD 40%
    engine.updateBankInterestRates(b->bankId, 50.0, 100.0, 150.0, 40.0);
    ASSERT_ALMOST_EQ(b->policies.savingsInterestRate, 50.0, 0.001);
    ASSERT_ALMOST_EQ(b->policies.homeLoanBaseRate, 100.0, 0.001);

    // Extreme negative / zero spreads
    engine.updateBankInterestRates(b->bankId, 0.0, -5.0, 0.0, 0.0);
    ASSERT_ALMOST_EQ(b->policies.savingsInterestRate, 0.0, 0.001);
    ASSERT_ALMOST_EQ(b->policies.homeLoanBaseRate, -5.0, 0.001);
}

TEST_CASE(test_tier2_bank_empty_registry_query, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    setupBankTestEnv(store, auth);
    RBIController rbi(store, auth);
    BankEngine engine(store, auth, rbi);

    // Clean data directory so banks list is empty
    store.saveBanks({});
    rbi.loadData();

    ASSERT_EQ(rbi.getBanks().size(), 0);

    // Call comparison matrix and recalculate totals with empty bank list
    engine.displayBankComparisonTable();
    engine.recalculateBankTotals("BNK_DUMMY");
    ASSERT_EQ(rbi.findBank("BNK_DUMMY"), nullptr);
}
