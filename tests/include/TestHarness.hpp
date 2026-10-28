#ifndef TEST_HARNESS_HPP
#define TEST_HARNESS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include <type_traits>

namespace E2ETestHarness {

// Type trait helper to check if a type is streamable to std::ostream
template <typename T, typename = void>
struct is_streamable : std::false_type {};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>> : std::true_type {};

// Format value helper supporting streamables, enums, and non-streamable types
template <typename T>
std::string formatValue(const T& val) {
    if constexpr (std::is_enum_v<T>) {
        return std::to_string(static_cast<std::underlying_type_t<T>>(val));
    } else if constexpr (is_streamable<T>::value) {
        std::ostringstream ss;
        ss << val;
        return ss.str();
    } else {
        return "<non-streamable object>";
    }
}

// Custom exception thrown when an assertion fails
class TestAssertionFailure : public std::runtime_error {
public:
    std::string file;
    int line;

    TestAssertionFailure(const std::string& msg, const std::string& f, int l)
        : std::runtime_error(msg), file(f), line(l) {}
};

// Data structure representing a test case
struct TestCase {
    std::string name;
    std::string group; // e.g. "tier1", "tier2", "tier3", "tier4"
    std::function<void()> func;
    bool passed = false;
    std::string failureMessage;
    std::string failureFile;
    int failureLine = 0;
    double durationMs = 0.0;
};

// Singleton Test Registry
class TestRegistry {
private:
    std::vector<TestCase> tests_;
    TestRegistry() = default;

public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void registerTest(const std::string& name, const std::string& group, std::function<void()> func) {
        TestCase tc;
        tc.name = name;
        tc.group = group;
        tc.func = func;
        tc.passed = false;
        tc.durationMs = 0.0;
        tests_.push_back(tc);
    }

    const std::vector<TestCase>& getTests() const {
        return tests_;
    }

    std::vector<TestCase>& getMutableTests() {
        return tests_;
    }

    void clear() {
        tests_.clear();
    }

    int runAll(const std::string& groupFilter = "", bool verbose = true);
};

// Stdin Pipe Builder for CLI interactive inputs
class StdinPipeBuilder {
private:
    std::stringstream stream_;

public:
    StdinPipeBuilder() = default;

    StdinPipeBuilder& addInput(const std::string& input) {
        stream_ << input << "\n";
        return *this;
    }

    StdinPipeBuilder& addLine(const std::string& line) {
        stream_ << line << "\n";
        return *this;
    }

    StdinPipeBuilder& addMenuOption(int option) {
        stream_ << option << "\n";
        return *this;
    }

    StdinPipeBuilder& addAmountPaise(int64_t paise) {
        stream_ << paise << "\n";
        return *this;
    }

    StdinPipeBuilder& addAmountRupees(double rupees) {
        stream_ << std::fixed << std::setprecision(2) << rupees << "\n";
        return *this;
    }

    StdinPipeBuilder& addBlankLine() {
        stream_ << "\n";
        return *this;
    }

    std::string build() const {
        return stream_.str();
    }

    void clear() {
        stream_.str("");
        stream_.clear();
    }
};

// CLI Command Execution Result
struct CLIExecutionResult {
    int exitCode = -1;
    std::string stdoutOutput;
    std::string stderrOutput;
    double durationMs = 0.0;

    bool containsStdout(const std::string& substring) const {
        return stdoutOutput.find(substring) != std::string::npos;
    }

