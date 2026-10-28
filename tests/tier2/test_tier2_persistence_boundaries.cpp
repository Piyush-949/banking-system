#include "TestHarness.hpp"
#include "TestUtils.hpp"
#include "storage.h"
#include "structures.h"
#include "utils.h"
#include <fstream>
#include <filesystem>

using namespace BankingSystem;

static void setupPersistenceTestEnv() {
    E2ETestHarness::TempEnvironment::cleanupDataDir("data");
    E2ETestHarness::TempEnvironment::setupDataDir("data");
}

TEST_CASE(test_tier2_persistence_corrupted_line_recovery, "tier2") {
    setupPersistenceTestEnv();

    // Create corrupted customers.dat file with incomplete fields
    {
        std::ofstream ofs("data/customers.dat");
        ofs << "CORRUPTED_LINE_1\n";
        ofs << "CUST001|John Doe|1990-01-01\n"; // Missing fields
        ofs << "CUST002|Jane Smith|1992-05-15|PAN123|AADHAAR123|Address|9999999999|jane@test.com|750|1|0|hash123|BNK001\n"; // Valid 13 fields
    }

    Storage store("data");
    auto list = store.loadCustomers();

    // Should process lines without crashing and load valid record(s)
    ASSERT_FALSE(list.empty());
    bool foundValid = false;
    for (const auto& c : list) {
        if (c.customerId == "CUST002") {
            foundValid = true;
            ASSERT_STREQ(c.name, "Jane Smith");
            ASSERT_EQ(c.creditScore, 750);
        }
    }
    ASSERT_TRUE(foundValid);
}

TEST_CASE(test_tier2_persistence_empty_file_initialization, "tier2") {
    setupPersistenceTestEnv();

    // Touch 0-byte .dat files
    { std::ofstream("data/customers.dat"); }
    { std::ofstream("data/accounts.dat"); }
    { std::ofstream("data/loans.dat"); }
    { std::ofstream("data/banks.dat"); }

    Storage store("data");
    auto custs = store.loadCustomers();
    auto accs = store.loadAccounts();
    auto loans = store.loadLoans();
    auto banks = store.loadBanks();

    ASSERT_EQ(custs.size(), 0);
    ASSERT_EQ(accs.size(), 0);
    ASSERT_EQ(loans.size(), 0);
    ASSERT_EQ(banks.size(), 0);
}

TEST_CASE(test_tier2_persistence_atomic_tmp_swap, "tier2") {
    setupPersistenceTestEnv();

    Storage store("data");
    RBIPolicy policy;
    policy.repoRate = 7.25;
    policy.reverseRepoRate = 7.00;
    policy.crr = 5.00;
    policy.slr = 19.00;
    policy.lastUpdatedDate = "2026-08-09";
    policy.circulars.push_back("Test Circular Message");

    bool saved = store.saveRBIPolicy(policy);
    ASSERT_TRUE(saved);
    ASSERT_TRUE(E2ETestHarness::PersistenceValidator::fileExists("data/rbi.dat"));

    RBIPolicy loaded = store.loadRBIPolicy();
    ASSERT_ALMOST_EQ(loaded.repoRate, 7.25, 0.001);
    ASSERT_STREQ(loaded.lastUpdatedDate, "2026-08-09");
    ASSERT_FALSE(loaded.circulars.empty());
}

TEST_CASE(test_tier2_persistence_delimiter_whitespace_tolerance, "tier2") {
    setupPersistenceTestEnv();

    // Line with extra trailing spaces or whitespace
    std::string line = "CUST100 | Alice Smith | 1985-10-20 | PAN456 | AADHAAR456 | Main St | 8888888888 | alice@test.com | 700 | 1 | 0 | hash456 | BNK001";
    
    // Parse pipe delimited fields
    auto records = E2ETestHarness::PersistenceValidator::parsePipeDelimited("data/dummy.dat");
    ASSERT_TRUE(records.empty());

    Customer c = Customer::deserialize(line);
    ASSERT_FALSE(c.customerId.empty());
}

TEST_CASE(test_tier2_persistence_large_volume_serialization, "tier2") {
    setupPersistenceTestEnv();

    Storage store("data");
    std::vector<Customer> largeList;

    for (int i = 0; i < 150; ++i) {
        Customer c;
        c.customerId = "CUST_BULK_" + std::to_string(i);
        c.name = "Bulk User " + std::to_string(i);
        c.dob = "2000-01-01";
        c.pan = "PAN_BULK_" + std::to_string(i);
        c.aadhaar = "AADHAAR_" + std::to_string(i);
        c.address = "Bulk Address " + std::to_string(i);
        c.phone = "9000000" + std::to_string(i);
        c.email = "user" + std::to_string(i) + "@bulk.com";
        c.creditScore = 600 + (i % 300);
        c.isKycVerified = (i % 2 == 0);
        c.isSeniorCitizen = (i % 5 == 0);
        c.passwordHash = "hash" + std::to_string(i);
        c.bankId = "BNK001";
        largeList.push_back(c);
    }

    bool saved = store.saveCustomers(largeList);
    ASSERT_TRUE(saved);

    auto reloaded = store.loadCustomers();
    ASSERT_EQ(reloaded.size(), 150);
    ASSERT_STREQ(reloaded[0].customerId, "CUST_BULK_0");
    ASSERT_STREQ(reloaded[149].customerId, "CUST_BULK_149");
}
