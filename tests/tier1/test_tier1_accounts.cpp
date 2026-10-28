#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../auth.h"
#include "../../account.h"
#include "../../utils.h"

using namespace BankingSystem;

static void setupAccountTestUser(AuthManager& auth, const std::string& custId, const std::string& name) {
    Customer c;
    c.customerId = custId;
    c.name = name;
    c.pan = "PAN_" + custId;
    c.isKycVerified = true;
    c.bankId = "SBI";
    c.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(c);
}

TEST_CASE(test_account_savings_opening_and_deposit, "tier1") {
    std::string testDir = "_test_data_acc_1";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);

    setupAccountTestUser(auth, "CUST_SAV", "Savings User");

    Account* a = acc.openAccount("CUST_SAV", "SBI", "BR001", AccountType::SAVINGS, 5000.0);
    ASSERT_TRUE(a != nullptr);
    std::string accNo = a->accountNo;
    ASSERT_EQ(static_cast<int>(a->type), static_cast<int>(AccountType::SAVINGS));
    ASSERT_ALMOST_EQ(a->balance, 5000.0, 0.01);

    // Deposit 3000
    ASSERT_TRUE(acc.deposit(accNo, 3000.0, "STAFF1"));
    Account* aUpdated = acc.findAccount(accNo);
    ASSERT_ALMOST_EQ(aUpdated->balance, 8000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_account_current_overdraft_withdrawal, "tier1") {
    std::string testDir = "_test_data_acc_2";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);

    setupAccountTestUser(auth, "CUST_CURR", "Current User");

    Account* a = acc.openAccount("CUST_CURR", "HDFC", "BR002", AccountType::CURRENT, 10000.0);
    ASSERT_TRUE(a != nullptr);
    std::string accNo = a->accountNo;
    ASSERT_EQ(static_cast<int>(a->type), static_cast<int>(AccountType::CURRENT));
    ASSERT_ALMOST_EQ(a->overdraftLimit, 50000.0, 0.01);

    // Balance is 10,000, Overdraft is 50,000 -> Max withdrawal is 60,000.
    // Withdraw 25,000 -> resulting balance should be -15,000.
    ASSERT_TRUE(acc.withdraw(accNo, 25000.0, "STAFF1"));
    Account* aUpdated = acc.findAccount(accNo);
    ASSERT_ALMOST_EQ(aUpdated->balance, -15000.0, 0.01);

    // Attempt to withdraw another 40,000 (total withdrawal 65,000 > max 60,000) should fail.
    ASSERT_FALSE(acc.withdraw(accNo, 40000.0, "STAFF1"));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_account_nri_and_joint_types, "tier1") {
    std::string testDir = "_test_data_acc_3";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);

    setupAccountTestUser(auth, "CUST_NRI", "NRI User");
    setupAccountTestUser(auth, "CUST_JOINT", "Joint User");

    Account* nriAcc = acc.openAccount("CUST_NRI", "ICICI", "BR003", AccountType::NRI, 15000.0);
    ASSERT_TRUE(nriAcc != nullptr);
    std::string nriNo = nriAcc->accountNo;

    Account* jointAcc = acc.openAccount("CUST_JOINT", "AXIS", "BR004", AccountType::JOINT, 20000.0);
    ASSERT_TRUE(jointAcc != nullptr);
    std::string jointNo = jointAcc->accountNo;

    Account* nriUpdated = acc.findAccount(nriNo);
    Account* jointUpdated = acc.findAccount(jointNo);
    ASSERT_TRUE(nriUpdated != nullptr);
    ASSERT_TRUE(jointUpdated != nullptr);

    ASSERT_EQ(static_cast<int>(nriUpdated->type), static_cast<int>(AccountType::NRI));
    ASSERT_EQ(static_cast<int>(jointUpdated->type), static_cast<int>(AccountType::JOINT));

    ASSERT_STREQ(accountTypeToString(AccountType::NRI), "NRI Account (NRE/NRO)");
    ASSERT_STREQ(accountTypeToString(AccountType::JOINT), "Joint Savings Account");

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_account_fund_transfers_neft_imps, "tier1") {
    std::string testDir = "_test_data_acc_4";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);

    setupAccountTestUser(auth, "CUST_SRC", "Sender User");
    setupAccountTestUser(auth, "CUST_DEST", "Receiver User");

    Account* srcAcc = acc.openAccount("CUST_SRC", "SBI", "BR001", AccountType::SAVINGS, 20000.0);
    ASSERT_TRUE(srcAcc != nullptr);
    std::string srcNo = srcAcc->accountNo;

    Account* destAcc = acc.openAccount("CUST_DEST", "HDFC", "BR002", AccountType::SAVINGS, 5000.0);
    ASSERT_TRUE(destAcc != nullptr);
    std::string destNo = destAcc->accountNo;

    // Transfer ₹8,000 from src to dest
    ASSERT_TRUE(acc.transfer(srcNo, destNo, 8000.0, "STAFF_ONLINE"));

    Account* sUpdated = acc.findAccount(srcNo);
    Account* dUpdated = acc.findAccount(destNo);

    ASSERT_ALMOST_EQ(sUpdated->balance, 12000.0, 0.01);
    ASSERT_ALMOST_EQ(dUpdated->balance, 13000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_account_debit_card_and_cheque_issuance, "tier1") {
    std::string testDir = "_test_data_acc_5";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);

    setupAccountTestUser(auth, "CUST_PERK", "Perk User");

    Account* a = acc.openAccount("CUST_PERK", "PNB", "BR005", AccountType::SAVINGS, 10000.0);
    ASSERT_TRUE(a != nullptr);
    std::string accNo = a->accountNo;

    ASSERT_FALSE(a->hasDebitCard);
    ASSERT_FALSE(a->hasChequeBook);

    ASSERT_TRUE(acc.applyDebitCard(accNo));
    ASSERT_TRUE(acc.applyChequeBook(accNo));

    Account* aUpdated = acc.findAccount(accNo);
    ASSERT_TRUE(aUpdated->hasDebitCard);
    ASSERT_TRUE(aUpdated->hasChequeBook);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
