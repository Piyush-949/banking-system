#include "TestHarness.hpp"
#include "TestUtils.hpp"
#include "fd_rd.h"
#include "account.h"
#include "rbi.h"
#include "auth.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

using namespace BankingSystem;

static void setupFDRDTestEnv(Storage& store, AuthManager& auth, RBIController& rbi, AccountManager& acc) {
    E2ETestHarness::TempEnvironment::cleanupDataDir("data");
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    auth.reloadUsers();
    rbi.loadData();
    acc.loadAccounts();

    rbi.registerBank("State Bank of India", "SBI", "CMD001", true);

    Customer cust;
    cust.customerId = "CUST_FD";
    cust.name = "FD Test User";
    cust.isKycVerified = true;
    cust.isSeniorCitizen = false;
    cust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(cust);

    Staff bm;
    bm.staffId = "BM001";
    bm.role = Role::BRANCH_MANAGER;
    bm.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(bm);
    auth.loginStaff("BM001", "pass");

    acc.openAccount("CUST_FD", "SBI", "BR001", AccountType::SAVINGS, 500000.0);
}

TEST_CASE(test_tier2_term_deposits_premature_closure_timing, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    FDRDManager fdrd(store, auth, rbi, acc);
    setupFDRDTestEnv(store, auth, rbi, acc);

    auto custAccs = acc.getCustomerAccounts("CUST_FD");
    std::string accNo = custAccs[0].accountNo;
    std::string bankId = rbi.getBanks()[0].bankId;

    // Create FD of 100,000 for 12 months
    FixedDeposit* fd = fdrd.createFD("CUST_FD", bankId, accNo, 100000.0, 12);
    ASSERT_NE(fd, nullptr);
    ASSERT_FALSE(fd->isClosed);

    // Premature closure on Day 1 (1% penalty rate)
    bool res = fdrd.prematureWithdrawFD(fd->fdNo);
    ASSERT_TRUE(res);

    auto fds = fdrd.getCustomerFDs("CUST_FD");
    ASSERT_TRUE(fds[0].isClosed);
}

TEST_CASE(test_tier2_term_deposits_tds_threshold_boundary, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    FDRDManager fdrd(store, auth, rbi, acc);
    setupFDRDTestEnv(store, auth, rbi, acc);

    auto custAccs = acc.getCustomerAccounts("CUST_FD");
    std::string accNo = custAccs[0].accountNo;
    std::string bankId = rbi.getBanks()[0].bankId;

    // Test 1: Principal where total interest <= ₹40,000 -> TDS = 0
    FixedDeposit* fdSmall = fdrd.createFD("CUST_FD", bankId, accNo, 50000.0, 12);
    ASSERT_NE(fdSmall, nullptr);
    ASSERT_ALMOST_EQ(fdSmall->tdsDeducted, 0.0, 0.001);

    // Test 2: Principal where total interest > ₹40,000 -> TDS applies
    // Deposit 1,000,000 @ ~6.75% for 60 months (5 years) -> Interest > 40,000
    acc.deposit(accNo, 1000000.0, "BM001");
    FixedDeposit* fdLarge = fdrd.createFD("CUST_FD", bankId, accNo, 1000000.0, 60);
    ASSERT_NE(fdLarge, nullptr);
    ASSERT_GT(fdLarge->tdsDeducted, 0.0);
}

TEST_CASE(test_tier2_term_deposits_zero_duration_deposit, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    FDRDManager fdrd(store, auth, rbi, acc);
    setupFDRDTestEnv(store, auth, rbi, acc);

    auto custAccs = acc.getCustomerAccounts("CUST_FD");
    std::string accNo = custAccs[0].accountNo;
    std::string bankId = rbi.getBanks()[0].bankId;

    // Create FD with 0 tenure months
    FixedDeposit* fd = fdrd.createFD("CUST_FD", bankId, accNo, 10000.0, 0);
    ASSERT_NE(fd, nullptr);
    ASSERT_EQ(fd->tenureMonths, 0);
    ASSERT_ALMOST_EQ(fd->maturityAmount, 10000.0, 0.001);
}

TEST_CASE(test_tier2_term_deposits_missed_rd_installment, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    FDRDManager fdrd(store, auth, rbi, acc);
    setupFDRDTestEnv(store, auth, rbi, acc);

    auto custAccs = acc.getCustomerAccounts("CUST_FD");
    std::string accNo = custAccs[0].accountNo;
    std::string bankId = rbi.getBanks()[0].bankId;

    // Create RD with 5,000 monthly installment for 12 months
    RecurringDeposit* rd = fdrd.createRD("CUST_FD", bankId, accNo, 5000.0, 12);
    ASSERT_NE(rd, nullptr);
    ASSERT_EQ(rd->monthsPaid, 1);

    // Empty savings account balance to simulate missed installment
    Account* a = acc.findAccount(accNo);
    a->balance = 0.0;

    // Attempt to pay RD monthly installment with insufficient balance
    bool res = fdrd.payRDMonthlyInstallment(rd->rdNo);
    ASSERT_FALSE(res);
}

TEST_CASE(test_tier2_term_deposits_fd_exceeding_balance, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    FDRDManager fdrd(store, auth, rbi, acc);
    setupFDRDTestEnv(store, auth, rbi, acc);

    auto custAccs = acc.getCustomerAccounts("CUST_FD");
    std::string accNo = custAccs[0].accountNo;
    std::string bankId = rbi.getBanks()[0].bankId;

    // Account has 500,000. Attempt to open FD for 1,000,000
    FixedDeposit* fd = fdrd.createFD("CUST_FD", bankId, accNo, 1000000.0, 12);
    ASSERT_EQ(fd, nullptr);
}
