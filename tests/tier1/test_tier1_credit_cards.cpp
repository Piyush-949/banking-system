#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../auth.h"
#include "../../account.h"
#include "../../credit_card.h"
#include "../../utils.h"

using namespace BankingSystem;

static void setupCardEnv(AuthManager& auth, AccountManager& acc, const std::string& custId, int cibilScore) {
    Customer c;
    c.customerId = custId;
    c.name = "Card User " + custId;
    c.creditScore = cibilScore;
    c.isKycVerified = true;
    c.bankId = "HDFC";
    c.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(c);

    acc.openAccount(custId, "HDFC", "BR001", AccountType::SAVINGS, 500000.0);
}

TEST_CASE(test_credit_card_issuance_tiers, "tier1") {
    std::string testDir = "_test_data_cc_1";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);
    CreditCardManager ccMgr(store, auth, acc);

    setupCardEnv(auth, acc, "CUST_HIGH_CIBIL", 850);
    std::string accNo = acc.getCustomerAccounts("CUST_HIGH_CIBIL")[0].accountNo;

    CreditCard* c1 = ccMgr.applyForCreditCard("CUST_HIGH_CIBIL", "HDFC", accNo, CreditCardType::CLASSIC);
    ASSERT_TRUE(c1 != nullptr);
    std::string card1No = c1->cardNumber;

    CreditCard* c2 = ccMgr.applyForCreditCard("CUST_HIGH_CIBIL", "HDFC", accNo, CreditCardType::GOLD);
    ASSERT_TRUE(c2 != nullptr);
    std::string card2No = c2->cardNumber;

    CreditCard* c3 = ccMgr.applyForCreditCard("CUST_HIGH_CIBIL", "HDFC", accNo, CreditCardType::PLATINUM);
    ASSERT_TRUE(c3 != nullptr);
    std::string card3No = c3->cardNumber;

    CreditCard* c4 = ccMgr.applyForCreditCard("CUST_HIGH_CIBIL", "HDFC", accNo, CreditCardType::SUPER_PREMIUM);
    ASSERT_TRUE(c4 != nullptr);
    std::string card4No = c4->cardNumber;

    CreditCard* c1Up = ccMgr.findCard(card1No);
    CreditCard* c2Up = ccMgr.findCard(card2No);
    CreditCard* c3Up = ccMgr.findCard(card3No);
    CreditCard* c4Up = ccMgr.findCard(card4No);

    ASSERT_TRUE(c1Up != nullptr);
    ASSERT_TRUE(c2Up != nullptr);
    ASSERT_TRUE(c3Up != nullptr);
    ASSERT_TRUE(c4Up != nullptr);

    ASSERT_ALMOST_EQ(c1Up->creditLimit, 50000.0, 0.01);
    ASSERT_ALMOST_EQ(c2Up->creditLimit, 150000.0, 0.01);
    ASSERT_ALMOST_EQ(c3Up->creditLimit, 300000.0, 0.01);
    ASSERT_ALMOST_EQ(c4Up->creditLimit, 1000000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_credit_card_cibil_score_check, "tier1") {
    std::string testDir = "_test_data_cc_2";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);
    CreditCardManager ccMgr(store, auth, acc);

    // Low CIBIL score (600)
    setupCardEnv(auth, acc, "CUST_LOW_CIBIL", 600);
    std::string accNo = acc.getCustomerAccounts("CUST_LOW_CIBIL")[0].accountNo;

    // Application for Classic requires score >= 650 -> Should be rejected (return nullptr)
    CreditCard* card = ccMgr.applyForCreditCard("CUST_LOW_CIBIL", "HDFC", accNo, CreditCardType::CLASSIC);
    ASSERT_TRUE(card == nullptr);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_credit_card_swipe_and_limit_deduction, "tier1") {
    std::string testDir = "_test_data_cc_3";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);
    CreditCardManager ccMgr(store, auth, acc);

    setupCardEnv(auth, acc, "CUST_SWIPE", 780);
    std::string accNo = acc.getCustomerAccounts("CUST_SWIPE")[0].accountNo;

    CreditCard* card = ccMgr.applyForCreditCard("CUST_SWIPE", "HDFC", accNo, CreditCardType::GOLD);
    ASSERT_TRUE(card != nullptr);
    std::string cardNo = card->cardNumber;
    double initialLimit = card->availableLimit; // 150,000

    // Swipe ₹20,000 at merchant
    ASSERT_TRUE(ccMgr.swipeCard(cardNo, 20000.0, "Amazon India"));

    CreditCard* cardUpdated = ccMgr.findCard(cardNo);
    ASSERT_ALMOST_EQ(cardUpdated->availableLimit, initialLimit - 20000.0, 0.01);
    ASSERT_ALMOST_EQ(cardUpdated->currentOutstanding, 20000.0, 0.01);
    ASSERT_TRUE(cardUpdated->rewardPoints > 100.0); // 100 welcome bonus + earned points

    // Swipe over limit (140,000 > 130,000 remaining) -> should fail
    ASSERT_FALSE(ccMgr.swipeCard(cardNo, 140000.0, "Luxury Store"));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_credit_card_bill_payment, "tier1") {
    std::string testDir = "_test_data_cc_4";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);
    CreditCardManager ccMgr(store, auth, acc);

    setupCardEnv(auth, acc, "CUST_BILL", 780);
    std::string accNo = acc.getCustomerAccounts("CUST_BILL")[0].accountNo;

    CreditCard* card = ccMgr.applyForCreditCard("CUST_BILL", "HDFC", accNo, CreditCardType::PLATINUM);
    std::string cardNo = card->cardNumber;

    ccMgr.swipeCard(cardNo, 50000.0, "Electronics Store");

    // Pay bill ₹30,000 from linked savings account
    ASSERT_TRUE(ccMgr.payCreditCardBill(cardNo, 30000.0));

    CreditCard* cardUpdated = ccMgr.findCard(cardNo);
    ASSERT_ALMOST_EQ(cardUpdated->currentOutstanding, 20000.0, 0.01);
    ASSERT_ALMOST_EQ(cardUpdated->availableLimit, 280000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_credit_card_statement_and_apr_calculation, "tier1") {
    std::string testDir = "_test_data_cc_5";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    AccountManager acc(store, auth);
    CreditCardManager ccMgr(store, auth, acc);

    setupCardEnv(auth, acc, "CUST_STMT", 780);
    std::string accNo = acc.getCustomerAccounts("CUST_STMT")[0].accountNo;

    CreditCard* card = ccMgr.applyForCreditCard("CUST_STMT", "HDFC", accNo, CreditCardType::GOLD);
    std::string cardNo = card->cardNumber;
    ccMgr.swipeCard(cardNo, 10000.0, "Departmental Store");

    // Print statement test
    ccMgr.printCardStatement(cardNo);

    // Test APR finance charge calculation helper (36% APR on ₹10,000 for 30 days)
    int64_t balancePaise = E2ETestUtils::rupeesToPaise(10000.0);
    int64_t chargePaise = E2ETestUtils::calculateDailyFinanceCharge(balancePaise, 36.0, 30);
    ASSERT_TRUE(chargePaise > 0);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
