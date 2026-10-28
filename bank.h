#ifndef BANK_H
#define BANK_H

#include "structures.h"
#include "storage.h"
#include "auth.h"
#include "rbi.h"
#include <vector>

namespace BankingSystem {

class BankEngine {
private:
    Storage& storage;
    AuthManager& authManager;
    RBIController& rbiController;

public:
    BankEngine(Storage& store, AuthManager& auth, RBIController& rbi);

    // Rate & Policy updates by Bank CMD
    void updateBankInterestRates(const std::string& bankId, double savingsROI, double homeROI, double personalROI, double fd1Yr);
    
    // Public comparison view
    void displayBankComparisonTable();
    void displayProductBrochure(const std::string& bankId);

    // NPA & compliance
    void recalculateBankTotals(const std::string& bankId);
};

} // namespace BankingSystem

#endif // BANK_H
