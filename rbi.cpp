#include "rbi.h"
#include "utils.h"
#include <iostream>
#include <iomanip>

namespace BankingSystem {

RBIController::RBIController(Storage& store, AuthManager& auth)
    : storage(store), authManager(auth) {
    loadData();
}

void RBIController::loadData() {
    rbiPolicy = storage.loadRBIPolicy();
    banks = storage.loadBanks();

    if (rbiPolicy.lastUpdatedDate.empty()) {
        rbiPolicy.repoRate = 6.50;
        rbiPolicy.reverseRepoRate = 6.25;
        rbiPolicy.crr = 4.50;
        rbiPolicy.slr = 18.00;
        rbiPolicy.lastUpdatedDate = Utils::getCurrentDate();
        rbiPolicy.circulars.push_back("Welcome to Reserve Bank of India Central Engine System");
        saveData();
    }
}

void RBIController::saveData() {
    storage.saveRBIPolicy(rbiPolicy);
    storage.saveBanks(banks);
}

void RBIController::setRepoRate(double rate) {
    if (!authManager.checkPermission(Role::RBI_GOVERNOR)) {
        Utils::printError("Unauthorized! Only RBI Governor can modify Repo Rate.");
        return;
    }
    double oldRate = rbiPolicy.repoRate;
    rbiPolicy.repoRate = rate;
    rbiPolicy.lastUpdatedDate = Utils::getCurrentDate();

    // Adjust bank base rates automatically based on repo rate change
    double delta = rate - oldRate;
    for (auto& b : banks) {
        b.policies.homeLoanBaseRate += delta * 0.8; // 80% transmission
        b.policies.personalLoanBaseRate += delta * 0.8;
    }

    std::string msg = "Repo Rate updated from " + std::to_string(oldRate) + "% to " + std::to_string(rate) + "%";
    broadcastCircular(msg);
    authManager.logAction("RBI_REPO_RATE_CHANGE", "RBI", msg);
    saveData();
    Utils::printSuccess(msg + " - Transmitted across commercial bank base rates.");
}

void RBIController::setReverseRepoRate(double rate) {
    if (!authManager.checkPermission(Role::RBI_GOVERNOR)) {
        Utils::printError("Unauthorized! Only RBI Governor can modify Reverse Repo Rate.");
        return;
    }
    rbiPolicy.reverseRepoRate = rate;
    rbiPolicy.lastUpdatedDate = Utils::getCurrentDate();
    authManager.logAction("RBI_REVERSE_REPO_CHANGE", "RBI", "Reverse Repo Rate set to " + std::to_string(rate) + "%");
    saveData();
    Utils::printSuccess("Reverse Repo Rate set to " + std::to_string(rate) + "%");
}

void RBIController::setCRR(double percent) {
    if (!authManager.checkPermission(Role::RBI_GOVERNOR)) {
        Utils::printError("Unauthorized! Only RBI Governor can modify CRR.");
        return;
    }
    rbiPolicy.crr = percent;
    rbiPolicy.lastUpdatedDate = Utils::getCurrentDate();
    std::string msg = "CRR updated to " + std::to_string(percent) + "%";
    broadcastCircular(msg);
    saveData();
    Utils::printSuccess(msg);
}

void RBIController::setSLR(double percent) {
    if (!authManager.checkPermission(Role::RBI_GOVERNOR)) {
        Utils::printError("Unauthorized! Only RBI Governor can modify SLR.");
        return;
    }
    rbiPolicy.slr = percent;
    rbiPolicy.lastUpdatedDate = Utils::getCurrentDate();
    std::string msg = "SLR updated to " + std::to_string(percent) + "%";
    broadcastCircular(msg);
    saveData();
    Utils::printSuccess(msg);
}

void RBIController::broadcastCircular(const std::string& message) {
    std::string fullMsg = "[" + Utils::getCurrentDate() + "] RBI CIRCULAR: " + message;
    rbiPolicy.circulars.push_back(fullMsg);
    saveData();
}

bool RBIController::registerBank(const std::string& name, const std::string& code, const std::string& cmdStaffId, bool overrideAuth) {
    if (!overrideAuth && !authManager.checkPermission(Role::RBI_GOVERNOR)) {
        Utils::printError("Only RBI Governor can license new banks.");
        return false;
    }
    for (const auto& b : banks) {
        if (b.code == code) {
            Utils::printError("Bank code " + code + " already exists!");
            return false;
        }
    }
    BankInfo b;
    b.bankId = Utils::generateRandomId("BNK");
    b.name = name;
    b.code = code;
    b.cmdStaffId = cmdStaffId;
    b.isLicensed = true;
    
    banks.push_back(b);
    saveData();
    authManager.logAction("BANK_LICENSED", b.bankId, "Issued Banking License to " + name + " (" + code + ")");
    Utils::printSuccess("Banking license issued to " + name + " [Bank ID: " + b.bankId + "]");
    return true;
}

bool RBIController::revokeBankLicense(const std::string& bankId) {
    if (!authManager.checkPermission(Role::RBI_GOVERNOR)) {
        Utils::printError("Only RBI Governor can revoke banking licenses.");
        return false;
    }
    for (auto& b : banks) {
        if (b.bankId == bankId) {
            b.isLicensed = false;
            saveData();
            authManager.logAction("BANK_LICENSE_REVOKED", bankId, "Revoked Banking License for " + b.name);
            Utils::printWarning("Banking License REVOKED for " + b.name);
            return true;
        }
    }
    Utils::printError("Bank ID not found.");
    return false;
}

void RBIController::runSystemicAudit() {
    Utils::printHeader("RESERVE BANK OF INDIA — SYSTEMIC RISK AUDIT REPORT");
    std::cout << Utils::BOLD << std::left
              << std::setw(10) << "Bank ID"
              << std::setw(20) << "Bank Name"
              << std::setw(15) << "Total Deposits"
              << std::setw(15) << "Loans Active"
              << std::setw(12) << "Total NPA"
              << std::setw(10) << "Status" << Utils::RESET << "\n";
    Utils::printDivider('-', 82);

    double grandDeposits = 0, grandLoans = 0, grandNPA = 0;
    for (const auto& b : banks) {
        grandDeposits += b.totalDeposits;
        grandLoans += b.totalLoansDisbursed;
        grandNPA += b.totalNPA;

        std::cout << std::left
                  << std::setw(10) << b.bankId
                  << std::setw(20) << b.name
                  << std::setw(15) << Utils::formatCurrency(b.totalDeposits)
                  << std::setw(15) << Utils::formatCurrency(b.totalLoansDisbursed)
                  << std::setw(12) << Utils::formatCurrency(b.totalNPA)
                  << std::setw(10) << (b.isLicensed ? (Utils::GREEN + std::string("ACTIVE") + Utils::RESET)
                                                    : (Utils::RED + std::string("REVOKED") + Utils::RESET))
                  << "\n";
    }
    Utils::printDivider('=', 82);
    std::cout << Utils::BOLD << "Total Systemic Deposits : " << Utils::formatCurrency(grandDeposits) << "\n"
              << "Total Systemic Loans    : " << Utils::formatCurrency(grandLoans) << "\n"
              << "Total Systemic NPA      : " << Utils::formatCurrency(grandNPA) << Utils::RESET << "\n";
}

void RBIController::displayMonetaryPolicy() {
    Utils::printHeader("RESERVE BANK OF INDIA — CURRENT MONETARY POLICY");
    std::cout << "  Repo Rate         : " << Utils::BOLD << Utils::GREEN << rbiPolicy.repoRate << "%" << Utils::RESET << "\n";
    std::cout << "  Reverse Repo Rate : " << Utils::BOLD << Utils::CYAN << rbiPolicy.reverseRepoRate << "%" << Utils::RESET << "\n";
    std::cout << "  CRR (Cash Reserve): " << Utils::BOLD << Utils::YELLOW << rbiPolicy.crr << "%" << Utils::RESET << "\n";
    std::cout << "  SLR (Liquidity)   : " << Utils::BOLD << Utils::MAGENTA << rbiPolicy.slr << "%" << Utils::RESET << "\n";
    std::cout << "  Last Revised      : " << rbiPolicy.lastUpdatedDate << "\n";
}

void RBIController::displayAllBanksMetrics() {
    displayMonetaryPolicy();
    runSystemicAudit();
}

void RBIController::displayCirculars() {
    Utils::printHeader("RBI CIRCULARS & NOTIFICATIONS");
    if (rbiPolicy.circulars.empty()) {
        std::cout << "No circulars broadcasted.\n";
        return;
    }
    for (const auto& c : rbiPolicy.circulars) {
        std::cout << " * " << c << "\n";
    }
}

BankInfo* RBIController::findBank(const std::string& bankId) {
    for (auto& b : banks) {
        if (b.bankId == bankId) return &b;
    }
    return nullptr;
}

BankInfo* RBIController::findBankByCode(const std::string& code) {
    for (auto& b : banks) {
        if (b.code == code) return &b;
    }
    return nullptr;
}

} // namespace BankingSystem
