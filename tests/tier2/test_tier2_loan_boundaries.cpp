#include "TestHarness.hpp"
#include "TestUtils.hpp"
#include "loan.h"
#include "account.h"
#include "rbi.h"
#include "auth.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

using namespace BankingSystem;

static void setupLoanTestEnv(Storage& store, AuthManager& auth, RBIController& rbi, AccountManager& acc) {
    E2ETestHarness::TempEnvironment::cleanupDataDir("data");
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    auth.reloadUsers();
    rbi.loadData();
    acc.loadAccounts();

    // Register Bank
    rbi.registerBank("State Bank of India", "SBI", "CMD001", true);

    // Register Staff
    Staff loStaff, bmStaff, zmStaff, cmdStaff;
    loStaff.staffId = "LO001"; loStaff.role = Role::LOAN_OFFICER; loStaff.passwordHash = Utils::hashPassword("pass");
    bmStaff.staffId = "BM001"; bmStaff.role = Role::BRANCH_MANAGER; bmStaff.passwordHash = Utils::hashPassword("pass");
    zmStaff.staffId = "ZM001"; zmStaff.role = Role::ZONAL_MANAGER; zmStaff.passwordHash = Utils::hashPassword("pass");
    cmdStaff.staffId = "CMD001"; cmdStaff.role = Role::BANK_CMD; cmdStaff.passwordHash = Utils::hashPassword("pass");

    auth.addStaffMember(loStaff);
    auth.addStaffMember(bmStaff);
    auth.addStaffMember(zmStaff);
    auth.addStaffMember(cmdStaff);

    // Register Customer with high credit score (800)
    Customer cust;
    cust.customerId = "CUST_LOAN";
    cust.name = "Rahul Sharma";
    cust.isKycVerified = true;
    cust.creditScore = 800;
    cust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(cust);

    // Open linked account
    auth.loginStaff("BM001", "pass");
    acc.openAccount("CUST_LOAN", "SBI", "BR001", AccountType::SAVINGS, 500000.0);
}

TEST_CASE(test_tier2_loan_threshold_bm_vs_zm, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    LoanManager loanMgr(store, auth, rbi, acc);
    setupLoanTestEnv(store, auth, rbi, acc);

    auto custAccounts = acc.getCustomerAccounts("CUST_LOAN");
    ASSERT_FALSE(custAccounts.empty());
    std::string accNo = custAccounts[0].accountNo;
    std::string bankId = rbi.getBanks()[0].bankId;

    // Apply for ₹24,99,999 (Below ₹25L threshold -> BM approval)
    Loan* l1 = loanMgr.applyForLoan("CUST_LOAN", bankId, accNo, LoanType::PERSONAL, 2499999.0, 36);
    ASSERT_NE(l1, nullptr);

    // Review by Loan Officer
    auth.loginStaff("LO001", "pass");
    loanMgr.reviewLoanOfficer(l1->loanId, true, "Verified documents");
    ASSERT_EQ(l1->status, LoanStatus::UNDER_BRANCH_MANAGER);

    // Apply for ₹25,00,000 (At ₹25L threshold -> ZM approval)
    Loan* l2 = loanMgr.applyForLoan("CUST_LOAN", bankId, accNo, LoanType::HOME, 2500000.0, 60);
    ASSERT_NE(l2, nullptr);

    loanMgr.reviewLoanOfficer(l2->loanId, true, "Verified documents");
    ASSERT_EQ(l2->status, LoanStatus::UNDER_ZONAL_MANAGER);
}

TEST_CASE(test_tier2_loan_threshold_zm_vs_cmd, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    LoanManager loanMgr(store, auth, rbi, acc);
    setupLoanTestEnv(store, auth, rbi, acc);

    auto custAccounts = acc.getCustomerAccounts("CUST_LOAN");
    std::string accNo = custAccounts[0].accountNo;
    std::string bankId = rbi.getBanks()[0].bankId;

    // ₹1,00,00,000 (Exactly ₹1 Crore -> ZM threshold)
    Loan* l1 = loanMgr.applyForLoan("CUST_LOAN", bankId, accNo, LoanType::HOME, 10000000.0, 120);
    ASSERT_NE(l1, nullptr);

    auth.loginStaff("LO001", "pass");
    loanMgr.reviewLoanOfficer(l1->loanId, true, "Valid");
    ASSERT_EQ(l1->status, LoanStatus::UNDER_ZONAL_MANAGER);

    // ₹1,00,00,001 (Above ₹1 Crore -> CMD threshold)
    Loan* l2 = loanMgr.applyForLoan("CUST_LOAN", bankId, accNo, LoanType::BUSINESS, 10000001.0, 120);
    ASSERT_NE(l2, nullptr);

    loanMgr.reviewLoanOfficer(l2->loanId, true, "Valid");
    ASSERT_EQ(l2->status, LoanStatus::UNDER_CMD);
}

TEST_CASE(test_tier2_loan_zero_principal_amount, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    LoanManager loanMgr(store, auth, rbi, acc);
    setupLoanTestEnv(store, auth, rbi, acc);

    auto custAccounts = acc.getCustomerAccounts("CUST_LOAN");
    std::string accNo = custAccounts[0].accountNo;
    std::string bankId = rbi.getBanks()[0].bankId;

    // Apply for ₹0 loan
    Loan* l = loanMgr.applyForLoan("CUST_LOAN", bankId, accNo, LoanType::PERSONAL, 0.0, 12);
    ASSERT_NE(l, nullptr);
    ASSERT_ALMOST_EQ(l->principalAmount, 0.0, 0.001);
    ASSERT_ALMOST_EQ(l->emiAmount, 0.0, 0.001);
}

TEST_CASE(test_tier2_loan_negative_principal_amount, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    LoanManager loanMgr(store, auth, rbi, acc);
    setupLoanTestEnv(store, auth, rbi, acc);

    auto custAccounts = acc.getCustomerAccounts("CUST_LOAN");
    std::string accNo = custAccounts[0].accountNo;
    std::string bankId = rbi.getBanks()[0].bankId;

    // Apply for negative loan amount (-50000.0)
    Loan* l = loanMgr.applyForLoan("CUST_LOAN", bankId, accNo, LoanType::PERSONAL, -50000.0, 12);
    ASSERT_NE(l, nullptr);
    ASSERT_ALMOST_EQ(l->principalAmount, -50000.0, 0.001);
}

TEST_CASE(test_tier2_loan_reapplication_after_rejection, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    LoanManager loanMgr(store, auth, rbi, acc);
    setupLoanTestEnv(store, auth, rbi, acc);

    auto custAccounts = acc.getCustomerAccounts("CUST_LOAN");
    std::string accNo = custAccounts[0].accountNo;
    std::string bankId = rbi.getBanks()[0].bankId;

    // First loan application
    Loan* l1 = loanMgr.applyForLoan("CUST_LOAN", bankId, accNo, LoanType::CAR, 500000.0, 36);
    ASSERT_NE(l1, nullptr);

    // Reject first loan by Loan Officer
    auth.loginStaff("LO001", "pass");
    loanMgr.reviewLoanOfficer(l1->loanId, false, "Insufficient income proof");
    ASSERT_EQ(l1->status, LoanStatus::REJECTED);

    // Re-apply for new loan
    Loan* l2 = loanMgr.applyForLoan("CUST_LOAN", bankId, accNo, LoanType::CAR, 400000.0, 36);
    ASSERT_NE(l2, nullptr);
    ASSERT_NE(l1->loanId, l2->loanId);
    ASSERT_EQ(l2->status, LoanStatus::UNDER_LOAN_OFFICER);
}
