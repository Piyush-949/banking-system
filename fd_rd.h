#ifndef FD_RD_H
#define FD_RD_H

#include "structures.h"
#include "storage.h"
#include "auth.h"
#include "rbi.h"
#include "account.h"
#include <vector>

namespace BankingSystem {

class FDRDManager {
private:
    Storage& storage;
    AuthManager& authManager;
    RBIController& rbiController;
    AccountManager& accountManager;
    std::vector<FixedDeposit> fixedDeposits;
    std::vector<RecurringDeposit> recurringDeposits;

public:
    FDRDManager(Storage& store, AuthManager& auth, RBIController& rbi, AccountManager& acc);

    void loadFDRD();
    void saveFDRD();

    // Fixed Deposit operations
    FixedDeposit* createFD(const std::string& customerId, const std::string& bankId, const std::string& linkedAccNo, double principal, int tenureMonths);
    bool prematureWithdrawFD(const std::string& fdNo);

    // Recurring Deposit operations
    RecurringDeposit* createRD(const std::string& customerId, const std::string& bankId, const std::string& linkedAccNo, double monthlyInstallment, int tenureMonths);
    bool payRDMonthlyInstallment(const std::string& rdNo);

    std::vector<FixedDeposit> getCustomerFDs(const std::string& customerId);
    std::vector<RecurringDeposit> getCustomerRDs(const std::string& customerId);
};

} // namespace BankingSystem

#endif // FD_RD_H
