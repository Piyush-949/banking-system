#include "fd_rd.h"
#include "utils.h"
#include <iostream>
#include <cmath>

namespace BankingSystem
{

    FDRDManager::FDRDManager(Storage &store, AuthManager &auth, RBIController &rbi, AccountManager &acc)
        : storage(store), authManager(auth), rbiController(rbi), accountManager(acc)
    {
        loadFDRD();
    }

    void FDRDManager::loadFDRD()
    {
        fixedDeposits = storage.loadFixedDeposits();
        recurringDeposits = storage.loadRecurringDeposits();
    }

    void FDRDManager::saveFDRD()
    {
        storage.saveFixedDeposits(fixedDeposits);
        storage.saveRecurringDeposits(recurringDeposits);
    }

    FixedDeposit *FDRDManager::createFD(const std::string &customerId, const std::string &bankId, const std::string &linkedAccNo, double principal, int tenureMonths)
    {
        const Money principalAmount = Money::fromRupees(principal);
        Account *acc = accountManager.findAccount(linkedAccNo);
        if (!acc || acc->balance < principalAmount)
        {
            Utils::printError("Insufficient balance in linked account to open Fixed Deposit.");
            return nullptr;
        }
        Customer *cust = authManager.findCustomer(customerId);
        if (!cust)
            return nullptr;

        BankInfo *b = rbiController.findBank(bankId);
        if (!b)
            return nullptr;

        double baseRate = b->policies.fd1YrRate;
        if (tenureMonths >= 36 && tenureMonths < 60)
            baseRate = b->policies.fd3YrRate;
        else if (tenureMonths >= 60)
            baseRate = b->policies.fd5YrRate;

        if (cust->isSeniorCitizen)
        {
            baseRate += b->policies.seniorCitizenBonus;
        }

        // Compound interest calculation: A = P(1 + r/4)^(4*t)
        const double years = tenureMonths / 12.0;
        const double maturityValue = principal * std::pow(1.0 + (baseRate / 400.0), 4.0 * years);
        const Money maturityAmount = Money::fromRupees(maturityValue);
        const Money totalInterest = maturityAmount - principalAmount;

        // TDS deduction rule: 10% TDS on annual interest exceeding ₹40,000
        Money tds = Money{};
        if (totalInterest > Money::fromRupees(40000.0))
        {
            const Money tdsBase = totalInterest - Money::fromRupees(40000.0);
            tds = tdsBase * 0.10;
        }

        // Deduct principal from savings account
        acc->balance -= principalAmount;
        accountManager.saveAccounts();

        FixedDeposit fd;
        fd.fdNo = Utils::generateRandomId("FD");
        fd.customerId = customerId;
        fd.bankId = bankId;
        fd.linkedAccountNo = linkedAccNo;
        fd.principal = principalAmount;
        fd.tenureMonths = tenureMonths;
        fd.interestRate = baseRate;
        fd.maturityAmount = maturityAmount - tds;
        fd.startDate = Utils::getCurrentDate();
        fd.maturityDate = Utils::getCurrentDate() + " (+ " + std::to_string(tenureMonths) + " M)";
        fd.isSeniorCitizen = cust->isSeniorCitizen;
        fd.isClosed = false;
        fd.tdsDeducted = tds;

        fixedDeposits.push_back(fd);
        saveFDRD();

        authManager.logAction("FD_CREATED", bankId, "Opened FD #" + fd.fdNo + " for " + Utils::formatCurrency(principal) + " @ " + std::to_string(baseRate) + "%");
        Utils::printSuccess("Fixed Deposit Opened! FD No: " + fd.fdNo + " | Maturity Amount: " + Utils::formatCurrency(static_cast<double>(fd.maturityAmount)));
        if (!tds.isZero())
            std::cout << Utils::YELLOW << " [TDS Alert] Projected TDS deduction of " << Utils::formatCurrency(static_cast<double>(tds)) << " applied." << Utils::RESET << "\n";
        return &fixedDeposits.back();
    }

