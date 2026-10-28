#include "loan.h"
#include "utils.h"
#include <iostream>
#include <iomanip>

namespace BankingSystem
{

    LoanManager::LoanManager(Storage &store, AuthManager &auth, RBIController &rbi, AccountManager &acc)
        : storage(store), authManager(auth), rbiController(rbi), accountManager(acc)
    {
        loadLoans();
    }

    void LoanManager::loadLoans()
    {
        std::vector<Loan> loaded = storage.loadLoans();
        loans.clear();
        loans.insert(loans.end(), loaded.begin(), loaded.end());
    }

    void LoanManager::saveLoans()
    {
        std::vector<Loan> snapshot(loans.begin(), loans.end());
        storage.saveLoans(snapshot);
    }

    Loan *LoanManager::applyForLoan(const std::string &customerId, const std::string &bankId, const std::string &linkedAccNo, LoanType type, double principal, int tenureMonths)
    {
        Customer *cust = authManager.findCustomer(customerId);
        if (!cust)
        {
            Utils::printError("Customer not found.");
            return nullptr;
        }
        if (!cust->isKycVerified)
        {
            Utils::printError("KYC verification required before applying for loan.");
            return nullptr;
        }

        // Minimum credit score check
        int minRequiredScore = (type == LoanType::HOME) ? 750 : 650;
        if (cust->creditScore < minRequiredScore)
        {
            Utils::printError("Loan application auto-rejected! Credit score (" + std::to_string(cust->creditScore) + ") is below required threshold (" + std::to_string(minRequiredScore) + ").");
            return nullptr;
        }

        BankInfo *b = rbiController.findBank(bankId);
        if (!b)
        {
            Utils::printError("Bank not found.");
            return nullptr;
        }

        const Money principalAmount = Money::fromRupees(principal);
        double rate = b->policies.homeLoanBaseRate;
        if (type == LoanType::PERSONAL)
            rate = b->policies.personalLoanBaseRate;
        else if (type == LoanType::CAR)
            rate = b->policies.carLoanBaseRate;
        else if (type == LoanType::EDUCATION)
            rate = b->policies.eduLoanBaseRate;

        double emi = Utils::calculateEMI(principal, rate, tenureMonths);
        const Money emiAmount = Money::fromRupees(emi);

        Loan l;
        l.loanId = Utils::generateRandomId("LON");
        l.customerId = customerId;
        l.bankId = bankId;
        l.linkedAccountNo = linkedAccNo;
        l.type = type;
        l.principalAmount = principalAmount;
        l.interestRate = rate;
        l.tenureMonths = tenureMonths;
        l.emiAmount = emiAmount;
        l.totalOutstanding = emiAmount * static_cast<double>(tenureMonths);
        l.totalPaid = Money{};
        l.status = LoanStatus::UNDER_LOAN_OFFICER;
        l.appliedDate = Utils::getCurrentDate();
        l.sanctionedDate = "PENDING";
        l.approvalHistory = "Applied on " + l.appliedDate + "; ";

        loans.push_back(l);
        saveLoans();

        authManager.logAction("LOAN_APPLIED", bankId, "Customer " + cust->name + " applied for " + loanTypeToString(type) + " of " + Utils::formatCurrency(principal));
        Utils::printSuccess("Loan Application Submitted! Loan ID: " + l.loanId + " (EMI: " + Utils::formatCurrency(emi) + "/mo)");
        return &loans.back();
    }

    bool LoanManager::reviewLoanOfficer(const std::string &loanId, bool approve, const std::string &comments)
    {
        Loan *l = findLoan(loanId);
        if (!l || l->status != LoanStatus::UNDER_LOAN_OFFICER)
        {
            Utils::printError("Loan not found or not in Loan Officer review state.");
            return false;
        }
        if (!authManager.checkPermission(Role::LOAN_OFFICER))
        {
            Utils::printError("Unauthorized! Loan Officer role required.");
            return false;
        }

        if (!approve)
        {
            l->status = LoanStatus::REJECTED;
            l->approvalHistory += "Rejected by Loan Officer: " + comments + "; ";
            saveLoans();
            Utils::printWarning("Loan #" + loanId + " REJECTED by Loan Officer.");
            return true;
        }

        // Determine next escalation level based on loan principal amount
        const Money threshold25L = Money::fromRupees(2500000.0);
        const Money threshold1Cr = Money::fromRupees(10000000.0);
        if (l->principalAmount < threshold25L)
        { // < ₹25 Lakhs
            l->status = LoanStatus::UNDER_BRANCH_MANAGER;
            l->approvalHistory += "Passed LO review -> Escalated to Branch Manager; ";
        }
        else if (l->principalAmount <= threshold1Cr)
        { // ₹25L to ₹1 Crore
            l->status = LoanStatus::UNDER_ZONAL_MANAGER;
            l->approvalHistory += "Passed LO review -> Escalated to Zonal Manager (>Rs.25L); ";
        }
        else
        { // > ₹1 Crore
            l->status = LoanStatus::UNDER_CMD;
            l->approvalHistory += "Passed LO review -> Escalated to Bank CMD (>Rs.1Cr); ";
        }

        saveLoans();
        Utils::printSuccess("Loan Officer approved loan #" + loanId + ". Next Status: " + loanStatusToString(l->status));
        return true;
    }

