#include "insurance.h"
#include "utils.h"
#include <iostream>

namespace BankingSystem {

InsuranceManager::InsuranceManager(Storage& store, AuthManager& auth)
    : storage(store), authManager(auth) {
    loadPolicies();
}

void InsuranceManager::loadPolicies() {
    std::vector<InsurancePolicy> loaded = storage.loadInsurancePolicies();
    policies.assign(loaded.begin(), loaded.end());
}

void InsuranceManager::savePolicies() {
    std::vector<InsurancePolicy> snapshot(policies.begin(), policies.end());
    storage.saveInsurancePolicies(snapshot);
}

double InsuranceManager::calculateAnnualPremium(InsuranceType type, double coverage) {
    double rate = 0.005; // 0.5% default
    switch (type) {
        case InsuranceType::LIFE_TERM: rate = 0.003; break;
        case InsuranceType::LIFE_ENDOWMENT: rate = 0.025; break;
        case InsuranceType::HEALTH_MEDICLAIM: rate = 0.015; break;
        case InsuranceType::HEALTH_CRITICAL: rate = 0.020; break;
        case InsuranceType::VEHICLE_COMPREHENSIVE: rate = 0.030; break;
        case InsuranceType::HOME_LOAN_PROTECT: rate = 0.008; break;
    }
    return coverage * rate;
}

InsurancePolicy* InsuranceManager::buyPolicy(const std::string& customerId, const std::string& bankId, InsuranceType type, double coverage, const std::string& nominee, const std::string& relation) {
    double premium = calculateAnnualPremium(type, coverage);

    InsurancePolicy p;
    p.policyNo = Utils::generateRandomId("INS");
    p.customerId = customerId;
    p.bankId = bankId;
    p.type = type;
    p.coverageAmount = coverage;
    p.annualPremium = premium;
    p.nomineeName = nominee;
    p.nomineeRelation = relation;
    p.startDate = Utils::getCurrentDate();
    p.isActive = true;
    p.claimPending = false;
    p.claimAmountRequested = 0.0;
    p.claimStatus = "NONE";

    policies.push_back(p);
    savePolicies();

    authManager.logAction("INSURANCE_BOUGHT", bankId, "Bought " + insuranceTypeToString(type) + " Policy #" + p.policyNo + " for " + Utils::formatCurrency(coverage));
    Utils::printSuccess("Insurance Policy Purchased! Policy No: " + p.policyNo + " | Annual Premium: " + Utils::formatCurrency(premium));
    return &policies.back();
}

bool InsuranceManager::fileClaim(const std::string& policyNo, double amountRequested) {
    for (auto& p : policies) {
        if (p.policyNo == policyNo && p.isActive) {
            if (amountRequested > p.coverageAmount) {
                Utils::printError("Claim amount cannot exceed policy coverage limit of " + Utils::formatCurrency(p.coverageAmount));
                return false;
            }
            p.claimPending = true;
            p.claimAmountRequested = amountRequested;
            p.claimStatus = "SUBMITTED";
            savePolicies();
            Utils::printSuccess("Insurance Claim submitted for Policy #" + policyNo + ". Pending Branch Manager Approval.");
            return true;
        }
    }
    Utils::printError("Policy not found or inactive.");
    return false;
}

bool InsuranceManager::processClaimBranchManager(const std::string& policyNo, bool approve) {
    if (!authManager.checkPermission(Role::BRANCH_MANAGER)) {
        Utils::printError("Only Branch Manager or higher authority can process insurance claims.");
        return false;
    }

    for (auto& p : policies) {
        if (p.policyNo == policyNo && p.claimPending) {
            p.claimPending = false;
            if (approve) {
                p.claimStatus = "APPROVED";
                Utils::printSuccess("Insurance Claim of " + Utils::formatCurrency(p.claimAmountRequested) + " APPROVED for Policy #" + policyNo);
            } else {
                p.claimStatus = "REJECTED";
                Utils::printWarning("Insurance Claim REJECTED for Policy #" + policyNo);
            }
            savePolicies();
            return true;
        }
    }
    Utils::printError("Pending claim not found.");
    return false;
}

std::vector<InsurancePolicy> InsuranceManager::getCustomerPolicies(const std::string& customerId) {
    std::vector<InsurancePolicy> res;
    for (const auto& p : policies) {
        if (p.customerId == customerId) res.push_back(p);
    }
    return res;
}

std::vector<InsurancePolicy> InsuranceManager::getPendingClaims(const std::string& bankId) {
    std::vector<InsurancePolicy> res;
    for (const auto& p : policies) {
        if (p.bankId == bankId && p.claimPending) res.push_back(p);
    }
    return res;
}

} // namespace BankingSystem
