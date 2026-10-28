#ifndef AUTH_H
#define AUTH_H

#include "structures.h"
#include "storage.h"
#include <string>
#include <vector>
#include <optional>

namespace BankingSystem {

struct Session {
    bool isLoggedIn = false;
    std::string userId;
    std::string userName;
    Role role = Role::PUBLIC_GUEST;
    std::string bankId;
    std::string branchCode;
};

class AuthManager {
private:
    Session currentSession;
    Storage& storage;
    std::vector<Staff> staffList;
    std::vector<Customer> customerList;
    std::vector<AuditLog> auditLogs;

public:
    explicit AuthManager(Storage& store);

    void reloadUsers();

    // Authentication methods
    bool loginStaff(const std::string& staffId, const std::string& rawPassword);
    bool loginCustomer(const std::string& customerId, const std::string& rawPassword);
    void logout();

    bool registerCustomer(const Customer& newCustomer);
    bool addStaffMember(const Staff& newStaff);

    const Session& getSession() const { return currentSession; }
    bool checkPermission(Role requiredRole) const;

    // Security helpers
    bool verifyOTP(const std::string& inputOtp, const std::string& generatedOtp);
    std::string generateOTP();
    void logAction(const std::string& action, const std::string& bankId, const std::string& details);

    const std::vector<Customer>& getCustomers() const { return customerList; }
    const std::vector<Staff>& getStaffList() const { return staffList; }
    const std::vector<AuditLog>& getAuditLogs() const { return auditLogs; }
    Customer* findCustomer(const std::string& customerId);
    Staff* findStaff(const std::string& staffId);
};

} // namespace BankingSystem

#endif // AUTH_H