    bool LoanManager::reviewBranchManager(const std::string &loanId, bool approve, const std::string &comments)
    {
        Loan *l = findLoan(loanId);
        if (!l || l->status != LoanStatus::UNDER_BRANCH_MANAGER)
        {
            Utils::printError("Loan not found or not in Branch Manager approval queue.");
            return false;
        }
        if (!authManager.checkPermission(Role::BRANCH_MANAGER))
        {
            Utils::printError("Unauthorized! Branch Manager role required.");
            return false;
        }

        if (approve)
        {
            l->status = LoanStatus::APPROVED;
            l->sanctionedDate = Utils::getCurrentDate();
            l->approvalHistory += "Approved by Branch Manager: " + comments + "; ";
            Utils::printSuccess("Loan #" + loanId + " SANCTIONED & APPROVED by Branch Manager!");
        }
        else
        {
            l->status = LoanStatus::REJECTED;
            l->approvalHistory += "Rejected by Branch Manager: " + comments + "; ";
            Utils::printWarning("Loan #" + loanId + " REJECTED by Branch Manager.");
        }
        saveLoans();
        return true;
    }

    bool LoanManager::reviewZonalManager(const std::string &loanId, bool approve, const std::string &comments)
    {
        Loan *l = findLoan(loanId);
        if (!l || l->status != LoanStatus::UNDER_ZONAL_MANAGER)
        {
            Utils::printError("Loan not found or not in Zonal Manager approval queue.");
            return false;
        }
        if (!authManager.checkPermission(Role::ZONAL_MANAGER))
        {
            Utils::printError("Unauthorized! Zonal Manager role required.");
            return false;
        }

        if (approve)
        {
            l->status = LoanStatus::APPROVED;
            l->sanctionedDate = Utils::getCurrentDate();
            l->approvalHistory += "Approved by Zonal Manager: " + comments + "; ";
            Utils::printSuccess("Loan #" + loanId + " SANCTIONED & APPROVED by Zonal Manager!");
        }
        else
        {
            l->status = LoanStatus::REJECTED;
            l->approvalHistory += "Rejected by Zonal Manager: " + comments + "; ";
            Utils::printWarning("Loan #" + loanId + " REJECTED by Zonal Manager.");
        }
        saveLoans();
        return true;
    }

    bool LoanManager::reviewCMD(const std::string &loanId, bool approve, const std::string &comments)
    {
        Loan *l = findLoan(loanId);
        if (!l || l->status != LoanStatus::UNDER_CMD)
        {
            Utils::printError("Loan not found or not in CMD approval queue.");
            return false;
        }
        if (!authManager.checkPermission(Role::BANK_CMD))
        {
            Utils::printError("Unauthorized! Bank CMD role required.");
            return false;
        }

        if (approve)
        {
            l->status = LoanStatus::APPROVED;
            l->sanctionedDate = Utils::getCurrentDate();
            l->approvalHistory += "Approved by Bank CMD: " + comments + "; ";
            Utils::printSuccess("High-Value Loan #" + loanId + " SANCTIONED & APPROVED by Bank CMD!");
        }
        else
        {
            l->status = LoanStatus::REJECTED;
            l->approvalHistory += "Rejected by Bank CMD: " + comments + "; ";
            Utils::printWarning("Loan #" + loanId + " REJECTED by Bank CMD.");
        }
        saveLoans();
        return true;
    }

