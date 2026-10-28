#include "TestHarness.hpp"
#include "TestUtils.hpp"
#include "credit_card.h"
#include "account.h"
#include "auth.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

using namespace BankingSystem;

static void setupCCTestEnv(Storage& store, AuthManager& auth, AccountManager& acc) {
    E2ETestHarness::TempEnvironment::cleanupDataDir("data");
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    auth.reloadUsers();
    acc.loadAccounts();

    Customer custMin, custMax;
    custMin.customerId = "CUST_CIBIL_300";
    custMin.name = "Min CIBIL User";
    custMin.creditScore = 300; // Minimum score
    custMin.isKycVerified = true;
    custMin.passwordHash = Utils::hashPassword("pass");

    custMax.customerId = "CUST_CIBIL_900";
    custMax.name = "Max CIBIL User";
    custMax.creditScore = 900; // Maximum score
    custMax.isKycVerified = true;
    custMax.passwordHash = Utils::hashPassword("pass");

    auth.registerCustomer(custMin);
    auth.registerCustomer(custMax);

    Staff bm;
    bm.staffId = "BM001";
    bm.role = Role::BRANCH_MANAGER;
    bm.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(bm);
    auth.loginStaff("BM001", "pass");

    acc.openAccount("CUST_CIBIL_300", "SBI", "BR001", AccountType::SAVINGS, 100000.0);
    acc.openAccount("CUST_CIBIL_900", "SBI", "BR001", AccountType::SAVINGS, 100000.0);
}

TEST_CASE(test_tier2_credit_cards_cibil_min_max_eligibility, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    AccountManager acc(store, auth);
    CreditCardManager ccMgr(store, auth, acc);
    setupCCTestEnv(store, auth, acc);

    auto accMin = acc.getCustomerAccounts("CUST_CIBIL_300");
    auto accMax = acc.getCustomerAccounts("CUST_CIBIL_900");

    // CIBIL 300: Application for Classic Card (min score 650) -> Rejected
    CreditCard* ccFail = ccMgr.applyForCreditCard("CUST_CIBIL_300", "SBI", accMin[0].accountNo, CreditCardType::CLASSIC);
    ASSERT_EQ(ccFail, nullptr);

    // CIBIL 900: Application for Super Premium Card (min score 800) -> Approved
    CreditCard* ccPass = ccMgr.applyForCreditCard("CUST_CIBIL_900", "SBI", accMax[0].accountNo, CreditCardType::SUPER_PREMIUM);
    ASSERT_NE(ccPass, nullptr);
    ASSERT_ALMOST_EQ(ccPass->creditLimit, 1000000.0, 0.001);
}

TEST_CASE(test_tier2_credit_cards_exact_limit_expenditure, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    AccountManager acc(store, auth);
    CreditCardManager ccMgr(store, auth, acc);
    setupCCTestEnv(store, auth, acc);

    auto accMax = acc.getCustomerAccounts("CUST_CIBIL_900");
    CreditCard* cc = ccMgr.applyForCreditCard("CUST_CIBIL_900", "SBI", accMax[0].accountNo, CreditCardType::CLASSIC);
    ASSERT_NE(cc, nullptr);
    ASSERT_ALMOST_EQ(cc->availableLimit, 50000.0, 0.001);

    // Swipe exact available limit (50,000.0) -> Approved
    bool swipe1 = ccMgr.swipeCard(cc->cardNumber, 50000.0, "Electronics Store");
    ASSERT_TRUE(swipe1);
    ASSERT_ALMOST_EQ(cc->availableLimit, 0.0, 0.001);
    ASSERT_ALMOST_EQ(cc->currentOutstanding, 50000.0, 0.001);

    // Swipe 1.0 more over limit -> Declined
    bool swipe2 = ccMgr.swipeCard(cc->cardNumber, 1.0, "Coffee Shop");
    ASSERT_FALSE(swipe2);
    ASSERT_ALMOST_EQ(cc->availableLimit, 0.0, 0.001);
}

TEST_CASE(test_tier2_credit_cards_bill_payment_exceeding_balance, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    AccountManager acc(store, auth);
    CreditCardManager ccMgr(store, auth, acc);
    setupCCTestEnv(store, auth, acc);

    auto accMax = acc.getCustomerAccounts("CUST_CIBIL_900");
    CreditCard* cc = ccMgr.applyForCreditCard("CUST_CIBIL_900", "SBI", accMax[0].accountNo, CreditCardType::GOLD);
    ASSERT_NE(cc, nullptr);

    ccMgr.swipeCard(cc->cardNumber, 80000.0, "Jewelry Store");

    // Reduce linked account balance to 1,000
    Account* a = acc.findAccount(accMax[0].accountNo);
    a->balance = 1000.0;

    // Attempt to pay 80,000 bill with only 1,000 balance -> Insufficient funds
    bool res = ccMgr.payCreditCardBill(cc->cardNumber, 80000.0);
    ASSERT_FALSE(res);
    ASSERT_ALMOST_EQ(cc->currentOutstanding, 80000.0, 0.001);
}

TEST_CASE(test_tier2_credit_cards_zero_cibil_rejection, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    AccountManager acc(store, auth);
    CreditCardManager ccMgr(store, auth, acc);
    setupCCTestEnv(store, auth, acc);

    Customer custZero;
    custZero.customerId = "CUST_CIBIL_0";
    custZero.name = "Zero CIBIL User";
    custZero.creditScore = 0; // 0 CIBIL score
    custZero.isKycVerified = true;
    custZero.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(custZero);

    acc.openAccount("CUST_CIBIL_0", "SBI", "BR001", AccountType::SAVINGS, 50000.0);
    auto accZero = acc.getCustomerAccounts("CUST_CIBIL_0");

    CreditCard* cc = ccMgr.applyForCreditCard("CUST_CIBIL_0", "SBI", accZero[0].accountNo, CreditCardType::CLASSIC);
    ASSERT_EQ(cc, nullptr);
}

TEST_CASE(test_tier2_credit_cards_daily_finance_charge, "tier2") {
    // Overdue balance ₹50,000 (5,000,000 Paise) at 42% APR for 30 days
    int64_t balancePaise = 5000000LL;
    double aprPct = 42.0;
    int days = 30;

    int64_t chargePaise = E2ETestUtils::calculateDailyFinanceCharge(balancePaise, aprPct, days);
    
    // Expected: 50,000 * (0.42 / 365) * 30 = ~1726.03 -> ~172603 Paise
    ASSERT_TRUE(chargePaise > 0);
    double chargeRupees = E2ETestUtils::paiseToRupees(chargePaise);
    ASSERT_ALMOST_EQ(chargeRupees, 1726.03, 5.0);
}
