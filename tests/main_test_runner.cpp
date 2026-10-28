#include "include/TestHarness.hpp"
#include "include/TestUtils.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    std::string groupFilter = "";
    std::string reportPath = "";
    bool verbose = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--group" || arg == "--tier") && i + 1 < argc) {
            groupFilter = argv[++i];
        } else if (arg == "--report" && i + 1 < argc) {
            reportPath = argv[++i];
        } else if (arg == "--quiet" || arg == "-q") {
            verbose = false;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "RBI Banking Management System - E2E Test Runner\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --group <group_name>   Run tests matching group/tier (e.g. tier1, tier2, tier3, tier4)\n";
            std::cout << "  --tier <tier_name>     Alias for --group\n";
            std::cout << "  --report <file_path>   Save detailed test report to specified file path\n";
            std::cout << "  --quiet, -q            Suppress detailed console output\n";
            std::cout << "  --help, -h             Show this help message\n";
            return 0;
        }
    }

    std::cout << "[E2E Test Runner] Initializing test suite execution...\n";
    if (!groupFilter.empty()) {
        std::cout << "[E2E Test Runner] Filtering by group/tier: " << groupFilter << "\n";
    }

    int failures = E2ETestHarness::TestRegistry::instance().runAll(groupFilter, verbose);

    if (!reportPath.empty()) {
        const auto& tests = E2ETestHarness::TestRegistry::instance().getTests();
        if (E2ETestHarness::TestReportGenerator::generateReportFile(tests, reportPath)) {
            std::cout << "[E2E Test Runner] Detailed test report written to: " << reportPath << "\n";
        } else {
            std::cerr << "[E2E Test Runner] Warning: Failed to write report to: " << reportPath << "\n";
        }
    }

    return (failures == 0) ? 0 : 1;
}
