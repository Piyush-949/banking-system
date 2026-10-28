#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "structures.h"
#include "storage.h"
#include "auth.h"
#include <vector>
#include <string>

namespace BankingSystem {

class AccountManager {
private:
    Storage& storage;
    AuthManager& authManager;
    std::vector<Account> accounts;
    std::vector<Transaction> transactions;
    std::vector<Account> customerAccountsView;

public:
    AccountManager(Storage& store, AuthManager& auth);

    void loadAccounts();
    void saveAccounts();

    Account* openAccount(const std::string& customerId, const std::string& bankId, const std::string& branchCode, AccountType type, double initialDeposit);
    
    bool deposit(const std::string& accountNo, double amount, const std::string& processedBy);
    bool withdraw(const std::string& accountNo, double amount, const std::string& processedBy);
    bool transfer(const std::string& fromAccNo, const std::string& toAccNo, double amount, const std::string& processedBy);

    bool applyDebitCard(const std::string& accountNo);
    bool applyChequeBook(const std::string& accountNo);

    void displayStatement(const std::string& accountNo);
    Account* findAccount(const std::string& accountNo);
    std::vector<Account>& getCustomerAccounts(const std::string& customerId);
    
    const std::vector<Account>& getAllAccounts() const { return accounts; }
};

} // namespace BankingSystem

#endif // ACCOUNT_H
