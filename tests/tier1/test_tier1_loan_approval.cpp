#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../auth.h"
#include "../../rbi.h"
#include "../../account.h"
#include "../../loan.h"
#include "../../utils.h"

using namespace BankingSystem;

static void setupTestEnv(Storage& store, AuthManager& auth, RBIController& rbi, AccountManager& acc) {
    Staff governor;
    governor.staffId = "GOV001";
    governor.role = Role::RBI_GOVERNOR;
    governor.passwordHash = Utils::hashPassword("gov");
    auth.addStaffMember(governor);

    Staff cmd;
    cmd.staffId = "CMD_SBI";
    cmd.role = Role::BANK_CMD;
    cmd.passwordHash = Utils::hashPassword("cmd");
    auth.addStaffMember(cmd);

    Staff zm;
    zm.staffId = "ZM_SBI";
    zm.role = Role::ZONAL_MANAGER;
    zm.passwordHash = Utils::hashPassword("zm");
    auth.addStaffMember(zm);

    Staff bm;
    bm.staffId = "BM_SBI";
    bm.role = Role::BRANCH_MANAGER;
    bm.passwordHash = Utils::hashPassword("bm");
    auth.addStaffMember(bm);

    Staff lo;
    lo.staffId = "LO_SBI";
    lo.role = Role::LOAN_OFFICER;
    lo.passwordHash = Utils::hashPassword("lo");
    auth.addStaffMember(lo);

    auth.loginStaff("GOV001", "gov");
    rbi.registerBank("State Bank of India", "SBI", "CMD_SBI");
    auth.logout();

    Customer c;
    c.customerId = "CUST1";
    c.name = "Test Applicant";
    c.creditScore = 780;
    c.isKycVerified = true;
    c.bankId = "SBI";
    c.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(c);

    auth.loginStaff("BM_SBI", "bm");
    acc.openAccount("CUST1", "SBI", "BR01", AccountType::SAVINGS, 100000.0);
    auth.logout();
}

