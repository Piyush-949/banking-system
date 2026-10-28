#ifndef RBI_H
#define RBI_H

#include "structures.h"
#include "storage.h"
#include "auth.h"
#include <vector>
#include <map>

namespace BankingSystem {

class RBIController {
private:
    RBIPolicy rbiPolicy;
    std::vector<BankInfo> banks;
    Storage& storage;
    AuthManager& authManager;

public:
    RBIController(Storage& store, AuthManager& auth);

    void loadData();
    void saveData();

    // Monetary policy actions
    void setRepoRate(double rate);
    void setReverseRepoRate(double rate);
    void setCRR(double percent);
    void setSLR(double percent);
    void broadcastCircular(const std::string& message);

    // Bank licensing & control
    bool registerBank(const std::string& name, const std::string& code, const std::string& cmdStaffId, bool overrideAuth = false);
    bool revokeBankLicense(const std::string& bankId);
    
    // Inspections & Audits
    void runSystemicAudit();
    void displayMonetaryPolicy();
    void displayAllBanksMetrics();
    void displayCirculars();

    const RBIPolicy& getPolicy() const { return rbiPolicy; }
    const std::vector<BankInfo>& getBanks() const { return banks; }
    BankInfo* findBank(const std::string& bankId);
    BankInfo* findBankByCode(const std::string& code);
};

} // namespace BankingSystem

#endif // RBI_H
