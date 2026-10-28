#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../auth.h"
#include "../../rbi.h"
#include "../../account.h"
#include "../../fd_rd.h"
#include "../../utils.h"

using namespace BankingSystem;

static void setupFDRDEnv(Storage& store, AuthManager& auth, RBIController& rbi, AccountManager& acc) {
    Staff governor;
    governor.staffId = "GOV001";
    governor.role = Role::RBI_GOVERNOR;
    governor.passwordHash = Utils::hashPassword("gov");
    auth.addStaffMember(governor);

    auth.loginStaff("GOV001", "gov");
    rbi.registerBank("State Bank of India", "SBI", "CMD_SBI");
    auth.logout();

    // Regular customer
    Customer c1;
    c1.customerId = "CUST_REG";
    c1.name = "Regular Saver";
    c1.pan = "PAN_REG123";
    c1.isKycVerified = true;
    c1.isSeniorCitizen = false;
    c1.bankId = "SBI";
    c1.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(c1);

    // Senior citizen customer
    Customer c2;
    c2.customerId = "CUST_SENIOR";
    c2.name = "Senior Saver";
    c2.pan = "PAN_SEN456";
    c2.isKycVerified = true;
    c2.isSeniorCitizen = true;
    c2.bankId = "SBI";
    c2.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(c2);

    acc.openAccount("CUST_REG", "SBI", "BR001", AccountType::SAVINGS, 1000000.0);
    acc.openAccount("CUST_SENIOR", "SBI", "BR001", AccountType::SAVINGS, 1000000.0);
}

TEST_CASE(test_term_deposit_fd_creation_and_interest, "tier1") {
    std::string testDir = "_test_data_fd_1";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    FDRDManager fdrd(store, auth, rbi, acc);

    setupFDRDEnv(store, auth, rbi, acc);

    std::string accNo = acc.getCustomerAccounts("CUST_REG")[0].accountNo;
    std::string bankId = rbi.findBankByCode("SBI")->bankId;

    // Create 12-month FD for ₹1,00,000
    FixedDeposit* fd = fdrd.createFD("CUST_REG", bankId, accNo, 100000.0, 12);
    ASSERT_TRUE(fd != nullptr);
    ASSERT_ALMOST_EQ(fd->principal, 100000.0, 0.01);
    ASSERT_ALMOST_EQ(fd->interestRate, 6.75, 0.01);
    ASSERT_TRUE(fd->maturityAmount > 100000.0);

    // Account balance should decrease by 1,00,000
    Account* accUpdated = acc.findAccount(accNo);
    ASSERT_ALMOST_EQ(accUpdated->balance, 900000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_term_deposit_senior_citizen_perks, "tier1") {
    std::string testDir = "_test_data_fd_2";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    FDRDManager fdrd(store, auth, rbi, acc);

    setupFDRDEnv(store, auth, rbi, acc);

    std::string accNoSenior = acc.getCustomerAccounts("CUST_SENIOR")[0].accountNo;
    std::string bankId = rbi.findBankByCode("SBI")->bankId;

    // Senior Citizen FD (Base 6.75% + 0.50% bonus = 7.25%)
    FixedDeposit* fd = fdrd.createFD("CUST_SENIOR", bankId, accNoSenior, 100000.0, 12);
    ASSERT_TRUE(fd != nullptr);
    ASSERT_TRUE(fd->isSeniorCitizen);
    ASSERT_ALMOST_EQ(fd->interestRate, 7.25, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_term_deposit_tds_deduction, "tier1") {
    std::string testDir = "_test_data_fd_3";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    FDRDManager fdrd(store, auth, rbi, acc);

    setupFDRDEnv(store, auth, rbi, acc);

    std::string accNo = acc.getCustomerAccounts("CUST_REG")[0].accountNo;
    std::string bankId = rbi.findBankByCode("SBI")->bankId;

    // Large FD ₹7,00,000 for 5 years @ 7.50% -> total interest will exceed ₹40,000 threshold
    FixedDeposit* fd = fdrd.createFD("CUST_REG", bankId, accNo, 700000.0, 60);
    ASSERT_TRUE(fd != nullptr);
    ASSERT_TRUE(fd->tdsDeducted > 0.0);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_term_deposit_rd_installment_workflow, "tier1") {
    std::string testDir = "_test_data_fd_4";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    FDRDManager fdrd(store, auth, rbi, acc);

    setupFDRDEnv(store, auth, rbi, acc);

    std::string accNo = acc.getCustomerAccounts("CUST_REG")[0].accountNo;
    std::string bankId = rbi.findBankByCode("SBI")->bankId;

    // Create 3-month RD with monthly installment ₹5,000
    RecurringDeposit* rd = fdrd.createRD("CUST_REG", bankId, accNo, 5000.0, 3);
    ASSERT_TRUE(rd != nullptr);
    ASSERT_EQ(rd->monthsPaid, 1);
    std::string rdNo = rd->rdNo;

    // Pay installment #2
    ASSERT_TRUE(fdrd.payRDMonthlyInstallment(rdNo));

    // Pay installment #3 (Final installment -> should mature RD)
    ASSERT_TRUE(fdrd.payRDMonthlyInstallment(rdNo));

    std::vector<RecurringDeposit> rds = fdrd.getCustomerRDs("CUST_REG");
    bool foundClosed = false;
    for (const auto& r : rds) {
        if (r.rdNo == rdNo && r.isClosed) foundClosed = true;
    }
    ASSERT_TRUE(foundClosed);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_term_deposit_fd_premature_withdrawal, "tier1") {
    std::string testDir = "_test_data_fd_5";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    FDRDManager fdrd(store, auth, rbi, acc);

    setupFDRDEnv(store, auth, rbi, acc);

    std::string accNo = acc.getCustomerAccounts("CUST_REG")[0].accountNo;
    std::string bankId = rbi.findBankByCode("SBI")->bankId;

    FixedDeposit* fd = fdrd.createFD("CUST_REG", bankId, accNo, 100000.0, 24);
    std::string fdNo = fd->fdNo;

    // Premature withdrawal
    ASSERT_TRUE(fdrd.prematureWithdrawFD(fdNo));

    std::vector<FixedDeposit> fds = fdrd.getCustomerFDs("CUST_REG");
    bool isClosed = false;
    for (const auto& f : fds) {
        if (f.fdNo == fdNo && f.isClosed) isClosed = true;
    }
    ASSERT_TRUE(isClosed);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
