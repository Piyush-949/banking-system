#include "TestHarness.hpp"
#include "TestUtils.hpp"

TEST_CASE(test_harness_tier3_pairwise_helper, "tier3") {
    auto path = E2ETestUtils::getLoanEscalationPath(E2ETestUtils::rupeesToPaise(5000000.0)); // 50L
    ASSERT_EQ(path.size(), 3U);
    ASSERT_STREQ(path[0], "Loan Officer");
    ASSERT_STREQ(path[1], "Branch Manager");
    ASSERT_STREQ(path[2], "Zonal Manager");
}
