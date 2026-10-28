#ifndef STORAGE_H
#define STORAGE_H

#include "structures.h"
#include <vector>
#include <map>
#include <string>

namespace BankingSystem {

class Storage {
private:
    std::string dataDir;

public:
    explicit Storage(std::string dir = "data");

    void ensureDirectoryExists();

    // Customers
    bool saveCustomers(const std::vector<Customer>& customers);
    std::vector<Customer> loadCustomers();

    // Staff
    bool saveStaff(const std::vector<Staff>& staffList);
    std::vector<Staff> loadStaff();

    // Accounts
    bool saveAccounts(const std::vector<Account>& accounts);
    std::vector<Account> loadAccounts();

    // Transactions
    bool saveTransactions(const std::vector<Transaction>& txns);
    std::vector<Transaction> loadTransactions();

    // Loans
    bool saveLoans(const std::vector<Loan>& loans);
    std::vector<Loan> loadLoans();

    // Fixed Deposits
    bool saveFixedDeposits(const std::vector<FixedDeposit>& fds);
    std::vector<FixedDeposit> loadFixedDeposits();

    // Recurring Deposits
    bool saveRecurringDeposits(const std::vector<RecurringDeposit>& rds);
    std::vector<RecurringDeposit> loadRecurringDeposits();

    // Insurance Policies
    bool saveInsurancePolicies(const std::vector<InsurancePolicy>& policies);
    std::vector<InsurancePolicy> loadInsurancePolicies();

    // Credit Cards
    bool saveCreditCards(const std::vector<CreditCard>& cards);
    std::vector<CreditCard> loadCreditCards();

    // Banks Info
    bool saveBanks(const std::vector<BankInfo>& banks);
    std::vector<BankInfo> loadBanks();

    // RBI Policy
    bool saveRBIPolicy(const RBIPolicy& policy);
    RBIPolicy loadRBIPolicy();

    // Audit Logs
    bool saveAuditLogs(const std::vector<AuditLog>& logs);
    std::vector<AuditLog> loadAuditLogs();
};

} // namespace BankingSystem

#endif // STORAGE_H
