#ifndef LOAN_H
#define LOAN_H

#include "structures.h"
#include "storage.h"
#include "auth.h"
#include "rbi.h"
#include "account.h"
#include <deque>
#include <vector>

namespace BankingSystem
{

    class LoanManager
    {
    private:
        Storage &storage;
        AuthManager &authManager;
        RBIController &rbiController;
        AccountManager &accountManager;
        std::deque<Loan> loans;

    public:
        LoanManager(Storage &store, AuthManager &auth, RBIController &rbi, AccountManager &acc);

        void loadLoans();
        void saveLoans();

        // Customer loan application
        Loan *applyForLoan(const std::string &customerId, const std::string &bankId, const std::string &linkedAccNo, LoanType type, double principal, int tenureMonths);

        // Multi-tier workflow approvals
        bool reviewLoanOfficer(const std::string &loanId, bool approve, const std::string &comments);
        bool reviewBranchManager(const std::string &loanId, bool approve, const std::string &comments);
        bool reviewZonalManager(const std::string &loanId, bool approve, const std::string &comments);
        bool reviewCMD(const std::string &loanId, bool approve, const std::string &comments);

        // Disbursal & Repayment
        bool disburseLoan(const std::string &loanId);
        bool payEMI(const std::string &loanId, double amount);
        void printSanctionLetter(const std::string &loanId);
        void checkAndMarkNPA();

        Loan *findLoan(const std::string &loanId);
        std::vector<Loan> getCustomerLoans(const std::string &customerId);
        std::vector<Loan> getPendingLoansForRole(Role role, const std::string &bankId);
    };

} // namespace BankingSystem

#endif // LOAN_H