    bool containsStderr(const std::string& substring) const {
        return stderrOutput.find(substring) != std::string::npos;
    }
};

// CLI Execution Helper using temporary input/output files (deadlock-free on Windows & Unix)
inline CLIExecutionResult executeCLI(
    const std::string& execPath,
    const std::string& stdinInput = "",
    const std::string& workingDir = "."
) {
    CLIExecutionResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    static uint64_t runCounter = 0;
    runCounter++;
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::string tempPrefix = workingDir + "/_e2e_tmp_" + std::to_string(runCounter) + "_" + std::to_string(timestamp);
    std::string stdinFile = tempPrefix + "_in.txt";
    std::string stdoutFile = tempPrefix + "_out.txt";
    std::string stderrFile = tempPrefix + "_err.txt";

    // Write stdin to temporary input file
    {
        std::ofstream out(stdinFile, std::ios::binary);
        out << stdinInput;
    }

    // Command line construction
    std::string cmd = "\"" + execPath + "\" < \"" + stdinFile + "\" > \"" + stdoutFile + "\" 2> \"" + stderrFile + "\"";

    result.exitCode = std::system(cmd.c_str());

    // Read stdout
    {
        std::ifstream in(stdoutFile, std::ios::binary);
        if (in) {
            std::ostringstream ss;
            ss << in.rdbuf();
            result.stdoutOutput = ss.str();
        }
    }

    // Read stderr
    {
        std::ifstream in(stderrFile, std::ios::binary);
        if (in) {
            std::ostringstream ss;
            ss << in.rdbuf();
            result.stderrOutput = ss.str();
        }
    }

    // Clean up temporary files
    std::error_code ec;
    std::filesystem::remove(stdinFile, ec);
    std::filesystem::remove(stdoutFile, ec);
    std::filesystem::remove(stderrFile, ec);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.durationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    return result;
}

// Temporary Environment & Persistence Setup / Teardown Helper
class TempEnvironment {
public:
    static void setupDataDir(const std::string& dirPath = "data") {
        std::error_code ec;
        std::filesystem::create_directories(dirPath, ec);
    }

    static void cleanupDataDir(const std::string& dirPath = "data") {
        std::error_code ec;
        if (std::filesystem::exists(dirPath, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(dirPath, ec)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".dat" || ext == ".csv" || ext == ".tmp" || ext == ".log") {
                        std::filesystem::remove(entry.path(), ec);
                    }
                }
            }
        }
    }

    static void removeDirectory(const std::string& dirPath) {
        std::error_code ec;
        std::filesystem::remove_all(dirPath, ec);
    }
};

// Persistence State Validator Helper
class PersistenceValidator {
public:
    static bool fileExists(const std::string& filePath) {
        std::error_code ec;
        return std::filesystem::exists(filePath, ec) && std::filesystem::is_regular_file(filePath, ec);
    }

    static bool fileIsNonEmpty(const std::string& filePath) {
        std::error_code ec;
        return fileExists(filePath) && (std::filesystem::file_size(filePath, ec) > 0);
    }

