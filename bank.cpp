#include "bank.h"
#include "utils.h"
#include <iostream>
#include <iomanip>

namespace BankingSystem {

BankEngine::BankEngine(Storage& store, AuthManager& auth, RBIController& rbi)
    : storage(store), authManager(auth), rbiController(rbi) {}

void BankEngine::updateBankInterestRates(const std::string& bankId, double savingsROI, double homeROI, double personalROI, double fd1Yr) {
    if (!authManager.checkPermission(Role::BANK_CMD)) {
        Utils::printError("Unauthorized! Only Bank CMD / CEO can modify bank-level interest rate policies.");
        return;
    }
    BankInfo* bank = rbiController.findBank(bankId);
    if (!bank) {
        Utils::printError("Bank not found.");
        return;
    }

    bank->policies.savingsInterestRate = savingsROI;
    bank->policies.homeLoanBaseRate = homeROI;
    bank->policies.personalLoanBaseRate = personalROI;
    bank->policies.fd1YrRate = fd1Yr;

    rbiController.saveData();
    authManager.logAction("UPDATE_BANK_RATES", bankId, "Updated Savings ROI: " + std::to_string(savingsROI) + "%, Home Loan: " + std::to_string(homeROI) + "%");
    Utils::printSuccess("Successfully updated interest rates for " + bank->name);
}

void BankEngine::displayBankComparisonTable() {
    Utils::printHeader("PUBLIC BANK COMPARISON & PERKS MATRIX");
    std::cout << Utils::BOLD << std::left
              << std::setw(8) << "Code"
              << std::setw(22) << "Bank Name"
              << std::setw(14) << "Savings ROI"
              << std::setw(15) << "Home Loan ROI"
              << std::setw(15) << "Personal Loan"
              << std::setw(12) << "FD 1-Yr"
              << std::setw(12) << "Sr Citizen" << Utils::RESET << "\n";
    Utils::printDivider('-', 98);

    const auto& banks = rbiController.getBanks();
    for (const auto& b : banks) {
        if (!b.isLicensed) continue;
        std::cout << std::left
                  << std::setw(8) << b.code
                  << std::setw(22) << b.name
                  << std::setw(14) << (std::to_string(b.policies.savingsInterestRate).substr(0, 4) + "%")
                  << std::setw(15) << (std::to_string(b.policies.homeLoanBaseRate).substr(0, 4) + "%")
                  << std::setw(15) << (std::to_string(b.policies.personalLoanBaseRate).substr(0, 4) + "%")
                  << std::setw(12) << (std::to_string(b.policies.fd1YrRate).substr(0, 4) + "%")
                  << std::setw(12) << ("+" + std::to_string(b.policies.seniorCitizenBonus).substr(0, 4) + "%")
                  << "\n";
    }
    Utils::printDivider('=', 98);
}

void BankEngine::displayProductBrochure(const std::string& bankId) {
    BankInfo* b = rbiController.findBank(bankId);
    if (!b) {
        Utils::printError("Bank not found.");
        return;
    }
    Utils::printHeader("PRODUCT BROCHURE & PERKS — " + b->name);
    std::cout << " 🏦 Min Balance (Savings) : " << Utils::formatCurrency(b->policies.minSavingsBalance) << "\n";
    std::cout << " 📈 Savings Account ROI   : " << b->policies.savingsInterestRate << "%\n";
    std::cout << " 🏡 Home Loan Rate        : Starting @ " << b->policies.homeLoanBaseRate << "%\n";
    std::cout << " 👤 Personal Loan Rate    : Starting @ " << b->policies.personalLoanBaseRate << "%\n";
    std::cout << " 🚗 Car Loan Rate         : Starting @ " << b->policies.carLoanBaseRate << "%\n";
    std::cout << " 🎓 Education Loan Rate   : Starting @ " << b->policies.eduLoanBaseRate << "%\n";
    std::cout << " 💰 Fixed Deposit (1 Year): " << b->policies.fd1YrRate << "%\n";
    std::cout << " 💰 Fixed Deposit (3 Year): " << b->policies.fd3YrRate << "%\n";
    std::cout << " 💰 Fixed Deposit (5 Year): " << b->policies.fd5YrRate << "%\n";
    std::cout << " 👵 Senior Citizen Bonus  : Extra " << b->policies.seniorCitizenBonus << "% on FDs\n";
    std::cout << " 💳 Credit Cards Available: Silver Classic, Gold Rewards, Platinum Select, Super Premium\n";
    std::cout << " 🛡️ Insurance Options     : Term Life, Mediclaim, Vehicle Comprehensive, Home Protector\n";
}

void BankEngine::recalculateBankTotals(const std::string& bankId) {
    BankInfo* b = rbiController.findBank(bankId);
    if (!b) return;

    double deposits = 0.0;
    auto accounts = storage.loadAccounts();
    for (const auto& a : accounts) {
        if (a.bankId == bankId && a.isActive) {
            deposits += a.balance;
        }
    }
    b->totalDeposits = deposits;

    double loans = 0.0;
    double npa = 0.0;
    auto loanList = storage.loadLoans();
    for (const auto& l : loanList) {
        if (l.bankId == bankId) {
            if (l.status == LoanStatus::DISBURSED) {
                loans += l.totalOutstanding;
            } else if (l.status == LoanStatus::NPA_DEFAULT) {
                npa += l.totalOutstanding;
            }
        }
    }
    b->totalLoansDisbursed = loans;
    b->totalNPA = npa;

    rbiController.saveData();
}

} // namespace BankingSystem