TEST_CASE(test_loan_application_creation, "tier1") {
    std::string testDir = "_test_data_loan_1";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    LoanManager loanMgr(store, auth, rbi, acc);

    setupTestEnv(store, auth, rbi, acc);

    std::vector<Account> customerAccs = acc.getCustomerAccounts("CUST1");
    ASSERT_FALSE(customerAccs.empty());
    std::string accNo = customerAccs[0].accountNo;
    BankInfo* sbi = rbi.findBankByCode("SBI");
    ASSERT_TRUE(sbi != nullptr);

    Loan* l = loanMgr.applyForLoan("CUST1", sbi->bankId, accNo, LoanType::HOME, 1000000.0, 120);
    ASSERT_TRUE(l != nullptr);
    ASSERT_EQ(static_cast<int>(l->status), static_cast<int>(LoanStatus::UNDER_LOAN_OFFICER));
    ASSERT_ALMOST_EQ(l->principalAmount, 1000000.0, 0.01);
    ASSERT_TRUE(l->emiAmount > 0);
    ASSERT_TRUE(l->totalOutstanding > 0);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_loan_approval_small_amount_bm, "tier1") {
    std::string testDir = "_test_data_loan_2";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    LoanManager loanMgr(store, auth, rbi, acc);

    setupTestEnv(store, auth, rbi, acc);

    std::string accNo = acc.getCustomerAccounts("CUST1")[0].accountNo;
    std::string bankId = rbi.findBankByCode("SBI")->bankId;

    // Small loan < 25 Lakhs (10 Lakhs)
    Loan* l = loanMgr.applyForLoan("CUST1", bankId, accNo, LoanType::CAR, 1000000.0, 60);
    ASSERT_TRUE(l != nullptr);
    std::string loanId = l->loanId;

    // 1. Review by Loan Officer
    auth.loginStaff("LO_SBI", "lo");
    ASSERT_TRUE(loanMgr.reviewLoanOfficer(loanId, true, "LO approved"));
    auth.logout();

    Loan* lCheck1 = loanMgr.findLoan(loanId);
    ASSERT_EQ(static_cast<int>(lCheck1->status), static_cast<int>(LoanStatus::UNDER_BRANCH_MANAGER));

    // 2. Final Review by Branch Manager
    auth.loginStaff("BM_SBI", "bm");
    ASSERT_TRUE(loanMgr.reviewBranchManager(loanId, true, "BM approved"));
    auth.logout();

    Loan* lCheck2 = loanMgr.findLoan(loanId);
    ASSERT_EQ(static_cast<int>(lCheck2->status), static_cast<int>(LoanStatus::APPROVED));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_loan_approval_medium_amount_zm, "tier1") {
    std::string testDir = "_test_data_loan_3";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    LoanManager loanMgr(store, auth, rbi, acc);

    setupTestEnv(store, auth, rbi, acc);

    std::string accNo = acc.getCustomerAccounts("CUST1")[0].accountNo;
    std::string bankId = rbi.findBankByCode("SBI")->bankId;

    // Medium loan ₹50 Lakhs (between 25L and 1Cr)
    Loan* l = loanMgr.applyForLoan("CUST1", bankId, accNo, LoanType::HOME, 5000000.0, 180);
    ASSERT_TRUE(l != nullptr);
    std::string loanId = l->loanId;

    // Loan Officer approval escalates to Zonal Manager
    auth.loginStaff("LO_SBI", "lo");
    ASSERT_TRUE(loanMgr.reviewLoanOfficer(loanId, true, "LO approved"));
    auth.logout();

    Loan* lCheck1 = loanMgr.findLoan(loanId);
    ASSERT_EQ(static_cast<int>(lCheck1->status), static_cast<int>(LoanStatus::UNDER_ZONAL_MANAGER));

    // Zonal Manager approval sanctions the loan
    auth.loginStaff("ZM_SBI", "zm");
    ASSERT_TRUE(loanMgr.reviewZonalManager(loanId, true, "ZM approved"));
    auth.logout();

    Loan* lCheck2 = loanMgr.findLoan(loanId);
    ASSERT_EQ(static_cast<int>(lCheck2->status), static_cast<int>(LoanStatus::APPROVED));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_loan_approval_large_amount_cmd, "tier1") {
    std::string testDir = "_test_data_loan_4";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    LoanManager loanMgr(store, auth, rbi, acc);

    setupTestEnv(store, auth, rbi, acc);

    std::string accNo = acc.getCustomerAccounts("CUST1")[0].accountNo;
    std::string bankId = rbi.findBankByCode("SBI")->bankId;

    // Large loan > ₹1 Crore (₹1.5 Crores)
    Loan* l = loanMgr.applyForLoan("CUST1", bankId, accNo, LoanType::BUSINESS, 15000000.0, 240);
    ASSERT_TRUE(l != nullptr);
    std::string loanId = l->loanId;

    // Loan Officer approval escalates to Bank CMD
    auth.loginStaff("LO_SBI", "lo");
    ASSERT_TRUE(loanMgr.reviewLoanOfficer(loanId, true, "LO approved high value"));
    auth.logout();

    Loan* lCheck1 = loanMgr.findLoan(loanId);
    ASSERT_EQ(static_cast<int>(lCheck1->status), static_cast<int>(LoanStatus::UNDER_CMD));

    // CMD approval sanctions the loan
    auth.loginStaff("CMD_SBI", "cmd");
    ASSERT_TRUE(loanMgr.reviewCMD(loanId, true, "CMD approved high value"));
    auth.logout();

    Loan* lCheck2 = loanMgr.findLoan(loanId);
    ASSERT_EQ(static_cast<int>(lCheck2->status), static_cast<int>(LoanStatus::APPROVED));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_loan_disbursal_and_repayment, "tier1") {
    std::string testDir = "_test_data_loan_5";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    RBIController rbi(store, auth);
    AccountManager acc(store, auth);
    LoanManager loanMgr(store, auth, rbi, acc);

    setupTestEnv(store, auth, rbi, acc);

    Account* userAcc = &acc.getCustomerAccounts("CUST1")[0];
    std::string accNo = userAcc->accountNo;
    double initialAccBal = userAcc->balance; // 100,000
    std::string bankId = rbi.findBankByCode("SBI")->bankId;

    Loan* l = loanMgr.applyForLoan("CUST1", bankId, accNo, LoanType::PERSONAL, 200000.0, 24);
    std::string loanId = l->loanId;

    // Approve loan
    auth.loginStaff("LO_SBI", "lo");
    loanMgr.reviewLoanOfficer(loanId, true, "Approved");
    auth.logout();
    auth.loginStaff("BM_SBI", "bm");
    loanMgr.reviewBranchManager(loanId, true, "Approved");
    auth.logout();

    // Disburse loan -> credits principal to account
    ASSERT_TRUE(loanMgr.disburseLoan(loanId));
    Account* accAfterDisburse = acc.findAccount(accNo);
    ASSERT_ALMOST_EQ(accAfterDisburse->balance, initialAccBal + 200000.0, 0.01);

    Loan* lDisbursed = loanMgr.findLoan(loanId);
    ASSERT_EQ(static_cast<int>(lDisbursed->status), static_cast<int>(LoanStatus::DISBURSED));

    // Pay EMI
    double emi = lDisbursed->emiAmount;
    double oldOutstanding = lDisbursed->totalOutstanding;
    ASSERT_TRUE(loanMgr.payEMI(loanId, emi));

    Loan* lAfterEmi = loanMgr.findLoan(loanId);
    ASSERT_ALMOST_EQ(lAfterEmi->totalOutstanding, oldOutstanding - emi, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