    static size_t countLines(const std::string& filePath) {
        if (!fileExists(filePath)) return 0;
        std::ifstream file(filePath);
        size_t lines = 0;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                lines++;
            }
        }
        return lines;
    }

    static bool containsRecord(const std::string& filePath, const std::string& keyword) {
        if (!fileExists(filePath)) return false;
        std::ifstream file(filePath);
        std::string line;
        while (std::getline(file, line)) {
            if (line.find(keyword) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    static std::vector<std::vector<std::string>> parsePipeDelimited(const std::string& filePath, char delimiter = '|') {
        std::vector<std::vector<std::string>> records;
        if (!fileExists(filePath)) return records;

        std::ifstream file(filePath);
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::vector<std::string> fields;
            std::stringstream ss(line);
            std::string field;
            while (std::getline(ss, field, delimiter)) {
                fields.push_back(field);
            }
            records.push_back(fields);
        }
        return records;
    }

    static bool validateFileStructure(const std::string& filePath, size_t expectedMinLines, size_t expectedMinFieldsPerLine, char delimiter = '|') {
        auto records = parsePipeDelimited(filePath, delimiter);
        if (records.size() < expectedMinLines) return false;
        for (const auto& row : records) {
            if (row.size() < expectedMinFieldsPerLine) return false;
        }
        return true;
    }
};

// Summary Report Generator
class TestReportGenerator {
public:
    static void printSummary(const std::vector<TestCase>& tests, const std::string& filterGroup = "") {
        size_t total = 0;
        size_t passed = 0;
        size_t failed = 0;
        double totalDurationMs = 0.0;

        std::cout << "\n=======================================================\n";
        std::cout << "             E2E TEST HARNESS SUMMARY REPORT           \n";
        std::cout << "=======================================================\n";

        for (const auto& tc : tests) {
            if (!filterGroup.empty() && tc.group != filterGroup) continue;
            total++;
            totalDurationMs += tc.durationMs;
            if (tc.passed) {
                passed++;
                std::cout << " [PASS] [" << std::left << std::setw(8) << tc.group << "] "
                          << std::left << std::setw(45) << tc.name
                          << " (" << std::fixed << std::setprecision(2) << tc.durationMs << " ms)\n";
            } else {
                failed++;
                std::cout << " [FAIL] [" << std::left << std::setw(8) << tc.group << "] "
                          << std::left << std::setw(45) << tc.name
                          << " (" << std::fixed << std::setprecision(2) << tc.durationMs << " ms)\n";
                std::cout << "        File: " << tc.failureFile << ":" << tc.failureLine << "\n";
                std::cout << "        Error: " << tc.failureMessage << "\n";
            }
        }

        std::cout << "-------------------------------------------------------\n";
        std::cout << " Total Executed: " << total << "\n";
        std::cout << " Passed:         " << passed << "\n";
        std::cout << " Failed:         " << failed << "\n";
        std::cout << " Total Time:     " << std::fixed << std::setprecision(2) << totalDurationMs << " ms\n";
        std::cout << " Status:         " << (failed == 0 ? "SUCCESS (ALL PASSED)" : "FAILURE") << "\n";
        std::cout << "=======================================================\n\n";
    }

    static bool generateReportFile(const std::vector<TestCase>& tests, const std::string& outputPath) {
        std::ofstream out(outputPath);
        if (!out) return false;

        out << "=======================================================\n";
        out << "             E2E TEST HARNESS DETAILED REPORT          \n";
        out << "=======================================================\n\n";

        size_t passed = 0, failed = 0;
        for (const auto& tc : tests) {
            if (tc.passed) passed++; else failed++;
            out << "Test: " << tc.name << " | Group: " << tc.group << " | Result: " << (tc.passed ? "PASS" : "FAIL")
                << " | Time: " << tc.durationMs << " ms\n";
            if (!tc.passed) {
                out << "   File: " << tc.failureFile << ":" << tc.failureLine << "\n";
                out << "   Reason: " << tc.failureMessage << "\n";
            }
        }
        out << "\nSummary: Total=" << tests.size() << ", Passed=" << passed << ", Failed=" << failed << "\n";
        return true;
    }
};

inline int TestRegistry::runAll(const std::string& groupFilter, bool verbose) {
    int failCount = 0;
    for (auto& tc : tests_) {
        if (!groupFilter.empty() && tc.group != groupFilter) {
            continue;
        }

        auto start = std::chrono::high_resolution_clock::now();
        try {
            tc.func();
            tc.passed = true;
        } catch (const TestAssertionFailure& e) {
            tc.passed = false;
            tc.failureMessage = e.what();
            tc.failureFile = e.file;
            tc.failureLine = e.line;
            failCount++;
        } catch (const std::exception& e) {
            tc.passed = false;
            tc.failureMessage = std::string("Unhandled std::exception: ") + e.what();
            tc.failureFile = "unknown";
            tc.failureLine = 0;
            failCount++;
        } catch (...) {
            tc.passed = false;
            tc.failureMessage = "Unhandled non-std exception caught";
            tc.failureFile = "unknown";
            tc.failureLine = 0;
            failCount++;
        }
        auto end = std::chrono::high_resolution_clock::now();
        tc.durationMs = std::chrono::duration<double, std::milli>(end - start).count();
    }

    if (verbose) {
        TestReportGenerator::printSummary(tests_, groupFilter);
    }

    return failCount;
}

} // namespace E2ETestHarness

