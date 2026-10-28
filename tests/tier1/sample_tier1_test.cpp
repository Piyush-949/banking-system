#include "TestHarness.hpp"
#include "TestUtils.hpp"

TEST_CASE(test_harness_tier1_smoke, "tier1") {
    ASSERT_TRUE(1 + 1 == 2);
    ASSERT_EQ(E2ETestUtils::rupeesToPaise(100.50), 10050LL);
    ASSERT_STREQ(E2ETestUtils::paiseToRupeesString(10050LL), "100.50");
    ASSERT_PAISE_EQ(E2ETestUtils::rupeesToPaise(50.25), 5025LL);
    ASSERT_RUPEES_EQ(E2ETestUtils::paiseToRupees(5025LL), 50.25);
}

TEST_CASE(test_fd_interest_and_tds_calc, "tier1") {
    int64_t principal = E2ETestUtils::rupeesToPaise(100000.0); // 1 Lakh
    double rate = 7.0; // 7.0% p.a.
    double tenure = 1.0; // 1 year
    int64_t expectedInterest = E2ETestUtils::calculateFDInterestPaise(principal, rate, tenure, false);
    
    // Principal 100000 * (1 + 0.07/4)^4 = 100000 * (1.0175)^4 = 107185.90 -> interest 7185.90 -> 718590 paise
    ASSERT_TRUE(expectedInterest > 700000LL);

    // TDS for non-senior citizen with interest < 40,000 INR should be 0
    int64_t tds = E2ETestUtils::calculateTDS(expectedInterest, false);
    ASSERT_PAISE_EQ(tds, 0LL);

    // TDS for high interest (> 40,000 INR = 4,000,000 paise)
    int64_t highInterestPaise = 5000000LL; // 50,000 INR
    int64_t expectedTds = E2ETestUtils::calculateTDS(highInterestPaise, false);
    ASSERT_PAISE_EQ(expectedTds, 500000LL); // 10% = 5,000 INR = 500,000 paise
}

TEST_CASE(test_loan_escalation_rules, "tier1") {
    // < 25 Lakhs -> Branch Manager
    ASSERT_STREQ(E2ETestUtils::getRequiredLoanApprovalRole(E2ETestUtils::rupeesToPaise(1000000.0)), "Branch Manager");
    // 25 Lakhs to 1 Crore -> Zonal Manager
    ASSERT_STREQ(E2ETestUtils::getRequiredLoanApprovalRole(E2ETestUtils::rupeesToPaise(5000000.0)), "Zonal Manager");
    // > 1 Crore -> CMD
    ASSERT_STREQ(E2ETestUtils::getRequiredLoanApprovalRole(E2ETestUtils::rupeesToPaise(20000000.0)), "CMD");
}
