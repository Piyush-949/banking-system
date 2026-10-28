#include "utils.h"
#include <ctime>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <random>
#include <functional>
#include <limits>

namespace BankingSystem {

std::string Utils::getCurrentDate() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << (tm->tm_year + 1900) << "-"
       << std::setw(2) << (tm->tm_mon + 1) << "-"
       << std::setw(2) << tm->tm_mday;
    return ss.str();
}

std::string Utils::getCurrentTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << (tm->tm_year + 1900) << "-"
       << std::setw(2) << (tm->tm_mon + 1) << "-"
       << std::setw(2) << tm->tm_mday << " "
       << std::setw(2) << tm->tm_hour << ":"
       << std::setw(2) << tm->tm_min << ":"
       << std::setw(2) << tm->tm_sec;
    return ss.str();
}

std::string Utils::formatCurrency(double amount) {
    std::stringstream ss;
    ss << "Rs. " << std::fixed << std::setprecision(2) << amount;
    return ss.str();
}

double Utils::calculateEMI(double principal, double annualRate, int tenureMonths) {
    if (principal <= 0 || tenureMonths <= 0) return 0.0;
    if (annualRate <= 0) return principal / tenureMonths;

    double monthlyRate = (annualRate / 12.0) / 100.0;
    double mathPower = std::pow(1.0 + monthlyRate, tenureMonths);
    double emi = (principal * monthlyRate * mathPower) / (mathPower - 1.0);
    return emi;
}

std::string Utils::hashPassword(const std::string& password) {
    std::hash<std::string> hasher;
    size_t hashVal = hasher(password + "RBI_SALT_2026");
    std::stringstream ss;
    ss << std::hex << hashVal;
    return ss.str();
}

std::string Utils::generateRandomId(const std::string& prefix) {
    static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_int_distribution<int> dist(100000, 999999);
    return prefix + std::to_string(dist(rng));
}

void Utils::printHeader(const std::string& title) {
    std::cout << "\n" << BOLD << CYAN;
    printDivider('=', 70);
    std::cout << "  " << title << "\n";
    printDivider('=', 70);
    std::cout << RESET;
}

void Utils::printSubHeader(const std::string& title) {
    std::cout << "\n" << BOLD << YELLOW << ">>> " << title << " <<<" << RESET << "\n";
}

void Utils::printSuccess(const std::string& msg) {
    std::cout << GREEN << "[SUCCESS] " << msg << RESET << "\n";
}

void Utils::printError(const std::string& msg) {
    std::cout << RED << "[ERROR] " << msg << RESET << "\n";
}

void Utils::printWarning(const std::string& msg) {
    std::cout << YELLOW << "[WARNING] " << msg << RESET << "\n";
}

void Utils::printInfo(const std::string& msg) {
    std::cout << BLUE << "[INFO] " << msg << RESET << "\n";
}

void Utils::printDivider(char symbol, int width) {
    std::cout << std::string(width, symbol) << "\n";
}

std::string Utils::getInput(const std::string& prompt) {
    std::cout << prompt << ": ";
    std::string line;
    std::getline(std::cin >> std::ws, line);
    return line;
}

double Utils::getDoubleInput(const std::string& prompt) {
    double val;
    while (true) {
        std::cout << prompt << ": ";
        if (std::cin >> val) {
            return val;
        }
        if (std::cin.eof()) return 0.0;
        std::cout << RED << "Invalid input. Please enter a valid number." << RESET << "\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

int Utils::getIntInput(const std::string& prompt) {
    int val;
    while (true) {
        std::cout << prompt << ": ";
        if (std::cin >> val) {
            return val;
        }
        if (std::cin.eof()) return 6; // Exit option default on EOF
        std::cout << RED << "Invalid input. Please enter a valid integer." << RESET << "\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void Utils::pressEnterToContinue() {
    std::cout << "\nPress ENTER to continue...";
    if (std::cin.eof()) return;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

} // namespace BankingSystem
