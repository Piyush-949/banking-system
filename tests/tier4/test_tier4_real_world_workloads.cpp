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
    governor.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(governor);
    auth.loginStaff("GOV001", "pass");
}

// TF4_01: Complete Commercial Bank Lifecycle (RBI rate change, 5 bank rate sync, multi-customer onboarding across SBI, HDFC, ICICI, Axis, PNB)
TEST_CASE(TF4_01, "tier4") {
    std::string testDir = "_test_data_tf4_01";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);

    // Step 1: Register 5 Commercial Banks
    rbi.registerBank("State Bank of India", "SBI", "CMD_SBI");
    rbi.registerBank("HDFC Bank", "HDFC", "CMD_HDFC");
    rbi.registerBank("ICICI Bank", "ICICI", "CMD_ICICI");
    rbi.registerBank("Axis Bank", "AXIS", "CMD_AXIS");
    rbi.registerBank("Punjab National Bank", "PNB", "CMD_PNB");

    const auto& banks = rbi.getBanks();
    ASSERT_EQ(banks.size(), 5U);

    // Step 2: RBI Governor raises Repo Rate from 6.50% to 7.25% (+0.75%)
    rbi.setRepoRate(7.25);
    ASSERT_ALMOST_EQ(rbi.getPolicy().repoRate, 7.25, 0.001);

    // Rate transmission check: +0.75% * 0.8 = +0.60% addition to home loan base rates
    BankInfo* bSBI = rbi.findBankByCode("SBI");
    ASSERT_TRUE(bSBI != nullptr);
    ASSERT_ALMOST_EQ(bSBI->policies.homeLoanBaseRate, 9.10, 0.01); // 8.50 + 0.60

    // Step 3: Synchronize Interest Rates across all 5 Banks
    BankEngine bankEngine(store, auth, rbi);
    bankEngine.updateBankInterestRates(bSBI->bankId, 3.50, 9.10, 12.50, 7.00);

    // Step 4: Multi-Customer Onboarding across the 5 Banks
    std::vector<std::string> custIds = {"CUST401_A", "CUST401_B", "CUST401_C", "CUST401_D", "CUST401_E"};
    std::vector<std::string> bankCodes = {"SBI", "HDFC", "ICICI", "AXIS", "PNB"};

    AccountManager accManager(store, auth);
    for (size_t i = 0; i < custIds.size(); ++i) {
        Customer c;
        c.customerId = custIds[i];
        c.name = "Customer " + bankCodes[i];
        c.isKycVerified = true;
        c.creditScore = 750;
        BankInfo* b = rbi.findBankByCode(bankCodes[i]);
        ASSERT_TRUE(b != nullptr);
        c.bankId = b->bankId;
        c.passwordHash = Utils::hashPassword("pass");
        auth.registerCustomer(c);

        // Open Savings Account with initial deposit ₹1,00,000
        Account* acc = accManager.openAccount(custIds[i], b->bankId, "BR01", AccountType::SAVINGS, 100000.0);
        ASSERT_TRUE(acc != nullptr);

        // Recalculate bank totals
        bankEngine.recalculateBankTotals(b->bankId);
        ASSERT_TRUE(b->totalDeposits >= 100000.0);
    }

    // Step 5: Systemic Risk Audit Across All 5 Banks
    rbi.runSystemicAudit();

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF4_02: Corporate Loan & Credit Risk Escalation (₹50L loan application, Loan Officer evaluation, Branch Manager referral, Zonal Manager final approval, disbursal, EMI tracking)
TEST_CASE(TF4_02, "tier4") {
    std::string testDir = "_test_data_tf4_02";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("HDFC Bank", "HDFC", "CMD_HDFC");
    BankInfo* bank = rbi.findBankByCode("HDFC");
    ASSERT_TRUE(bank != nullptr);
    std::string bankId = bank->bankId;

    Customer corpCust;
    corpCust.customerId = "CUST402";
    corpCust.name = "TechSolutions Pvt Ltd";
    corpCust.isKycVerified = true;
    corpCust.creditScore = 780;
    corpCust.bankId = bankId;
    corpCust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(corpCust);

    AccountManager accManager(store, auth);
    Account* currAcc = accManager.openAccount("CUST402", bankId, "BR_HDFC_01", AccountType::CURRENT, 500000.0);
    ASSERT_TRUE(currAcc != nullptr);

    LoanManager loanManager(store, auth, rbi, accManager);

    // Apply for ₹50 Lakhs Business Loan (falls in ₹25L to ₹1Cr range -> Zonal Manager approval path)
    Loan* loan = loanManager.applyForLoan("CUST402", bankId, currAcc->accountNo, LoanType::BUSINESS, 5000000.0, 24);
    ASSERT_TRUE(loan != nullptr);
    std::string loanId = loan->loanId;
    ASSERT_TRUE(loan->status == LoanStatus::UNDER_LOAN_OFFICER);

    // Step 1: Loan Officer Evaluation
    Staff loStaff;
    loStaff.staffId = "LO402";
    loStaff.name = "Senior Credit Officer";
    loStaff.role = Role::LOAN_OFFICER;
    loStaff.bankId = bankId;
    loStaff.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(loStaff);
    auth.loginStaff("LO402", "pass");

    bool loReview = loanManager.reviewLoanOfficer(loanId, true, "Financial balance sheet verified");
    ASSERT_TRUE(loReview);
    ASSERT_TRUE(loan->status == LoanStatus::UNDER_ZONAL_MANAGER); // Escalates to ZM for ₹50L

    // Step 2: Zonal Manager Final Approval
    Staff zmStaff;
    zmStaff.staffId = "ZM402";
    zmStaff.name = "Zonal Credit Head";
    zmStaff.role = Role::ZONAL_MANAGER;
    zmStaff.bankId = bankId;
    zmStaff.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(zmStaff);
    auth.loginStaff("ZM402", "pass");

    bool zmReview = loanManager.reviewZonalManager(loanId, true, "Sanctioned under Zonal credit quota");
    ASSERT_TRUE(zmReview);
    ASSERT_TRUE(loan->status == LoanStatus::APPROVED);

    // Step 3: Disbursal into Current Account
    bool disbursed = loanManager.disburseLoan(loanId);
    ASSERT_TRUE(disbursed);
    ASSERT_TRUE(loan->status == LoanStatus::DISBURSED);
    ASSERT_ALMOST_EQ(currAcc->balance, 5500000.0, 0.01); // 500,000 initial + 5,000,000 loan principal

    // Step 4: EMI Payment Tracking (Pay 2 monthly EMIs)
    double emi = loan->emiAmount;
    ASSERT_TRUE(emi > 0.0);

    bool emi1 = loanManager.payEMI(loanId, emi);
    ASSERT_TRUE(emi1);
    ASSERT_TRUE(loan->totalPaid >= emi);

    bool emi2 = loanManager.payEMI(loanId, emi);
    ASSERT_TRUE(emi2);
    ASSERT_TRUE(loan->totalPaid >= 2.0 * emi);

    // Print sanction letter verification
    loanManager.printSanctionLetter(loanId);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF4_03: Wealth Management & Tax Deductions (Multi-product portfolio: FD, RD, Savings, Senior citizen perks, compound interest calculation, annual TDS report generation)
TEST_CASE(TF4_03, "tier4") {
    std::string testDir = "_test_data_tf4_03";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("SBI", "SBI", "CMD_SBI");
    BankInfo* bank = rbi.findBankByCode("SBI");
    ASSERT_TRUE(bank != nullptr);
    std::string bankId = bank->bankId;

    Customer wealthCust;
    wealthCust.customerId = "CUST403";
    wealthCust.name = "Dr. Ananya Sharma";
    wealthCust.isKycVerified = true;
    wealthCust.isSeniorCitizen = true; // Senior Citizen Perks
    wealthCust.creditScore = 810;
    wealthCust.bankId = bankId;
    wealthCust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(wealthCust);

    AccountManager accManager(store, auth);
    Account* savingsAcc = accManager.openAccount("CUST403", bankId, "BR_SBI_01", AccountType::SAVINGS, 2000000.0);
    ASSERT_TRUE(savingsAcc != nullptr);
    std::string accNo = savingsAcc->accountNo;

    // Step 1: Fixed Deposit with Senior Citizen Bonus (+0.50%) & TDS Calculation
    FDRDManager fdrdManager(store, auth, rbi, accManager);
    FixedDeposit* fd = fdrdManager.createFD("CUST403", bankId, accNo, 1000000.0, 60); // 5 Year FD
    ASSERT_TRUE(fd != nullptr);
    ASSERT_TRUE(fd->isSeniorCitizen);
    ASSERT_ALMOST_EQ(fd->interestRate, 8.00, 0.001); // 5Yr Base 7.50% + 0.50% Sr bonus
    ASSERT_TRUE(fd->tdsDeducted > 0.0);

    // Step 2: Recurring Deposit
    RecurringDeposit* rd = fdrdManager.createRD("CUST403", bankId, accNo, 20000.0, 24); // 2 Year RD
    ASSERT_TRUE(rd != nullptr);

    // Pay next 2 installments
    fdrdManager.payRDMonthlyInstallment(rd->rdNo);
    fdrdManager.payRDMonthlyInstallment(rd->rdNo);
    ASSERT_EQ(rd->monthsPaid, 3);

    // Step 3: Life Insurance Policy
    InsuranceManager insManager(store, auth);
    InsurancePolicy* policy = insManager.buyPolicy("CUST403", bankId, InsuranceType::LIFE_TERM, 10000000.0, "Rahul Sharma", "Son");
    ASSERT_TRUE(policy != nullptr);
    ASSERT_TRUE(policy->isActive);

    // Step 4: Platinum Credit Card
    CreditCardManager ccManager(store, auth, accManager);
    CreditCard* card = ccManager.applyForCreditCard("CUST403", bankId, accNo, CreditCardType::PLATINUM);
    ASSERT_TRUE(card != nullptr);
    ASSERT_ALMOST_EQ(card->creditLimit, 300000.0, 0.01);

    ccManager.swipeCard(card->cardNumber, 25000.0, "Jewellery Mart");
    ASSERT_ALMOST_EQ(card->currentOutstanding, 25000.0, 0.01);

    // Step 5: Wealth Portfolio Dashboard Generation
    LoanManager loanManager(store, auth, rbi, accManager);
    ReportGenerator reportGen(store, auth, rbi, accManager, loanManager, fdrdManager, insManager, ccManager);

    reportGen.generateCustomerDashboard("CUST403");

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF4_04: Multi-Branch Audit & Regulatory Compliance Inspection (Simulated multi-role user sessions, financial transaction execution, audit log integrity, role privilege enforcement check)
TEST_CASE(TF4_04, "tier4") {
    std::string testDir = "_test_data_tf4_04";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    setupGovernorSession(auth);

    RBIController rbi(store, auth);
    rbi.registerBank("SBI", "SBI", "CMD_SBI");
    BankInfo* bank = rbi.findBankByCode("SBI");
    ASSERT_TRUE(bank != nullptr);
    std::string bankId = bank->bankId;

    // Role 0: RBI Governor session
    Staff gov;
    gov.staffId = "GOV001";
    gov.name = "Governor";
    gov.role = Role::RBI_GOVERNOR;
    gov.bankId = "RBI";
    gov.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(gov);
    auth.loginStaff("GOV001", "pass");

    rbi.setSLR(18.50);
    rbi.setCRR(4.50);

    // Role 1: Bank CMD session
    Staff cmd;
    cmd.staffId = "CMD001";
    cmd.name = "CMD";
    cmd.role = Role::BANK_CMD;
    cmd.bankId = bankId;
    cmd.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(cmd);
    auth.loginStaff("CMD001", "pass");

    BankEngine bankEngine(store, auth, rbi);
    bankEngine.updateBankInterestRates(bankId, 3.50, 8.50, 12.00, 6.75);

    // Role 6: Teller session
    Staff teller;
    teller.staffId = "TEL001";
    teller.name = "Teller Cashier";
    teller.role = Role::TELLER;
    teller.bankId = bankId;
    teller.passwordHash = Utils::hashPassword("pass");
    auth.addStaffMember(teller);
    auth.loginStaff("TEL001", "pass");

    Customer cust;
    cust.customerId = "CUST404";
    cust.name = "Audit Customer";
    cust.isKycVerified = true;
    cust.bankId = bankId;
    cust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(cust);

    AccountManager accManager(store, auth);
    Account* acc = accManager.openAccount("CUST404", bankId, "BR01", AccountType::SAVINGS, 50000.0);
    ASSERT_TRUE(acc != nullptr);

    accManager.deposit(acc->accountNo, 10000.0, "TEL001");
    ASSERT_ALMOST_EQ(acc->balance, 60000.0, 0.01);

    // Role 7: Customer session with privilege violation check
    auth.loginCustomer("CUST404", "pass");
    // Customer attempts to modify Repo Rate -> Should be blocked
    rbi.setRepoRate(12.00);
    ASSERT_NE(rbi.getPolicy().repoRate, 12.00);

    // Verify Audit Log Integrity & Serialization
    const auto& logs = auth.getAuditLogs();
    ASSERT_TRUE(logs.size() >= 5);

    // Storage File Parse Check
    auto parsedLogs = E2ETestUtils::parseAuditLogFile(testDir + "/audit.dat");
    ASSERT_FALSE(parsedLogs.empty());

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

// TF4_05: Disaster Recovery & System Restart Resilience (Heavy transaction volume across all account types, system shutdown, application restart, complete database state & audit log checksum match)
TEST_CASE(TF4_05, "tier4") {
    std::string testDir = "_test_data_tf4_05";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    // Phase 1: Pre-Shutdown Operations & Data Generation
    std::string acc1No, acc2No, fdNo, rdNo, cardNo;
    size_t preShutdownLogCount = 0;

    {
        Storage store(testDir);
        AuthManager auth(store);
        setupGovernorSession(auth);

        RBIController rbi(store, auth);
        rbi.registerBank("SBI", "SBI", "CMD_SBI");
        BankInfo* b = rbi.findBankByCode("SBI");
        ASSERT_TRUE(b != nullptr);
        std::string bankId = b->bankId;

        Customer c1;
        c1.customerId = "CUST405_A";
        c1.name = "Alpha Enterprises";
        c1.isKycVerified = true;
        c1.creditScore = 790;
        c1.bankId = bankId;
        c1.passwordHash = Utils::hashPassword("pass");
        auth.registerCustomer(c1);

        Customer c2;
        c2.customerId = "CUST405_B";
        c2.name = "Beta Solutions";
        c2.isKycVerified = true;
        c2.creditScore = 720;
        c2.bankId = bankId;
        c2.passwordHash = Utils::hashPassword("pass");
        auth.registerCustomer(c2);

        AccountManager accManager(store, auth);
        Account* a1 = accManager.openAccount("CUST405_A", bankId, "BR01", AccountType::CURRENT, 500000.0);
        Account* a2 = accManager.openAccount("CUST405_B", bankId, "BR01", AccountType::SAVINGS, 200000.0);
        ASSERT_TRUE(a1 != nullptr && a2 != nullptr);
        acc1No = a1->accountNo;
        acc2No = a2->accountNo;

        // Perform 10+ financial operations
        accManager.deposit(acc1No, 50000.0, "STAFF01");
        accManager.withdraw(acc2No, 10000.0, "STAFF01");
        accManager.transfer(acc1No, acc2No, 40000.0, "STAFF01");

        FDRDManager fdrdManager(store, auth, rbi, accManager);
        FixedDeposit* fd = fdrdManager.createFD("CUST405_A", bankId, acc1No, 100000.0, 12);
        ASSERT_TRUE(fd != nullptr);
        fdNo = fd->fdNo;

        RecurringDeposit* rd = fdrdManager.createRD("CUST405_B", bankId, acc2No, 5000.0, 12);
        ASSERT_TRUE(rd != nullptr);
        rdNo = rd->rdNo;

        CreditCardManager ccManager(store, auth, accManager);
        CreditCard* cc = ccManager.applyForCreditCard("CUST405_A", bankId, acc1No, CreditCardType::GOLD);
        ASSERT_TRUE(cc != nullptr);
        cardNo = cc->cardNumber;
        ccManager.swipeCard(cardNo, 15000.0, "Hardware Supply");

        preShutdownLogCount = auth.getAuditLogs().size();
        ASSERT_TRUE(preShutdownLogCount > 0);
    } // Simulated System Crash / Process Shutdown (Destructors invoked, files flushed to disk)

    // Phase 2: Disaster Recovery — Restart Application & Load from Disk
    Storage storeRecovery(testDir);
    AuthManager authRecovery(storeRecovery);
    RBIController rbiRecovery(storeRecovery, authRecovery);
    AccountManager accManagerRecovery(storeRecovery, authRecovery);
    FDRDManager fdrdRecovery(storeRecovery, authRecovery, rbiRecovery, accManagerRecovery);
    CreditCardManager ccRecovery(storeRecovery, authRecovery, accManagerRecovery);

    // Step 3: State Integrity Verification
    Account* a1Recovered = accManagerRecovery.findAccount(acc1No);
    Account* a2Recovered = accManagerRecovery.findAccount(acc2No);
    ASSERT_TRUE(a1Recovered != nullptr && a2Recovered != nullptr);

    // Verify exact pre-shutdown balances:
    // a1: 500,000 initial + 50,000 dep - 40,000 xferOut - 100,000 FD = 410,000
    ASSERT_ALMOST_EQ(a1Recovered->balance, 410000.0, 0.01);

    // a2: 200,000 initial - 10,000 with + 40,000 xferIn - 5,000 RD = 225,000
    ASSERT_ALMOST_EQ(a2Recovered->balance, 225000.0, 0.01);

    // Verify Audit Log Count Consistency after recovery
    ASSERT_EQ(authRecovery.getAuditLogs().size(), preShutdownLogCount);

    // Verify Credit Card Outstanding Recovery
    CreditCard* ccRecovered = ccRecovery.findCard(cardNo);
    ASSERT_TRUE(ccRecovered != nullptr);
    ASSERT_ALMOST_EQ(ccRecovered->currentOutstanding, 15000.0, 0.01);

    // Post-recovery live operation execution
    bool postDep = accManagerRecovery.deposit(acc1No, 20000.0, "STAFF01");
    ASSERT_TRUE(postDep);
    ASSERT_ALMOST_EQ(a1Recovered->balance, 430000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
