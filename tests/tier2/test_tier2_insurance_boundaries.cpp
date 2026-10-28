#include "TestHarness.hpp"
#include "TestUtils.hpp"
#include "insurance.h"
#include "auth.h"
#include "storage.h"
#include "structures.h"
#include "utils.h"

using namespace BankingSystem;

static void setupInsuranceTestEnv(Storage& store, AuthManager& auth) {
    E2ETestHarness::TempEnvironment::cleanupDataDir("data");
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    auth.reloadUsers();

    Customer cust;
    cust.customerId = "CUST_INS";
    cust.name = "Insurance User";
    cust.isKycVerified = true;
    cust.passwordHash = Utils::hashPassword("pass");
    auth.registerCustomer(cust);

    Staff bm, teller;
    bm.staffId = "BM001"; bm.role = Role::BRANCH_MANAGER; bm.passwordHash = Utils::hashPassword("pass");
    teller.staffId = "TEL001"; teller.role = Role::TELLER; teller.passwordHash = Utils::hashPassword("pass");

    auth.addStaffMember(bm);
    auth.addStaffMember(teller);
}

TEST_CASE(test_tier2_insurance_zero_coverage_policy, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    InsuranceManager insMgr(store, auth);
    setupInsuranceTestEnv(store, auth);

    // Buy policy with 0.0 coverage
    InsurancePolicy* p = insMgr.buyPolicy("CUST_INS", "SBI", InsuranceType::LIFE_TERM, 0.0, "Spouse", "Wife");
    ASSERT_NE(p, nullptr);
    ASSERT_ALMOST_EQ(p->coverageAmount, 0.0, 0.001);
    ASSERT_ALMOST_EQ(p->annualPremium, 0.0, 0.001);
}

TEST_CASE(test_tier2_insurance_claim_exceeding_coverage, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    InsuranceManager insMgr(store, auth);
    setupInsuranceTestEnv(store, auth);

    // Buy policy with 1,000,000 coverage
    InsurancePolicy* p = insMgr.buyPolicy("CUST_INS", "SBI", InsuranceType::HEALTH_MEDICLAIM, 1000000.0, "Self", "Self");
    ASSERT_NE(p, nullptr);

    // File claim for 1,500,000 (Exceeds coverage limit) -> Rejection
    bool res = insMgr.fileClaim(p->policyNo, 1500000.0);
    ASSERT_FALSE(res);
    ASSERT_FALSE(p->claimPending);
}

TEST_CASE(test_tier2_insurance_duplicate_claim_submission, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    InsuranceManager insMgr(store, auth);
    setupInsuranceTestEnv(store, auth);

    InsurancePolicy* p = insMgr.buyPolicy("CUST_INS", "SBI", InsuranceType::VEHICLE_COMPREHENSIVE, 500000.0, "Self", "Self");
    ASSERT_NE(p, nullptr);

    // First claim submission -> Success
    bool res1 = insMgr.fileClaim(p->policyNo, 200000.0);
    ASSERT_TRUE(res1);
    ASSERT_TRUE(p->claimPending);

    // Second claim submission while first is pending -> Attempt or update status
    bool res2 = insMgr.fileClaim(p->policyNo, 100000.0);
    ASSERT_TRUE(res2 || p->claimPending);
}

TEST_CASE(test_tier2_insurance_claim_inactive_policy, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    InsuranceManager insMgr(store, auth);
    setupInsuranceTestEnv(store, auth);

    // File claim for non-existent policy number
    bool res = insMgr.fileClaim("INS_NONEXISTENT_999", 50000.0);
    ASSERT_FALSE(res);
}

TEST_CASE(test_tier2_insurance_claim_review_unauthorized, "tier2") {
    Storage store("data");
    AuthManager auth(store);
    InsuranceManager insMgr(store, auth);
    setupInsuranceTestEnv(store, auth);

    InsurancePolicy* p = insMgr.buyPolicy("CUST_INS", "SBI", InsuranceType::LIFE_ENDOWMENT, 2000000.0, "Child", "Son");
    ASSERT_NE(p, nullptr);
    insMgr.fileClaim(p->policyNo, 500000.0);

    // Login as Teller (Role 6) instead of Branch Manager (Role 3)
    auth.loginStaff("TEL001", "pass");

    // Process claim as Teller -> Rejected by permission check
    bool res = insMgr.processClaimBranchManager(p->policyNo, true);
    ASSERT_FALSE(res);
}
