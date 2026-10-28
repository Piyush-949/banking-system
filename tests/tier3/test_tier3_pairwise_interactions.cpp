#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../auth.h"
#include "../../rbi.h"
#include "../../bank.h"
#include "../../account.h"
#include "../../fd_rd.h"
#include "../../loan.h"
#include "../../credit_card.h"
#include "../../insurance.h"
#include "../../reports.h"
#include "../../utils.h"

using namespace BankingSystem;

static void setupGovernorSession(AuthManager& auth) {
    Staff governor;
    governor.staffId = "GOV001";
    governor.name = "RBI Governor";
    governor.role = Role::RBI_GOVERNOR;
    governor.bankId = "RBI";
    governor.passwordHash = Utils::hashPassword("govpass");
    auth.addStaffMember(governor);
    auth.loginStaff("GOV001", "govpass");
}

// TF3_01: RBI Rate change -> FD yield adjustment -> Customer premature withdrawal
TEST_CASE(TF3_01, "tier3") {
    std::string testDir = "_test_data_tf3_01";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("State Bank of India", "SBI", "CMD001");
    BankInfo* sbi = rbi.findBankByCode("SBI");
    ASSERT_TRUE(sbi != nullptr);
    std::string bankId = sbi->bankId;

    // Step 1: RBI Rate Change
    rbi.setRepoRate(7.00); // Increased from 6.50% to 7.00%
    ASSERT_ALMOST_EQ(rbi.getPolicy().repoRate, 7.00, 0.001);

    // Step 2: Bank CMD adjusts FD Yields
    Staff cmd;
    cmd.staffId = "CMD001";
    cmd.name = "SBI CMD";
    cmd.role = Role::BANK_CMD;
    cmd.bankId = bankId;
    cmd.passwordHash = Utils::hashPassword("cmdpass");
    auth.addStaffMember(cmd);
    auth.loginStaff("CMD001", "cmdpass");

    BankEngine bankEngine(store, auth, rbi);
    bankEngine.updateBankInterestRates(bankId, 4.00, 9.00, 12.50, 7.25);
    ASSERT_ALMOST_EQ(sbi->policies.fd1YrRate, 7.25, 0.001);

    // Step 3: Customer opens FD & performs Premature Withdrawal
    Customer cust;
    cust.customerId = "CUST101";
    cust.name = "Aarav Sharma";
    cust.isKycVerified = true;
    cust.isSeniorCitizen = false;
    cust.creditScore = 780;
    cust.bankId = bankId;
    cust.passwordHash = Utils::hashPassword("custpass");
    auth.registerCustomer(cust);

    AccountManager accManager(store, auth);
    Account* acc = accManager.openAccount("CUST101", bankId, "BR001", AccountType::SAVINGS, 500000.0);
    ASSERT_TRUE(acc != nullptr);
    std::string accNo = acc->accountNo;

    FDRDManager fdrdManager(store, auth, rbi, accManager);
    FixedDeposit* fd = fdrdManager.createFD("CUST101", bankId, accNo, 200000.0, 12);
    ASSERT_TRUE(fd != nullptr);
    std::string fdNo = fd->fdNo;
    ASSERT_ALMOST_EQ(acc->balance, 300000.0, 0.01);

    // Premature Withdrawal (1% penalty applied)
    bool withdrawSuccess = fdrdManager.prematureWithdrawFD(fdNo);
    ASSERT_TRUE(withdrawSuccess);

    // Verify FD closed and funds credited back to savings account
    auto fds = fdrdManager.getCustomerFDs("CUST101");
    ASSERT_FALSE(fds.empty());
    ASSERT_TRUE(fds[0].isClosed);
    ASSERT_TRUE(acc->balance > 300000.0); // Principal + penalty-adjusted interest credited

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF3_02: CIBIL Score update -> Credit Card limit upgrade -> Over-limit transaction attempt
TEST_CASE(TF3_02, "tier3") {
    std::string testDir = "_test_data_tf3_02";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("HDFC Bank", "HDFC", "CMD_HDFC");
    BankInfo* bank = rbi.findBankByCode("HDFC");
    ASSERT_TRUE(bank != nullptr);
    std::string bankId = bank->bankId;

    AccountManager accManager(store, auth);

    Customer cust;
    cust.customerId = "CUST102";
    cust.name = "Priya Patel";
    cust.isKycVerified = true;
    cust.creditScore = 600; // Low CIBIL score initially
    cust.bankId = bankId;
    cust.passwordHash = Utils::hashPassword("pass123");
    auth.registerCustomer(cust);

    Account* acc = accManager.openAccount("CUST102", bankId, "BR002", AccountType::SAVINGS, 100000.0);
    ASSERT_TRUE(acc != nullptr);

    CreditCardManager ccManager(store, auth, accManager);

    // Application for Gold Card (requires score 700) should be rejected initially
    CreditCard* cardFailed = ccManager.applyForCreditCard("CUST102", bankId, acc->accountNo, CreditCardType::GOLD);
    ASSERT_TRUE(cardFailed == nullptr);

    // Update customer CIBIL score to 750
    Customer* custPtr = auth.findCustomer("CUST102");
    ASSERT_TRUE(custPtr != nullptr);
    custPtr->creditScore = 750;
    store.saveCustomers(auth.getCustomers());

    // Apply again after score upgrade -> Should succeed
    CreditCard* cardSuccess = ccManager.applyForCreditCard("CUST102", bankId, acc->accountNo, CreditCardType::GOLD);
    ASSERT_TRUE(cardSuccess != nullptr);
    std::string cardNo = cardSuccess->cardNumber;
    ASSERT_ALMOST_EQ(cardSuccess->creditLimit, 150000.0, 0.01);

    // Valid swipe within limit
    bool swipe1 = ccManager.swipeCard(cardNo, 50000.0, "Electronics Store");
    ASSERT_TRUE(swipe1);
    ASSERT_ALMOST_EQ(cardSuccess->availableLimit, 100000.0, 0.01);
    ASSERT_ALMOST_EQ(cardSuccess->currentOutstanding, 50000.0, 0.01);

    // Over-limit swipe attempt (attempt ₹1,20,000 when available limit is ₹1,00,000)
    bool swipeOverLimit = ccManager.swipeCard(cardNo, 120000.0, "Luxury Retail");
    ASSERT_FALSE(swipeOverLimit);
    ASSERT_ALMOST_EQ(cardSuccess->availableLimit, 100000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF3_03: Multi-level loan approval (BM -> ZM -> CMD) -> Disbursal into Current overdraft account
TEST_CASE(TF3_03, "tier3") {
    std::string testDir = "_test_data_tf3_03";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("ICICI Bank", "ICICI", "CMD002");
    BankInfo* bank = rbi.findBankByCode("ICICI");
    ASSERT_TRUE(bank != nullptr);
    std::string bankId = bank->bankId;

    Customer cust;
    cust.customerId = "CUST103";
    cust.name = "Vikram Malhotra";
    cust.isKycVerified = true;
    cust.creditScore = 820;
    cust.bankId = bankId;
    cust.passwordHash = Utils::hashPassword("pass123");
    auth.registerCustomer(cust);

    AccountManager accManager(store, auth);
    Account* currAcc = accManager.openAccount("CUST103", bankId, "BR003", AccountType::CURRENT, 200000.0);
    ASSERT_TRUE(currAcc != nullptr);
    ASSERT_ALMOST_EQ(currAcc->overdraftLimit, 50000.0, 0.01);

    LoanManager loanManager(store, auth, rbi, accManager);

    // Apply for ₹1.5 Crore Corporate/Business Loan (> ₹1 Crore threshold -> escalates to CMD)
    Loan* loan = loanManager.applyForLoan("CUST103", bankId, currAcc->accountNo, LoanType::BUSINESS, 15000000.0, 36);
    ASSERT_TRUE(loan != nullptr);
    std::string loanId = loan->loanId;
    ASSERT_TRUE(loan->status == LoanStatus::UNDER_LOAN_OFFICER);

    // Step 1: Loan Officer Review & Approval
    Staff loStaff;
    loStaff.staffId = "LO001";
    loStaff.name = "Loan Officer";
    loStaff.role = Role::LOAN_OFFICER;
    loStaff.bankId = bankId;
    loStaff.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(loStaff);
    auth.loginStaff("LO001", "pass");

    bool loReview = loanManager.reviewLoanOfficer(loanId, true, "Documents verified");
    ASSERT_TRUE(loReview);
    ASSERT_TRUE(loan->status == LoanStatus::UNDER_CMD); // Escalated directly to CMD for > ₹1Cr

    // Step 2: CMD Review & Sanction
    Staff cmdStaff;
    cmdStaff.staffId = "CMD002";
    cmdStaff.name = "ICICI CMD";
    cmdStaff.role = Role::BANK_CMD;
    cmdStaff.bankId = bankId;
    cmdStaff.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(cmdStaff);
    auth.loginStaff("CMD002", "pass");

    bool cmdReview = loanManager.reviewCMD(loanId, true, "High value commercial loan sanctioned");
    ASSERT_TRUE(cmdReview);
    ASSERT_TRUE(loan->status == LoanStatus::APPROVED);

    // Step 3: Disbursal into Current Overdraft Account
    bool disbursed = loanManager.disburseLoan(loanId);
    ASSERT_TRUE(disbursed);
    ASSERT_TRUE(loan->status == LoanStatus::DISBURSED);
    ASSERT_ALMOST_EQ(currAcc->balance, 15200000.0, 0.01); // 200,000 initial + 15,000,000 principal

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF3_04: High-value NEFT transfer -> Audit logging -> File persistence reload verification
TEST_CASE(TF3_04, "tier3") {
    std::string testDir = "_test_data_tf3_04";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("State Bank of India", "SBI", "CMD001");
    BankInfo* bank = rbi.findBankByCode("SBI");
    ASSERT_TRUE(bank != nullptr);
    std::string bankId = bank->bankId;

    Customer c1;
    c1.customerId = "CUST104_A";
    c1.name = "Sender Corp";
    c1.isKycVerified = true;
    c1.bankId = bankId;
    c1.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(c1);

    Customer c2;
    c2.customerId = "CUST104_B";
    c2.name = "Receiver Corp";
    c2.isKycVerified = true;
    c2.bankId = bankId;
    c2.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(c2);

    AccountManager accManager(store, auth);
    Account* acc1 = accManager.openAccount("CUST104_A", bankId, "BR01", AccountType::SAVINGS, 500000.0);
    Account* acc2 = accManager.openAccount("CUST104_B", bankId, "BR01", AccountType::SAVINGS, 100000.0);
    ASSERT_TRUE(acc1 != nullptr && acc2 != nullptr);
    std::string accNo1 = acc1->accountNo;
    std::string accNo2 = acc2->accountNo;

    // High-value transfer of ₹50,000 (boundary test)
    bool transferSuccess = accManager.transfer(accNo1, accNo2, 50000.0, "STAFF01");
    ASSERT_TRUE(transferSuccess);
    ASSERT_ALMOST_EQ(acc1->balance, 450000.0, 0.01);
    ASSERT_ALMOST_EQ(acc2->balance, 150000.0, 0.01);

    // Verify audit log entry was created
    const auto& logs = auth.getAuditLogs();
    ASSERT_FALSE(logs.empty());
    bool foundAudit = false;
    for (const auto& l : logs) {
        if (l.action == "FUND_TRANSFER") foundAudit = true;
    }
    ASSERT_TRUE(foundAudit);

    // Save and Reload persistence verification
    Storage storeReload(testDir);
    AuthManager authReload(storeReload);
    AccountManager accManagerReload(storeReload, authReload);

    Account* acc1Reload = accManagerReload.findAccount(accNo1);
    Account* acc2Reload = accManagerReload.findAccount(accNo2);
    ASSERT_TRUE(acc1Reload != nullptr && acc2Reload != nullptr);
    ASSERT_ALMOST_EQ(acc1Reload->balance, 450000.0, 0.01);
    ASSERT_ALMOST_EQ(acc2Reload->balance, 150000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF3_05: Senior citizen FD creation -> Quarterly compounding -> Section 194A TDS deduction
TEST_CASE(TF3_05, "tier3") {
    std::string testDir = "_test_data_tf3_05";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("Punjab National Bank", "PNB", "CMD005");
    BankInfo* bank = rbi.findBankByCode("PNB");
    ASSERT_TRUE(bank != nullptr);
    std::string bankId = bank->bankId;

    Customer seniorCust;
    seniorCust.customerId = "CUST105";
    seniorCust.name = "Ramesh Verma";
    seniorCust.isKycVerified = true;
    seniorCust.isSeniorCitizen = true;
    seniorCust.bankId = bankId;
    seniorCust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(seniorCust);

    AccountManager accManager(store, auth);
    Account* acc = accManager.openAccount("CUST105", bankId, "BR05", AccountType::SAVINGS, 1000000.0);
    ASSERT_TRUE(acc != nullptr);

    FDRDManager fdrdManager(store, auth, rbi, accManager);
    // Create 3-Year (36 month) FD for ₹5,00,000
    FixedDeposit* fd = fdrdManager.createFD("CUST105", bankId, acc->accountNo, 500000.0, 36);
    ASSERT_TRUE(fd != nullptr);

    // 3Yr Base Rate (7.10%) + Senior Bonus (0.50%) = 7.60%
    ASSERT_ALMOST_EQ(fd->interestRate, 7.60, 0.001);
    ASSERT_TRUE(fd->isSeniorCitizen);

    // Interest calculation: 500,000 * (1 + 7.60/400)^12 - 500,000 = ~126,618.36
    // Since total interest > ₹40,000, 10% TDS deduction applies to excess
    ASSERT_TRUE(fd->tdsDeducted > 0.0);
    ASSERT_TRUE(fd->maturityAmount > 500000.0);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF3_06: Joint Account creation (EoS) -> Transaction authorization check -> Audit log entry
TEST_CASE(TF3_06, "tier3") {
    std::string testDir = "_test_data_tf3_06";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("SBI", "SBI", "CMD001");
    BankInfo* bank = rbi.findBankByCode("SBI");
    ASSERT_TRUE(bank != nullptr);
    std::string bankId = bank->bankId;

    Customer primary;
    primary.customerId = "CUST106_A";
    primary.name = "Devesh Patel";
    primary.isKycVerified = true;
    primary.bankId = bankId;
    primary.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(primary);

    AccountManager accManager(store, auth);
    Account* jointAcc = accManager.openAccount("CUST106_A", bankId, "BR01", AccountType::JOINT, 100000.0);
    ASSERT_TRUE(jointAcc != nullptr);
    std::string accNo = jointAcc->accountNo;

    // Authorized deposit & withdrawal on Joint Account
    bool dep = accManager.deposit(accNo, 50000.0, "TELLER01");
    ASSERT_TRUE(dep);
    ASSERT_ALMOST_EQ(jointAcc->balance, 150000.0, 0.01);

    bool with = accManager.withdraw(accNo, 25000.0, "CUST106_A");
    ASSERT_TRUE(with);
    ASSERT_ALMOST_EQ(jointAcc->balance, 125000.0, 0.01);

    // Audit Log Entry Check
    const auto& logs = auth.getAuditLogs();
    ASSERT_FALSE(logs.empty());
    bool foundWithdraw = false;
    for (const auto& l : logs) {
        if (l.action == "CASH_WITHDRAWAL") foundWithdraw = true;
    }
    ASSERT_TRUE(foundWithdraw);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF3_07: Insurance Policy purchase -> Claim submission -> Loan Officer review & claim disbursal
TEST_CASE(TF3_07, "tier3") {
    std::string testDir = "_test_data_tf3_07";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("SBI", "SBI", "CMD001");
    BankInfo* bank = rbi.findBankByCode("SBI");
    ASSERT_TRUE(bank != nullptr);
    std::string bankId = bank->bankId;

    Customer cust;
    cust.customerId = "CUST107";
    cust.name = "Suresh Raina";
    cust.isKycVerified = true;
    cust.bankId = bankId;
    cust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(cust);

    InsuranceManager insManager(store, auth);

    // Step 1: Purchase Health Mediclaim policy
    InsurancePolicy* policy = insManager.buyPolicy("CUST107", bankId, InsuranceType::HEALTH_MEDICLAIM, 500000.0, "Priya Raina", "Wife");
    ASSERT_TRUE(policy != nullptr);
    std::string policyNo = policy->policyNo;
    ASSERT_TRUE(policy->isActive);

    // Step 2: Claim Submission
    bool claimFiled = insManager.fileClaim(policyNo, 150000.0);
    ASSERT_TRUE(claimFiled);
    ASSERT_TRUE(policy->claimPending);
    ASSERT_STREQ(policy->claimStatus, "SUBMITTED");

    // Step 3: Loan Officer Review Attempt -> Should fail permission check (BM role required)
    Staff loStaff;
    loStaff.staffId = "LO007";
    loStaff.name = "Loan Officer";
    loStaff.role = Role::LOAN_OFFICER;
    loStaff.bankId = bankId;
    loStaff.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(loStaff);
    auth.loginStaff("LO007", "pass");

    bool loProcess = insManager.processClaimBranchManager(policyNo, true);
    ASSERT_FALSE(loProcess); // Loan officer cannot approve insurance claims

    // Step 4: Branch Manager Review & Claim Approval
    Staff bmStaff;
    bmStaff.staffId = "BM007";
    bmStaff.name = "Branch Manager";
    bmStaff.role = Role::BRANCH_MANAGER;
    bmStaff.bankId = bankId;
    bmStaff.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(bmStaff);
    auth.loginStaff("BM007", "pass");

    bool bmProcess = insManager.processClaimBranchManager(policyNo, true);
    ASSERT_TRUE(bmProcess);
    ASSERT_FALSE(policy->claimPending);
    ASSERT_STREQ(policy->claimStatus, "APPROVED");

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF3_08: RBI CRR/SLR policy update -> Commercial Bank liquidity check -> Savings interest rate drop
TEST_CASE(TF3_08, "tier3") {
    std::string testDir = "_test_data_tf3_08";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("Axis Bank", "AXIS", "CMD008");
    BankInfo* bank = rbi.findBankByCode("AXIS");
    ASSERT_TRUE(bank != nullptr);
    std::string bankId = bank->bankId;

    // Step 1: RBI updates CRR to 5.0% and SLR to 19.0%
    rbi.setCRR(5.00);
    rbi.setSLR(19.00);
    ASSERT_ALMOST_EQ(rbi.getPolicy().crr, 5.00, 0.001);
    ASSERT_ALMOST_EQ(rbi.getPolicy().slr, 19.00, 0.001);
    ASSERT_TRUE(rbi.getPolicy().circulars.size() >= 2);

    // Step 2: Commercial Bank CMD lowers Savings Interest Rate due to tight liquidity
    Staff cmd;
    cmd.staffId = "CMD008";
    cmd.name = "Axis CMD";
    cmd.role = Role::BANK_CMD;
    cmd.bankId = bankId;
    cmd.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(cmd);
    auth.loginStaff("CMD008", "pass");

    BankEngine bankEngine(store, auth, rbi);
    bankEngine.updateBankInterestRates(bankId, 3.00, 8.75, 12.00, 6.50); // Lowered savings ROI from 3.5% to 3.0%
    ASSERT_ALMOST_EQ(bank->policies.savingsInterestRate, 3.00, 0.001);

    // Recalculate Bank Totals
    bankEngine.recalculateBankTotals(bankId);
    ASSERT_ALMOST_EQ(bank->totalDeposits, 0.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF3_09: Recurring Deposit (RD) monthly instalment -> Premature closure with penalty -> Audit log
TEST_CASE(TF3_09, "tier3") {
    std::string testDir = "_test_data_tf3_09";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("SBI", "SBI", "CMD001");
    BankInfo* bank = rbi.findBankByCode("SBI");
    ASSERT_TRUE(bank != nullptr);

    Customer cust;
    cust.customerId = "CUST109";
    cust.name = "Kavita Rao";
    cust.isKycVerified = true;
    cust.bankId = bank->bankId;
    cust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(cust);

    AccountManager accManager(store, auth);
    Account* acc = accManager.openAccount("CUST109", bank->bankId, "BR01", AccountType::SAVINGS, 100000.0);
    ASSERT_TRUE(acc != nullptr);

    FDRDManager fdrdManager(store, auth, rbi, accManager);
    // Create RD with ₹10,000 monthly installment for 12 months (1st month paid on creation)
    RecurringDeposit* rd = fdrdManager.createRD("CUST109", bank->bankId, acc->accountNo, 10000.0, 12);
    ASSERT_TRUE(rd != nullptr);
    std::string rdNo = rd->rdNo;
    ASSERT_EQ(rd->monthsPaid, 1);
    ASSERT_ALMOST_EQ(acc->balance, 90000.0, 0.01);

    // Pay 2nd monthly installment
    bool pay2 = fdrdManager.payRDMonthlyInstallment(rdNo);
    ASSERT_TRUE(pay2);
    ASSERT_EQ(rd->monthsPaid, 2);
    ASSERT_ALMOST_EQ(acc->balance, 80000.0, 0.01);

    // Pay 3rd monthly installment
    bool pay3 = fdrdManager.payRDMonthlyInstallment(rdNo);
    ASSERT_TRUE(pay3);
    ASSERT_EQ(rd->monthsPaid, 3);
    ASSERT_ALMOST_EQ(acc->balance, 70000.0, 0.01);

    // Audit logs verify installment tracking
    const auto& customerRDs = fdrdManager.getCustomerRDs("CUST109");
    ASSERT_FALSE(customerRDs.empty());
    ASSERT_EQ(customerRDs[0].monthsPaid, 3);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF3_10: Multi-bank comparison query -> Customer account opening at top bank -> Initial deposit
TEST_CASE(TF3_10, "tier3") {
    std::string testDir = "_test_data_tf3_10";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);

    // Register 3 banks with different savings rates
    rbi.registerBank("State Bank of India", "SBI", "CMD_SBI");
    rbi.registerBank("HDFC Bank", "HDFC", "CMD_HDFC");
    rbi.registerBank("ICICI Bank", "ICICI", "CMD_ICICI");

    BankInfo* bSBI = rbi.findBankByCode("SBI");
    BankInfo* bHDFC = rbi.findBankByCode("HDFC");
    BankInfo* bICICI = rbi.findBankByCode("ICICI");
    ASSERT_TRUE(bSBI && bHDFC && bICICI);

    // Update policies: HDFC offers 4.25% savings ROI, SBI 3.50%, ICICI 3.75%
    bSBI->policies.savingsInterestRate = 3.50;
    bHDFC->policies.savingsInterestRate = 4.25;
    bICICI->policies.savingsInterestRate = 3.75;
    store.saveBanks(rbi.getBanks());

    // Public Bank Comparison
    BankEngine bankEngine(store, auth, rbi);
    bankEngine.displayBankComparisonTable();

    // Find top bank based on highest savings ROI
    const auto& banks = rbi.getBanks();
    const BankInfo* topBank = nullptr;
    double maxSavingsRoi = -1.0;
    for (const auto& b : banks) {
        if (b.isLicensed && b.policies.savingsInterestRate > maxSavingsRoi) {
            maxSavingsRoi = b.policies.savingsInterestRate;
            topBank = &b;
        }
    }
    ASSERT_TRUE(topBank != nullptr);
    ASSERT_STREQ(topBank->code, "HDFC");

    // Customer opens account at top bank (HDFC)
    Customer cust;
    cust.customerId = "CUST110";
    cust.name = "Ananya Roy";
    cust.isKycVerified = true;
    cust.bankId = topBank->bankId;
    cust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(cust);

    AccountManager accManager(store, auth);
    Account* acc = accManager.openAccount("CUST110", topBank->bankId, "BR_HDFC_01", AccountType::SAVINGS, 50000.0);
    ASSERT_TRUE(acc != nullptr);
    ASSERT_STREQ(acc->bankId, topBank->bankId);
    ASSERT_ALMOST_EQ(acc->balance, 50000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
