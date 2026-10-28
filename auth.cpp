#include "auth.h"
#include "utils.h"
#include <iostream>
#include <random>

namespace BankingSystem
{

    AuthManager::AuthManager(Storage &store) : storage(store)
    {
        reloadUsers();
    }

    void AuthManager::reloadUsers()
    {
        staffList = storage.loadStaff();
        customerList = storage.loadCustomers();
        auditLogs = storage.loadAuditLogs();
    }

    bool AuthManager::loginStaff(const std::string &staffId, const std::string &rawPassword)
    {
        std::string hashed = Utils::hashPassword(rawPassword);
        for (const auto &s : staffList)
        {
            if (s.staffId == staffId && s.passwordHash == hashed)
            {
                currentSession.isLoggedIn = true;
                currentSession.userId = s.staffId;
                currentSession.userName = s.name;
                currentSession.role = s.role;
                currentSession.bankId = s.bankId;
                currentSession.branchCode = s.branchCode;

                logAction("STAFF_LOGIN", s.bankId, "Staff logged in successfully");
                return true;
            }
        }
        return false;
    }

    bool AuthManager::loginCustomer(const std::string &customerId, const std::string &rawPassword)
    {
        std::string hashed = Utils::hashPassword(rawPassword);
        for (const auto &c : customerList)
        {
            if (c.customerId == customerId && c.passwordHash == hashed)
            {
                currentSession.isLoggedIn = true;
                currentSession.userId = c.customerId;
                currentSession.userName = c.name;
                currentSession.role = Role::CUSTOMER;
                currentSession.bankId = c.bankId;
                currentSession.branchCode = "";

                logAction("CUSTOMER_LOGIN", c.bankId, "Customer logged in successfully");
                return true;
            }
        }
        return false;
    }

    void AuthManager::logout()
    {
        if (currentSession.isLoggedIn)
        {
            logAction("LOGOUT", currentSession.bankId, "User logged out");
            currentSession = Session();
        }
    }

    bool AuthManager::registerCustomer(const Customer &newCustomer)
    {
        for (const auto &c : customerList)
        {
            if (c.customerId == newCustomer.customerId)
            {
                return false; // Already exists
            }
            if (!c.pan.empty() && !newCustomer.pan.empty() && c.pan == newCustomer.pan)
            {
                return false; // Duplicate PAN when both are populated
            }
        }
        customerList.push_back(newCustomer);
        storage.saveCustomers(customerList);
        logAction("CUSTOMER_REGISTERED", newCustomer.bankId, "New customer onboarding: " + newCustomer.name);
        return true;
    }

    bool AuthManager::addStaffMember(const Staff &newStaff)
    {
        for (const auto &s : staffList)
        {
            if (s.staffId == newStaff.staffId)
                return false;
        }
        staffList.push_back(newStaff);
        storage.saveStaff(staffList);
        logAction("STAFF_ADDED", newStaff.bankId, "Added staff: " + newStaff.name + " (" + roleToString(newStaff.role) + ")");
        return true;
    }

    bool AuthManager::checkPermission(Role requiredRole) const
    {
        if (!currentSession.isLoggedIn)
            return false;
        // Lower int enum value means higher authority
        return static_cast<int>(currentSession.role) <= static_cast<int>(requiredRole);
    }

    std::string AuthManager::generateOTP()
    {
        static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
        std::uniform_int_distribution<int> dist(100000, 999999);
        return std::to_string(dist(rng));
    }

    bool AuthManager::verifyOTP(const std::string &inputOtp, const std::string &generatedOtp)
    {
        return inputOtp == generatedOtp;
    }

    void AuthManager::logAction(const std::string &action, const std::string &bankId, const std::string &details)
    {
        AuditLog log;
        log.logId = Utils::generateRandomId("LOG");
        log.timestamp = Utils::getCurrentTimestamp();
        log.actorId = currentSession.isLoggedIn ? currentSession.userId : "GUEST";
        log.actorRole = currentSession.role;
        log.action = action;
        log.bankId = bankId;
        log.details = details;

        auditLogs.push_back(log);
        storage.saveAuditLogs(auditLogs);
    }

    Customer *AuthManager::findCustomer(const std::string &customerId)
    {
        for (auto &c : customerList)
        {
            if (c.customerId == customerId)
                return &c;
        }
        return nullptr;
    }

    Staff *AuthManager::findStaff(const std::string &staffId)
    {
        for (auto &s : staffList)
        {
            if (s.staffId == staffId)
                return &s;
        }
        return nullptr;
    }

} // namespace BankingSystem