    bool FDRDManager::prematureWithdrawFD(const std::string &fdNo)
    {
        FixedDeposit *fd = nullptr;
        for (auto &f : fixedDeposits)
        {
            if (f.fdNo == fdNo && !f.isClosed)
            {
                fd = &f;
                break;
            }
        }
        if (!fd)
        {
            Utils::printError("Active Fixed Deposit not found.");
            return false;
        }

        // 1% premature withdrawal penalty on rate
        double penaltyRate = fd->interestRate - 1.0;
        if (penaltyRate < 0)
            penaltyRate = 0;

        const double years = fd->tenureMonths / 12.0;
        const double reducedMaturityValue = static_cast<double>(fd->principal) * std::pow(1.0 + (penaltyRate / 400.0), 4.0 * years);
        const Money reducedMaturity = Money::fromRupees(reducedMaturityValue);

        Account *acc = accountManager.findAccount(fd->linkedAccountNo);
        if (acc)
        {
            acc->balance += reducedMaturity;
            accountManager.saveAccounts();
        }

        fd->isClosed = true;
        saveFDRD();

        authManager.logAction("FD_PREMATURE_WITHDRAW", fd->bankId, "Prematurely closed FD #" + fdNo);
        Utils::printWarning("FD #" + fdNo + " closed prematurely with 1% penalty. Credited " + Utils::formatCurrency(static_cast<double>(reducedMaturity)) + " to linked account.");
        return true;
    }

    RecurringDeposit *FDRDManager::createRD(const std::string &customerId, const std::string &bankId, const std::string &linkedAccNo, double monthlyInstallment, int tenureMonths)
    {
        const Money installmentAmount = Money::fromRupees(monthlyInstallment);
        Account *acc = accountManager.findAccount(linkedAccNo);
        if (!acc || acc->balance < installmentAmount)
        {
            Utils::printError("Insufficient balance in linked account for 1st installment.");
            return nullptr;
        }

        BankInfo *b = rbiController.findBank(bankId);
        double rate = b ? b->policies.fd1YrRate : 6.5;

        // RD maturity formula: P * n + P * n(n+1)/2 * (r/1200)
        const Money totalDeposited = installmentAmount * static_cast<double>(tenureMonths);
        const Money interest = installmentAmount * (static_cast<double>(tenureMonths) * (static_cast<double>(tenureMonths) + 1.0) / 2.0) * (rate / 1200.0);
        const Money maturityAmount = totalDeposited + interest;

        acc->balance -= installmentAmount;
        accountManager.saveAccounts();

        RecurringDeposit rd;
        rd.rdNo = Utils::generateRandomId("RD");
        rd.customerId = customerId;
        rd.bankId = bankId;
        rd.linkedAccountNo = linkedAccNo;
        rd.monthlyInstallment = installmentAmount;
        rd.tenureMonths = tenureMonths;
        rd.monthsPaid = 1;
        rd.interestRate = rate;
        rd.maturityAmount = maturityAmount;
        rd.startDate = Utils::getCurrentDate();
        rd.isClosed = false;

        recurringDeposits.push_back(rd);
        saveFDRD();

        Utils::printSuccess("Recurring Deposit Opened! RD No: " + rd.rdNo + " | Maturity Amount: " + Utils::formatCurrency(static_cast<double>(rd.maturityAmount)));
        return &recurringDeposits.back();
    }

    bool FDRDManager::payRDMonthlyInstallment(const std::string &rdNo)
    {
        for (auto &rd : recurringDeposits)
        {
            if (rd.rdNo == rdNo && !rd.isClosed)
            {
                Account *acc = accountManager.findAccount(rd.linkedAccountNo);
                if (!acc || acc->balance < rd.monthlyInstallment)
                {
                    Utils::printError("Insufficient balance to pay RD installment.");
                    return false;
                }
                acc->balance -= rd.monthlyInstallment;
                rd.monthsPaid++;
                if (rd.monthsPaid >= rd.tenureMonths)
                {
                    rd.isClosed = true;
                    acc->balance += rd.maturityAmount;
                    Utils::printSuccess("🎉 RD #" + rdNo + " MATURED! Total Payout of " + Utils::formatCurrency(static_cast<double>(rd.maturityAmount)) + " credited to account!");
                }
                else
                {
                    Utils::printSuccess("Paid installment #" + std::to_string(rd.monthsPaid) + " of " + std::to_string(rd.tenureMonths) + " for RD #" + rdNo);
                }
                saveFDRD();
                accountManager.saveAccounts();
                return true;
            }
        }
        Utils::printError("RD not found or closed.");
        return false;
    }

    std::vector<FixedDeposit> FDRDManager::getCustomerFDs(const std::string &customerId)
    {
        std::vector<FixedDeposit> res;
        for (const auto &f : fixedDeposits)
        {
            if (f.customerId == customerId)
                res.push_back(f);
        }
        return res;
    }

    std::vector<RecurringDeposit> FDRDManager::getCustomerRDs(const std::string &customerId)
    {
        std::vector<RecurringDeposit> res;
        for (const auto &r : recurringDeposits)
        {
            if (r.customerId == customerId)
                res.push_back(r);
        }
        return res;
    }

} // namespace BankingSystem
