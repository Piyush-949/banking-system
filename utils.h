#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <iostream>

namespace BankingSystem {

class Utils {
public:
    // ANSI Color Codes
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* BOLD = "\033[1m";
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* BLUE = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* WHITE = "\033[37m";

    // Helpers
    static std::string getCurrentDate();
    static std::string getCurrentTimestamp();
    static std::string formatCurrency(double amount);
    static double calculateEMI(double principal, double annualRate, int tenureMonths);
    static std::string hashPassword(const std::string& password);
    static std::string generateRandomId(const std::string& prefix);
    
    // UI Helpers
    static void printHeader(const std::string& title);
    static void printSubHeader(const std::string& title);
    static void printSuccess(const std::string& msg);
    static void printError(const std::string& msg);
    static void printWarning(const std::string& msg);
    static void printInfo(const std::string& msg);
    static void printDivider(char symbol = '=', int width = 70);

    // Input Helpers
    static std::string getInput(const std::string& prompt);
    static double getDoubleInput(const std::string& prompt);
    static int getIntInput(const std::string& prompt);
    static void pressEnterToContinue();
};

} // namespace BankingSystem

#endif // UTILS_H
