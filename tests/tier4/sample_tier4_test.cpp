#include "TestHarness.hpp"
#include "TestUtils.hpp"

TEST_CASE(test_harness_tier4_workflow_helper, "tier4") {
    auto pathCmd = E2ETestUtils::getLoanEscalationPath(E2ETestUtils::rupeesToPaise(20000000.0)); // 2 Crore
    ASSERT_EQ(pathCmd.size(), 4U);
    ASSERT_STREQ(pathCmd[3], "CMD");
}
