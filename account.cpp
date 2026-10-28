#include "account.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace BankingSystem
{

    AccountManager::AccountManager(Storage &store, AuthManager &auth)
        : storage(store), authManager(auth)
    {
        accounts.reserve(256);
        transactions.reserve(1024);
        loadAccounts();
    }

    void AccountManager::loadAccounts()
    {
        accounts = storage.loadAccounts();
        transactions = storage.loadTransactions();
        accounts.reserve(std::max<size_t>(accounts.capacity(), 256));
        transactions.reserve(std::max<size_t>(transactions.capacity(), 1024));
    }

    void AccountManager::saveAccounts()
    {
        storage.saveAccounts(accounts);
        storage.saveTransactions(transactions);
    }

    Account *AccountManager::openAccount(const std::string &customerId, const std::string &bankId, const std::string &branchCode, AccountType type, double initialDeposit)
    {
        Customer *cust = authManager.findCustomer(customerId);
        if (!cust)
        {
            Utils::printError("Customer ID not found.");
            return nullptr;
        }
        if (!cust->isKycVerified)
        {
            Utils::printError("KYC verification required before opening an account.");
            return nullptr;
        }

        const Money depositAmount = Money::fromRupees(initialDeposit);

        Account acc;
        acc.accountNo = Utils::generateRandomId("ACC");
        acc.customerId = customerId;
        acc.bankId = bankId;
        acc.branchCode = branchCode;
        acc.type = type;
        acc.balance = depositAmount;
        acc.overdraftLimit = (type == AccountType::CURRENT) ? Money::fromRupees(50000.0) : Money{};
        acc.isActive = true;
        acc.hasDebitCard = false;
        acc.hasChequeBook = false;
        acc.createdDate = Utils::getCurrentDate();

        accounts.push_back(acc);

        // Initial deposit transaction log
        if (depositAmount > Money{})
        {
            Transaction t;
            t.txnId = Utils::generateRandomId("TXN");
            t.timestamp = Utils::getCurrentTimestamp();
            t.accountNo = acc.accountNo;
            t.txnType = "INITIAL_DEPOSIT";
            t.amount = depositAmount;
            t.balanceAfter = acc.balance;
            t.narration = "Account Opening Deposit";
            t.processedByStaffId = authManager.getSession().userId;
            transactions.push_back(t);
        }

        saveAccounts();
        authManager.logAction("ACCOUNT_OPENED", bankId, "Opened " + accountTypeToString(type) + " #" + acc.accountNo + " for " + cust->name);
        Utils::printSuccess("Account successfully created! Account Number: " + acc.accountNo);
        return &accounts.back();
    }

    bool AccountManager::deposit(const std::string &accountNo, double amount, const std::string &processedBy)
    {
        const Money depositAmount = Money::fromRupees(amount);
        if (depositAmount <= Money{})
        {
            Utils::printError("Deposit amount must be positive.");
            return false;
        }
        Account *acc = findAccount(accountNo);
        if (!acc || !acc->isActive)
        {
            Utils::printError("Account not found or inactive.");
            return false;
        }

        acc->balance += depositAmount;

        Transaction t;
        t.txnId = Utils::generateRandomId("TXN");
        t.timestamp = Utils::getCurrentTimestamp();
        t.accountNo = accountNo;
        t.txnType = "DEPOSIT";
        t.amount = depositAmount;
        t.balanceAfter = acc->balance;
        t.narration = "Cash Deposit";
        t.processedByStaffId = processedBy;
        transactions.push_back(t);

        saveAccounts();
        authManager.logAction("CASH_DEPOSIT", acc->bankId, "Deposited " + Utils::formatCurrency(amount) + " to Acc #" + accountNo);
        Utils::printSuccess("Deposited " + Utils::formatCurrency(amount) + ". New Balance: " + Utils::formatCurrency(static_cast<double>(acc->balance)));
        return true;
    }

    bool AccountManager::withdraw(const std::string &accountNo, double amount, const std::string &processedBy)
    {
        const Money withdrawalAmount = Money::fromRupees(amount);
        if (withdrawalAmount <= Money{})
        {
            Utils::printError("Withdrawal amount must be positive.");
            return false;
        }
        Account *acc = findAccount(accountNo);
        if (!acc || !acc->isActive)
        {
            Utils::printError("Account not found or inactive.");
            return false;
        }

        const Money maxAvailable = acc->balance + acc->overdraftLimit;
        if (withdrawalAmount > maxAvailable)
        {
            Utils::printError("Insufficient balance / overdraft limit exceeded.");
            return false;
        }

        acc->balance -= withdrawalAmount;

        Transaction t;
        t.txnId = Utils::generateRandomId("TXN");
        t.timestamp = Utils::getCurrentTimestamp();
        t.accountNo = accountNo;
        t.txnType = "WITHDRAW";
        t.amount = withdrawalAmount;
        t.balanceAfter = acc->balance;
        t.narration = "Cash Withdrawal";
        t.processedByStaffId = processedBy;
        transactions.push_back(t);

        saveAccounts();
        authManager.logAction("CASH_WITHDRAWAL", acc->bankId, "Withdrew " + Utils::formatCurrency(amount) + " from Acc #" + accountNo);
        Utils::printSuccess("Withdrew " + Utils::formatCurrency(amount) + ". Remaining Balance: " + Utils::formatCurrency(static_cast<double>(acc->balance)));
        return true;
    }

    bool AccountManager::transfer(const std::string &fromAccNo, const std::string &toAccNo, double amount, const std::string &processedBy)
    {
        const Money transferAmount = Money::fromRupees(amount);
        if (transferAmount <= Money{})
        {
            Utils::printError("Transfer amount must be positive.");
            return false;
        }
        Account *src = findAccount(fromAccNo);
        Account *dest = findAccount(toAccNo);

        if (!src || !src->isActive)
        {
            Utils::printError("Source account not found or inactive.");
            return false;
        }
        if (!dest || !dest->isActive)
        {
            Utils::printError("Destination account not found or inactive.");
            return false;
        }
        if (src == dest)
        {
            Utils::printError("Source and destination account cannot be the same.");
            return false;
        }

        const Money maxAvailable = src->balance + src->overdraftLimit;
        if (transferAmount > maxAvailable)
        {
            Utils::printError("Transfer failed: Insufficient funds.");
            return false;
        }

        // High value transfer OTP simulation
        if (amount > 50000.0)
        {
            std::string otp = authManager.generateOTP();
            std::cout << Utils::YELLOW << "[SECURITY] High-value transfer detected (>Rs. 50,000). OTP sent to registered mobile: " << otp << Utils::RESET << "\n";
            std::string inputOtp = Utils::getInput("Enter OTP to authorize transfer");
            if (!authManager.verifyOTP(inputOtp, otp))
            {
                Utils::printError("OTP Verification Failed! Transfer cancelled.");
                return false;
            }
        }

        src->balance -= transferAmount;
        dest->balance += transferAmount;

        // Outgoing transaction
        Transaction t1;
        t1.txnId = Utils::generateRandomId("TXN");
        t1.timestamp = Utils::getCurrentTimestamp();
        t1.accountNo = fromAccNo;
        t1.txnType = "TRANSFER_OUT";
        t1.amount = transferAmount;
        t1.balanceAfter = src->balance;
        t1.narration = "Fund Transfer to Acc #" + toAccNo;
        t1.processedByStaffId = processedBy;
        transactions.push_back(t1);

        // Incoming transaction
        Transaction t2;
        t2.txnId = Utils::generateRandomId("TXN");
        t2.timestamp = Utils::getCurrentTimestamp();
        t2.accountNo = toAccNo;
        t2.txnType = "TRANSFER_IN";
        t2.amount = transferAmount;
        t2.balanceAfter = dest->balance;
        t2.narration = "Fund Transfer from Acc #" + fromAccNo;
        t2.processedByStaffId = processedBy;
        transactions.push_back(t2);

        saveAccounts();
        authManager.logAction("FUND_TRANSFER", src->bankId, "Transferred " + Utils::formatCurrency(amount) + " from " + fromAccNo + " to " + toAccNo);
        Utils::printSuccess("Successfully transferred " + Utils::formatCurrency(amount) + " to Acc #" + toAccNo);
        return true;
    }

    bool AccountManager::applyDebitCard(const std::string &accountNo)
    {
        Account *acc = findAccount(accountNo);
        if (!acc)
            return false;
        acc->hasDebitCard = true;
        saveAccounts();
        Utils::printSuccess("Debit Card issued for Account #" + accountNo);
        return true;
    }

    bool AccountManager::applyChequeBook(const std::string &accountNo)
    {
        Account *acc = findAccount(accountNo);
        if (!acc)
            return false;
        acc->hasChequeBook = true;
        saveAccounts();
        Utils::printSuccess("25-Leaf Cheque Book issued for Account #" + accountNo);
        return true;
    }

    void AccountManager::displayStatement(const std::string &accountNo)
    {
        Account *acc = findAccount(accountNo);
        if (!acc)
        {
            Utils::printError("Account not found.");
            return;
        }

        Utils::printHeader("ACCOUNT STATEMENT — ACC #" + accountNo);
        std::cout << " Account Type : " << accountTypeToString(acc->type) << "\n";
        std::cout << " Current Balance: " << Utils::formatCurrency(static_cast<double>(acc->balance)) << "\n";
        std::cout << " Opened Date  : " << acc->createdDate << "\n\n";

        std::cout << Utils::BOLD << std::left
                  << std::setw(12) << "Txn ID"
                  << std::setw(20) << "Timestamp"
                  << std::setw(15) << "Type"
                  << std::setw(14) << "Amount"
                  << std::setw(14) << "Balance After"
                  << std::setw(20) << "Narration" << Utils::RESET << "\n";
        Utils::printDivider('-', 95);

        for (const auto &t : transactions)
        {
            if (t.accountNo == accountNo)
            {
                std::cout << std::left
                          << std::setw(12) << t.txnId
                          << std::setw(20) << t.timestamp
                          << std::setw(15) << t.txnType
                          << std::setw(14) << Utils::formatCurrency(static_cast<double>(t.amount))
                          << std::setw(14) << Utils::formatCurrency(static_cast<double>(t.balanceAfter))
                          << std::setw(20) << t.narration << "\n";
            }
        }
        Utils::printDivider('=', 95);
    }

    Account *AccountManager::findAccount(const std::string &accountNo)
    {
        for (auto &a : accounts)
        {
            if (a.accountNo == accountNo)
                return &a;
        }
        return nullptr;
    }

    std::vector<Account> &AccountManager::getCustomerAccounts(const std::string &customerId)
    {
        customerAccountsView.clear();
        for (const auto &a : accounts)
        {
            if (a.customerId == customerId)
                customerAccountsView.push_back(a);
        }
        return customerAccountsView;
    }

} // namespace BankingSystem
