#include "TestHarness.hpp"
#include "TestUtils.hpp"
#include "account.h"
#include "auth.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

using namespace BankingSystem;

static void setupAccountTestEnv(Storage& store, AuthManager& auth, AccountManager& acc) {
    E2ETestHarness::TempEnvironment::cleanupDataDir("data");
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    auth.reloadUsers();
    acc.loadAccounts();

    Customer cust;
    cust.customerId = "CUST_ACC";
    cust.name = "Account Test User";
    cust.isKycVerified = true;
    cust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(cust);

    Staff teller;
    teller.staffId = "TEL001";
    teller.role = Role::TELLER;
    teller.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(teller);
    auth.loginStaff("TEL001", "pass");
}

TEST_CASE(test_tier2_accounts_zero_balance_withdrawal, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    AccountManager acc(store, auth);
    setupAccountTestEnv(store, auth, acc);

    // Open Savings Account with 0.0 initial deposit
    Account* a = acc.openAccount("CUST_ACC", "SBI", "BR001", AccountType::SAVINGS, 0.0);
    ASSERT_NE(a, nullptr);
    ASSERT_ALMOST_EQ(a->balance, 0.0, 0.001);

    // Attempt withdrawal of 100.0 from 0 balance account
    bool res = acc.withdraw(a->accountNo, 100.0, "TEL001");
    ASSERT_FALSE(res);
    ASSERT_ALMOST_EQ(a->balance, 0.0, 0.001);
}

TEST_CASE(test_tier2_accounts_exact_overdraft_boundary, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    AccountManager acc(store, auth);
    setupAccountTestEnv(store, auth, acc);

    // Open Current Account (overdraft limit = 50,000) with 0 initial deposit
    Account* a = acc.openAccount("CUST_ACC", "SBI", "BR001", AccountType::CURRENT, 0.0);
    ASSERT_NE(a, nullptr);
    ASSERT_ALMOST_EQ(a->overdraftLimit, 50000.0, 0.001);

    // Withdraw exactly max available (balance 0 + overdraft 50000 = 50000)
    bool res1 = acc.withdraw(a->accountNo, 50000.0, "TEL001");
    ASSERT_TRUE(res1);
    ASSERT_ALMOST_EQ(a->balance, -50000.0, 0.001);

    // Attempt to withdraw 1.0 more over overdraft limit
    bool res2 = acc.withdraw(a->accountNo, 1.0, "TEL001");
    ASSERT_FALSE(res2);
    ASSERT_ALMOST_EQ(a->balance, -50000.0, 0.001);
}

TEST_CASE(test_tier2_accounts_min_balance_rule, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    AccountManager acc(store, auth);
    setupAccountTestEnv(store, auth, acc);

    // Deposit 1000.0 (Standard min balance requirement)
    Account* a = acc.openAccount("CUST_ACC", "SBI", "BR001", AccountType::SAVINGS, 1000.0);
    ASSERT_NE(a, nullptr);
    ASSERT_ALMOST_EQ(a->balance, 1000.0, 0.001);

    // Withdraw down to exactly 0.0
    bool res = acc.withdraw(a->accountNo, 1000.0, "TEL001");
    ASSERT_TRUE(res);
    ASSERT_ALMOST_EQ(a->balance, 0.0, 0.001);
}

TEST_CASE(test_tier2_accounts_transfer_invalid_account, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    AccountManager acc(store, auth);
    setupAccountTestEnv(store, auth, acc);

    Account* a = acc.openAccount("CUST_ACC", "SBI", "BR001", AccountType::SAVINGS, 5000.0);
    ASSERT_NE(a, nullptr);

    // Transfer to non-existent account number
    bool res = acc.transfer(a->accountNo, "ACC_NONEXISTENT_999", 1000.0, "TEL001");
    ASSERT_FALSE(res);
    ASSERT_ALMOST_EQ(a->balance, 5000.0, 0.001);
}

TEST_CASE(test_tier2_accounts_negative_deposit_withdrawal, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    AccountManager acc(store, auth);
    setupAccountTestEnv(store, auth, acc);

    Account* a = acc.openAccount("CUST_ACC", "SBI", "BR001", AccountType::SAVINGS, 2000.0);
    ASSERT_NE(a, nullptr);

    // Negative deposit
    bool resDep = acc.deposit(a->accountNo, -500.0, "TEL001");
    ASSERT_FALSE(resDep);
    ASSERT_ALMOST_EQ(a->balance, 2000.0, 0.001);

    // Negative withdrawal
    bool resWth = acc.withdraw(a->accountNo, -500.0, "TEL001");
    ASSERT_FALSE(resWth);
    ASSERT_ALMOST_EQ(a->balance, 2000.0, 0.001);
}
