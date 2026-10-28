#include "../include/TestHarness.hpp"
#include "../include/TestUtils.hpp"
#include "../../structures.h"
#include "../../storage.h"
#include "../../auth.h"
#include "../../insurance.h"
#include "../../utils.h"

using namespace BankingSystem;

static void setupInsuranceUser(AuthManager& auth, const std::string& custId) {
    Customer c;
    c.customerId = custId;
    c.name = "Insurance User " + custId;
    c.isKycVerified = true;
    c.bankId = "ICICI";
    c.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(c);

    Staff bm;
    bm.staffId = "BM_ICICI";
    bm.role = Role::BRANCH_MANAGER;
    bm.bankId = "ICICI";
    bm.passwordHash = Utils::hashPassword("bmpass");
    auth.addStaffMember(bm);
}

TEST_CASE(test_insurance_policy_issuance, "tier1") {
    std::string testDir = "_test_data_ins_1";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    InsuranceManager insMgr(store, auth);

    setupInsuranceUser(auth, "CUST_INS1");

    InsurancePolicy* p1 = insMgr.buyPolicy("CUST_INS1", "ICICI", InsuranceType::LIFE_TERM, 5000000.0, "Jane Doe", "Spouse");
    InsurancePolicy* p2 = insMgr.buyPolicy("CUST_INS1", "ICICI", InsuranceType::HEALTH_MEDICLAIM, 500000.0, "Jane Doe", "Spouse");

    ASSERT_TRUE(p1 != nullptr);
    ASSERT_TRUE(p2 != nullptr);
    ASSERT_TRUE(p1->isActive);
    ASSERT_TRUE(p2->isActive);
    ASSERT_FALSE(p1->policyNo.empty());

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_insurance_premium_computation, "tier1") {
    std::string testDir = "_test_data_ins_2";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    InsuranceManager insMgr(store, auth);

    // Life Term rate = 0.3% -> Premium on 1,00,00,000 = 30,000
    double pLife = insMgr.calculateAnnualPremium(InsuranceType::LIFE_TERM, 10000000.0);
    ASSERT_ALMOST_EQ(pLife, 30000.0, 0.01);

    // Health Mediclaim rate = 1.5% -> Premium on 10,00,000 = 15,000
    double pHealth = insMgr.calculateAnnualPremium(InsuranceType::HEALTH_MEDICLAIM, 1000000.0);
    ASSERT_ALMOST_EQ(pHealth, 15000.0, 0.01);

    // Vehicle Comprehensive rate = 3.0% -> Premium on 10,00,000 = 30,000
    double pVehicle = insMgr.calculateAnnualPremium(InsuranceType::VEHICLE_COMPREHENSIVE, 1000000.0);
    ASSERT_ALMOST_EQ(pVehicle, 30000.0, 0.01);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_insurance_claim_submission, "tier1") {
    std::string testDir = "_test_data_ins_3";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    InsuranceManager insMgr(store, auth);

    setupInsuranceUser(auth, "CUST_INS3");

    InsurancePolicy* p = insMgr.buyPolicy("CUST_INS3", "ICICI", InsuranceType::HEALTH_MEDICLAIM, 500000.0, "Jane Doe", "Spouse");
    std::string polNo = p->policyNo;

    // File claim within coverage (2,00,000 <= 5,00,000)
    ASSERT_TRUE(insMgr.fileClaim(polNo, 200000.0));

    std::vector<InsurancePolicy> userPols = insMgr.getCustomerPolicies("CUST_INS3");
    bool foundClaim = false;
    for (const auto& pol : userPols) {
        if (pol.policyNo == polNo && pol.claimPending) {
            foundClaim = true;
            ASSERT_STREQ(pol.claimStatus, "SUBMITTED");
            ASSERT_ALMOST_EQ(pol.claimAmountRequested, 200000.0, 0.01);
        }
    }
    ASSERT_TRUE(foundClaim);

    // File claim exceeding coverage limit -> Should fail
    ASSERT_FALSE(insMgr.fileClaim(polNo, 800000.0));

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_insurance_claim_branch_manager_approval, "tier1") {
    std::string testDir = "_test_data_ins_4";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    InsuranceManager insMgr(store, auth);

    setupInsuranceUser(auth, "CUST_INS4");

    InsurancePolicy* p = insMgr.buyPolicy("CUST_INS4", "ICICI", InsuranceType::VEHICLE_COMPREHENSIVE, 800000.0, "Nominee", "Father");
    std::string polNo = p->policyNo;

    insMgr.fileClaim(polNo, 150000.0);

    // Branch Manager login required to process claim
    auth.loginStaff("BM_ICICI", "bmpass");
    ASSERT_TRUE(insMgr.processClaimBranchManager(polNo, true));
    auth.logout();

    std::vector<InsurancePolicy> userPols = insMgr.getCustomerPolicies("CUST_INS4");
    bool approved = false;
    for (const auto& pol : userPols) {
        if (pol.policyNo == polNo && pol.claimStatus == "APPROVED") {
            approved = true;
        }
    }
    ASSERT_TRUE(approved);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}

TEST_CASE(test_insurance_nominee_details_and_list, "tier1") {
    std::string testDir = "_test_data_ins_5";
    E2ETestHarness::TempEnvironment::setupDataDir(testDir);

    Storage store(testDir);
    AuthManager auth(store);
    InsuranceManager insMgr(store, auth);

    setupInsuranceUser(auth, "CUST_INS5");

    InsurancePolicy* p = insMgr.buyPolicy("CUST_INS5", "ICICI", InsuranceType::LIFE_ENDOWMENT, 2000000.0, "Mary Johnson", "Daughter");
    std::string polNo = p->policyNo;
    insMgr.fileClaim(polNo, 500000.0);

    std::vector<InsurancePolicy> pendingList = insMgr.getPendingClaims("ICICI");
    ASSERT_FALSE(pendingList.empty());

    bool foundPolicy = false;
    for (const auto& pol : pendingList) {
        if (pol.policyNo == polNo) {
            foundPolicy = true;
            ASSERT_STREQ(pol.nomineeName, "Mary Johnson");
            ASSERT_STREQ(pol.nomineeRelation, "Daughter");
        }
    }
    ASSERT_TRUE(foundPolicy);

    E2ETestHarness::TempEnvironment::removeDirectory(testDir);
}
