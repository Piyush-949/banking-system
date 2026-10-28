#include "reports.h"
#include "utils.h"
#include <iostream>
#include <iomanip>

namespace BankingSystem {

ReportGenerator::ReportGenerator(Storage& store, AuthManager& auth, RBIController& rbi, AccountManager& acc, LoanManager& loan, FDRDManager& fd, InsuranceManager& ins, CreditCardManager& cc)
    : storage(store), authManager(auth), rbiController(rbi), accountManager(acc), loanManager(loan), fdrdManager(fd), insuranceManager(ins), creditCardManager(cc) {}

void ReportGenerator::generateCustomerDashboard(const std::string& customerId) {
    Customer* cust = authManager.findCustomer(customerId);
    if (!cust) {
        Utils::printError("Customer profile not found.");
        return;
    }

    Utils::printHeader("CUSTOMER COMPLETE FINANCIAL PORTFOLIO — " + cust->name);
    std::cout << " Customer ID  : " << cust->customerId << " | PAN: " << cust->pan << "\n";
    std::cout << " Credit Score : " << cust->creditScore << " / 900 (" << (cust->creditScore >= 750 ? "EXCELLENT" : "AVERAGE") << ")\n";
    std::cout << " KYC Status   : " << (cust->isKycVerified ? "VERIFIED ✅" : "PENDING ❌") << "\n\n";

    // Accounts
    std::cout << Utils::BOLD << "--- ACCOUNTS ---" << Utils::RESET << "\n";
    auto accs = accountManager.getCustomerAccounts(customerId);
    if (accs.empty()) std::cout << " No active bank accounts.\n";
    for (const auto& a : accs) {
        std::cout << " Account #" << a.accountNo << " (" << accountTypeToString(a.type) << ") : " << Utils::formatCurrency(a.balance) << "\n";
    }

    // Loans
    std::cout << "\n" << Utils::BOLD << "--- LOANS ---" << Utils::RESET << "\n";
    auto loans = loanManager.getCustomerLoans(customerId);
    if (loans.empty()) std::cout << " No active loan contracts.\n";
    for (const auto& l : loans) {
        std::cout << " Loan #" << l.loanId << " (" << loanTypeToString(l.type) << ") : Principal " << Utils::formatCurrency(l.principalAmount) << " | Outstanding: " << Utils::formatCurrency(l.totalOutstanding) << " [" << loanStatusToString(l.status) << "]\n";
    }

    // FDs & RDs
    std::cout << "\n" << Utils::BOLD << "--- FIXED DEPOSITS & RD ---" << Utils::RESET << "\n";
    auto fds = fdrdManager.getCustomerFDs(customerId);
    for (const auto& f : fds) {
        std::cout << " FD #" << f.fdNo << " : Principal " << Utils::formatCurrency(f.principal) << " @ " << f.interestRate << "% | Maturity: " << Utils::formatCurrency(f.maturityAmount) << "\n";
    }

    // Credit Cards
    std::cout << "\n" << Utils::BOLD << "--- CREDIT CARDS ---" << Utils::RESET << "\n";
    auto ccs = creditCardManager.getCustomerCards(customerId);
    for (const auto& c : ccs) {
        std::cout << " Card #" << c.cardNumber << " (" << creditCardTypeToString(c.type) << ") : Limit " << Utils::formatCurrency(c.creditLimit) << " | Outstanding: " << Utils::formatCurrency(c.currentOutstanding) << "\n";
    }

    // Insurance
    std::cout << "\n" << Utils::BOLD << "--- INSURANCE POLICIES ---" << Utils::RESET << "\n";
    auto inss = insuranceManager.getCustomerPolicies(customerId);
    for (const auto& i : inss) {
        std::cout << " Policy #" << i.policyNo << " (" << insuranceTypeToString(i.type) << ") : Sum Assured " << Utils::formatCurrency(i.coverageAmount) << " | Status: " << i.claimStatus << "\n";
    }
    Utils::printDivider('=', 75);
}

void ReportGenerator::generateBranchReport(const std::string& bankId, const std::string& branchCode) {
    Utils::printHeader("BRANCH PERFORMANCE REPORT — BRANCH: " + branchCode);
    
    int accCount = 0;
    double totalDeposits = 0.0;
    for (const auto& a : accountManager.getAllAccounts()) {
        if (a.bankId == bankId && a.branchCode == branchCode) {
            accCount++;
            totalDeposits += a.balance;
        }
    }

    std::cout << " Total Active Accounts : " << accCount << "\n";
    std::cout << " Total Deposits Held   : " << Utils::formatCurrency(totalDeposits) << "\n";
    Utils::printDivider('=', 70);
}

void ReportGenerator::generateBankBalanceSheet(const std::string& bankId) {
    BankInfo* b = rbiController.findBank(bankId);
    if (!b) return;

    Utils::printHeader("BANK FINANCIAL BALANCE SHEET — " + b->name);
    std::cout << " Bank Code            : " << b->code << "\n";
    std::cout << " Licensing Status     : " << (b->isLicensed ? "LICENSED & REGULATED" : "SUSPENDED") << "\n";
    std::cout << " Total Customer Deposit: " << Utils::formatCurrency(b->totalDeposits) << "\n";
    std::cout << " Total Disbursed Loans : " << Utils::formatCurrency(b->totalLoansDisbursed) << "\n";
    std::cout << " Non-Performing Assets : " << Utils::formatCurrency(b->totalNPA) << "\n";
    Utils::printDivider('=', 70);
}

void ReportGenerator::generateRBIGovernorDashboard() {
    rbiController.displayMonetaryPolicy();
    rbiController.runSystemicAudit();
}

} // namespace BankingSystem
