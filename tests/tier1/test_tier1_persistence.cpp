#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../utils.h"

using namespace BankingSystem;

TEST_CASE(test_persistence_data_seeding, "tier1") {
    std::string testDir = "_test_data_persist_1";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);

    RBIPolicy pol;
    pol.repoRate = 6.50;
    pol.reverseRepoRate = 6.25;
    pol.crr = 4.50;
    pol.slr = 18.00;
    pol.lastUpdatedDate = "2026-08-09";
    pol.circulars.push_back("Initial Seed Policy");
    store.saveRBIPolicy(pol);

    RBIPolicy loadedPol = store.loadRBIPolicy();
    ASSERT_ALMOST_EQ(loadedPol.repoRate, 6.50, 0.001);
    ASSERT_ALMOST_EQ(loadedPol.reverseRepoRate, 6.25, 0.001);
    ASSERT_FALSE(loadedPol.circulars.empty());

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_persistence_save_load_customers_and_staff, "tier1") {
    std::string testDir = "_test_data_persist_2";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);

    // Create & save customers
    std::vector<Customer> custs;
    Customer c1;
    c1.customerId = "CUST_P1";
    c1.name = "Alice Persist";
    c1.pan = "ABCDE1111A";
    c1.creditScore = 750;
    c1.isKycVerified = true;
    c1.bankId = "SBI";
    custs.push_back(c1);
    store.saveCustomers(custs);

    // Create & save staff
    std::vector<Staff> staffList;
    Staff s1;
    s1.staffId = "STAFF_P1";
    s1.name = "Bob Staff";
    s1.role = Role::BRANCH_MANAGER;
    s1.bankId = "SBI";
    staffList.push_back(s1);
    store.saveStaff(staffList);

    // Reload from new Storage instance
    Storage storeReloaded(testDir);
    std::vector<Customer> loadedCusts = storeReloaded.loadCustomers();
    std::vector<Staff> loadedStaff = storeReloaded.loadStaff();

    ASSERT_EQ(loadedCusts.size(), static_cast<size_t>(1));
    ASSERT_STREQ(loadedCusts[0].customerId, "CUST_P1");
    ASSERT_STREQ(loadedCusts[0].name, "Alice Persist");

    ASSERT_EQ(loadedStaff.size(), static_cast<size_t>(1));
    ASSERT_STREQ(loadedStaff[0].staffId, "STAFF_P1");
    ASSERT_EQ(static_cast<int>(loadedStaff[0].role), static_cast<int>(Role::BRANCH_MANAGER));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_persistence_save_load_accounts_and_loans, "tier1") {
    std::string testDir = "_test_data_persist_3";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);

    std::vector<Account> accs;
    Account a;
    a.accountNo = "ACC_P100";
    a.customerId = "CUST_P1";
    a.bankId = "SBI";
    a.type = AccountType::SAVINGS;
    a.balance = 55000.0;
    a.isActive = true;
    accs.push_back(a);
    store.saveAccounts(accs);

    std::vector<Loan> loans;
    Loan l;
    l.loanId = "LON_P200";
    l.customerId = "CUST_P1";
    l.bankId = "SBI";
    l.principalAmount = 500000.0;
    l.status = LoanStatus::UNDER_BRANCH_MANAGER;
    loans.push_back(l);
    store.saveLoans(loans);

    Storage storeReloaded(testDir);
    std::vector<Account> loadedAccs = storeReloaded.loadAccounts();
    std::vector<Loan> loadedLoans = storeReloaded.loadLoans();

    ASSERT_EQ(loadedAccs.size(), static_cast<size_t>(1));
    ASSERT_STREQ(loadedAccs[0].accountNo, "ACC_P100");
    ASSERT_ALMOST_EQ(loadedAccs[0].balance, 55000.0, 0.01);

    ASSERT_EQ(loadedLoans.size(), static_cast<size_t>(1));
    ASSERT_STREQ(loadedLoans[0].loanId, "LON_P200");
    ASSERT_EQ(static_cast<int>(loadedLoans[0].status), static_cast<int>(LoanStatus::UNDER_BRANCH_MANAGER));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_persistence_save_load_fd_and_credit_cards, "tier1") {
    std::string testDir = "_test_data_persist_4";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);

    std::vector<FixedDeposit> fds;
    FixedDeposit fd;
    fd.fdNo = "FD_P300";
    fd.customerId = "CUST_P1";
    fd.bankId = "SBI";
    fd.principal = 100000.0;
    fd.interestRate = 6.75;
    fd.isClosed = false;
    fds.push_back(fd);
    store.saveFixedDeposits(fds);

    std::vector<CreditCard> cards;
    CreditCard cc;
    cc.cardNumber = "4532_P400";
    cc.customerId = "CUST_P1";
    cc.bankId = "SBI";
    cc.type = CreditCardType::GOLD;
    cc.creditLimit = 150000.0;
    cc.availableLimit = 150000.0;
    cc.isBlocked = false;
    cards.push_back(cc);
    store.saveCreditCards(cards);

    Storage storeReloaded(testDir);
    std::vector<FixedDeposit> loadedFDs = storeReloaded.loadFixedDeposits();
    std::vector<CreditCard> loadedCards = storeReloaded.loadCreditCards();

    ASSERT_EQ(loadedFDs.size(), static_cast<size_t>(1));
    ASSERT_STREQ(loadedFDs[0].fdNo, "FD_P300");
    ASSERT_ALMOST_EQ(loadedFDs[0].principal, 100000.0, 0.01);

    ASSERT_EQ(loadedCards.size(), static_cast<size_t>(1));
    ASSERT_STREQ(loadedCards[0].cardNumber, "4532_P400");
    ASSERT_ALMOST_EQ(loadedCards[0].creditLimit, 150000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_persistence_pipe_delimited_formatting_and_integrity, "tier1") {
    // Roundtrip test for struct serialization / deserialization pipe format
    Customer c1;
    c1.customerId = "C1";
    c1.name = "Test Name";
    c1.dob = "1990-01-01";
    c1.pan = "ABCDE1234F";
    c1.aadhaar = "123456789012";
    c1.address = "123 Main St";
    c1.phone = "9876543210";
    c1.email = "test@example.com";
    c1.creditScore = 750;
    c1.isKycVerified = true;
    c1.isSeniorCitizen = false;
    c1.passwordHash = "hash123";
    c1.bankId = "SBI";

    std::string serializedStr = c1.serialize();
    ASSERT_FALSE(serializedStr.empty());
    ASSERT_TRUE(serializedStr.find('|') != std::string::npos);

    Customer cDeserialized = Customer::deserialize(serializedStr);
    ASSERT_STREQ(cDeserialized.customerId, "C1");
    ASSERT_STREQ(cDeserialized.name, "Test Name");
    ASSERT_STREQ(cDeserialized.pan, "ABCDE1234F");
    ASSERT_EQ(cDeserialized.creditScore, 750);
    ASSERT_TRUE(cDeserialized.isKycVerified);
}