// Assertion Macros
#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_TRUE failed: (" #cond ") evaluated to false"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_FALSE failed: (" #cond ") evaluated to true"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_EQ(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 == _v2)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_EQ failed: Actual [" << E2ETestHarness::formatValue(_v1) \
                 << "], Expected [" << E2ETestHarness::formatValue(_v2) << "] (" #val1 " == " #val2 ")"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_NE(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (_v1 == _v2) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_NE failed: Expected not equal to [" << E2ETestHarness::formatValue(_v2) \
                 << "], but both are [" << E2ETestHarness::formatValue(_v1) << "]"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_GT(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 > _v2)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_GT failed: Actual [" << E2ETestHarness::formatValue(_v1) \
                 << "] is not > Expected [" << E2ETestHarness::formatValue(_v2) << "] (" #val1 " > " #val2 ")"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_GE(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 >= _v2)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_GE failed: Actual [" << E2ETestHarness::formatValue(_v1) \
                 << "] is not >= Expected [" << E2ETestHarness::formatValue(_v2) << "] (" #val1 " >= " #val2 ")"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_LT(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 < _v2)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_LT failed: Actual [" << E2ETestHarness::formatValue(_v1) \
                 << "] is not < Expected [" << E2ETestHarness::formatValue(_v2) << "] (" #val1 " < " #val2 ")"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_LE(val1, val2) \
    do { \
        auto _v1 = (val1); \
        auto _v2 = (val2); \
        if (!(_v1 <= _v2)) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_LE failed: Actual [" << E2ETestHarness::formatValue(_v1) \
                 << "] is not <= Expected [" << E2ETestHarness::formatValue(_v2) << "] (" #val1 " <= " #val2 ")"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_STREQ(str1, str2) \
    do { \
        std::string _s1 = (str1); \
        std::string _s2 = (str2); \
        if (_s1 != _s2) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_STREQ failed:\n  Expected: \"" << _s2 << "\"\n  Actual:   \"" << _s1 << "\""; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_ALMOST_EQ(val1, val2, tolerance) \
    do { \
        double _v1 = static_cast<double>(val1); \
        double _v2 = static_cast<double>(val2); \
        double _tol = static_cast<double>(tolerance); \
        if (std::abs(_v1 - _v2) > _tol) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_ALMOST_EQ failed: Actual [" << _v1 << "], Expected [" << _v2 << "] (diff " << std::abs(_v1 - _v2) << " > tol " << _tol << ")"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define ASSERT_THROWS(statement, ExceptionType) \
    do { \
        bool _caught = false; \
        try { \
            statement; \
        } catch (const ExceptionType&) { \
            _caught = true; \
        } catch (...) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_THROWS failed: Statement (" #statement ") threw an unexpected exception type (expected " #ExceptionType ")"; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
        if (!_caught) { \
            std::ostringstream _oss; \
            _oss << "ASSERT_THROWS failed: Statement (" #statement ") did not throw expected exception " #ExceptionType; \
            throw E2ETestHarness::TestAssertionFailure(_oss.str(), __FILE__, __LINE__); \
        } \
    } while(0)

// EXPECT_* aliases for ASSERT_*
#define EXPECT_TRUE(cond) ASSERT_TRUE(cond)
#define EXPECT_FALSE(cond) ASSERT_FALSE(cond)
#define EXPECT_EQ(val1, val2) ASSERT_EQ(val1, val2)
#define EXPECT_NE(val1, val2) ASSERT_NE(val1, val2)
#define EXPECT_GT(val1, val2) ASSERT_GT(val1, val2)
#define EXPECT_GE(val1, val2) ASSERT_GE(val1, val2)
#define EXPECT_LT(val1, val2) ASSERT_LT(val1, val2)
#define EXPECT_LE(val1, val2) ASSERT_LE(val1, val2)
#define EXPECT_STREQ(str1, str2) ASSERT_STREQ(str1, str2)

// Test Case Registration Macro
#define TEST_CASE(name, group) \
    void name(); \
    struct Register_##name { \
        Register_##name() { \
            E2ETestHarness::TestRegistry::instance().registerTest(#name, group, name); \
        } \
    } instance_Register_##name; \
    void name()

#endif // TEST_HARNESS_HPP
