#ifndef REPORTS_H
#define REPORTS_H

#include "structures.h"
#include "storage.h"
#include "auth.h"
#include "rbi.h"
#include "account.h"
#include "loan.h"
#include "fd_rd.h"
#include "insurance.h"
#include "credit_card.h"

namespace BankingSystem {

class ReportGenerator {
private:
    Storage& storage;
    AuthManager& authManager;
    RBIController& rbiController;
    AccountManager& accountManager;
    LoanManager& loanManager;
    FDRDManager& fdrdManager;
    InsuranceManager& insuranceManager;
    CreditCardManager& creditCardManager;

public:
    ReportGenerator(Storage& store, AuthManager& auth, RBIController& rbi, AccountManager& acc, LoanManager& loan, FDRDManager& fd, InsuranceManager& ins, CreditCardManager& cc);

    void generateCustomerDashboard(const std::string& customerId);
    void generateBranchReport(const std::string& bankId, const std::string& branchCode);
    void generateBankBalanceSheet(const std::string& bankId);
    void generateRBIGovernorDashboard();
};

} // namespace BankingSystem

#endif // REPORTS_H