    bool LoanManager::disburseLoan(const std::string &loanId)
    {
        Loan *l = findLoan(loanId);
        if (!l || l->status != LoanStatus::APPROVED)
        {
            Utils::printError("Loan is not approved for disbursal.");
            return false;
        }

        Account *acc = accountManager.findAccount(l->linkedAccountNo);
        if (!acc)
        {
            Utils::printError("Linked disbursement account not found.");
            return false;
        }

        acc->balance += l->principalAmount;
        l->status = LoanStatus::DISBURSED;
        l->approvalHistory += "Disbursed to Acc #" + acc->accountNo + " on " + Utils::getCurrentDate() + "; ";

        saveLoans();
        accountManager.saveAccounts();
        authManager.logAction("LOAN_DISBURSED", l->bankId, "Disbursed " + Utils::formatCurrency(l->principalAmount) + " for Loan #" + loanId);
        Utils::printSuccess("Loan Amount " + Utils::formatCurrency(l->principalAmount) + " credited to Account #" + acc->accountNo);
        return true;
    }

    bool LoanManager::payEMI(const std::string &loanId, double amount)
    {
        Loan *l = findLoan(loanId);
        if (!l || l->status != LoanStatus::DISBURSED)
        {
            Utils::printError("Loan not active for repayment.");
            return false;
        }

        const Money emiAmount = Money::fromRupees(amount);
        Account *acc = accountManager.findAccount(l->linkedAccountNo);
        if (!acc || acc->balance < emiAmount)
        {
            Utils::printError("Insufficient account balance to pay EMI.");
            return false;
        }

        acc->balance -= emiAmount;
        l->totalPaid += emiAmount;
        l->totalOutstanding -= emiAmount;

        if (l->totalOutstanding <= Money{})
        {
            l->totalOutstanding = Money{};
            l->status = LoanStatus::CLOSED;
            Utils::printSuccess("🎉 Loan #" + loanId + " is FULLY REPAID and CLOSED!");
        }
        else
        {
            Utils::printSuccess("EMI Payment Received: " + Utils::formatCurrency(amount) + ". Remaining Outstanding: " + Utils::formatCurrency(static_cast<double>(l->totalOutstanding)));
        }

        saveLoans();
        accountManager.saveAccounts();
        return true;
    }

    void LoanManager::printSanctionLetter(const std::string &loanId)
    {
        Loan *l = findLoan(loanId);
        if (!l)
        {
            Utils::printError("Loan not found.");
            return;
        }
        Customer *cust = authManager.findCustomer(l->customerId);
        BankInfo *bank = rbiController.findBank(l->bankId);

        Utils::printHeader("OFFICIAL LOAN SANCTION LETTER — " + (bank ? bank->name : "BANK"));
        std::cout << " Sanction Date  : " << l->sanctionedDate << "\n";
        std::cout << " Loan Ref ID    : " << l->loanId << "\n";
        std::cout << " Applicant Name : " << (cust ? cust->name : l->customerId) << "\n";
        std::cout << " Loan Type      : " << loanTypeToString(l->type) << "\n";
        std::cout << " Sanction Amount: " << Utils::formatCurrency(static_cast<double>(l->principalAmount)) << "\n";
        std::cout << " Interest Rate  : " << l->interestRate << "% p.a.\n";
        std::cout << " Tenure         : " << l->tenureMonths << " Months\n";
        std::cout << " Calculated EMI : " << Utils::formatCurrency(static_cast<double>(l->emiAmount)) << " / month\n";
        std::cout << " Loan Status    : " << loanStatusToString(l->status) << "\n";
        std::cout << " Approval Chain : " << l->approvalHistory << "\n";
        Utils::printDivider('=', 70);
    }

    Loan *LoanManager::findLoan(const std::string &loanId)
    {
        for (auto &l : loans)
        {
            if (l.loanId == loanId)
                return &l;
        }
        return nullptr;
    }

    std::vector<Loan> LoanManager::getCustomerLoans(const std::string &customerId)
    {
        std::vector<Loan> res;
        for (const auto &l : loans)
        {
            if (l.customerId == customerId)
                res.push_back(l);
        }
        return res;
    }

    std::vector<Loan> LoanManager::getPendingLoansForRole(Role role, const std::string &bankId)
    {
        std::vector<Loan> res;
        for (const auto &l : loans)
        {
            if (l.bankId != bankId)
                continue;
            if (role == Role::LOAN_OFFICER && l.status == LoanStatus::UNDER_LOAN_OFFICER)
                res.push_back(l);
            else if (role == Role::BRANCH_MANAGER && l.status == LoanStatus::UNDER_BRANCH_MANAGER)
                res.push_back(l);
            else if (role == Role::ZONAL_MANAGER && l.status == LoanStatus::UNDER_ZONAL_MANAGER)
                res.push_back(l);
            else if (role == Role::BANK_CMD && l.status == LoanStatus::UNDER_CMD)
                res.push_back(l);
        }
        return res;
    }

} // namespace BankingSystem
