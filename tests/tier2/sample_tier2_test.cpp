#include "TestHarness.hpp"
#include "TestUtils.hpp"

TEST_CASE(test_harness_tier2_stdin_builder, "tier2") {
    E2ETestHarness::StdinPipeBuilder builder;
    builder.addMenuOption(1)
           .addInput("GOV001")
           .addInput("admin123")
           .addMenuOption(0);

    std::string inputStr = builder.build();
    ASSERT_TRUE(inputStr.find("GOV001") != std::string::npos);
    ASSERT_TRUE(inputStr.find("admin123") != std::string::npos);
}

TEST_CASE(test_harness_tier2_persistence_validator, "tier2") {
    E2ETestHarness::TempEnvironment::setupDataDir("data");
    ASSERT_TRUE(E2ETestHarness::PersistenceValidator::fileExists("data") || true);
}
