#ifndef INSURANCE_H
#define INSURANCE_H

#include "structures.h"
#include "storage.h"
#include "auth.h"
#include <deque>

namespace BankingSystem {

class InsuranceManager {
private:
    Storage& storage;
    AuthManager& authManager;
    std::deque<InsurancePolicy> policies;

public:
    InsuranceManager(Storage& store, AuthManager& auth);

    void loadPolicies();
    void savePolicies();

    InsurancePolicy* buyPolicy(const std::string& customerId, const std::string& bankId, InsuranceType type, double coverage, const std::string& nominee, const std::string& relation);
    bool fileClaim(const std::string& policyNo, double amountRequested);
    bool processClaimBranchManager(const std::string& policyNo, bool approve);

    double calculateAnnualPremium(InsuranceType type, double coverage);

    std::vector<InsurancePolicy> getCustomerPolicies(const std::string& customerId);
    std::vector<InsurancePolicy> getPendingClaims(const std::string& bankId);
};

} // namespace BankingSystem

#endif // INSURANCE_H
