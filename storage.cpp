#include "storage.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <utility>

namespace BankingSystem
{

    namespace fs = std::filesystem;

    Storage::Storage(std::string dir) : dataDir(dir)
    {
        ensureDirectoryExists();
    }

    void Storage::ensureDirectoryExists()
    {
        if (!fs::exists(dataDir))
        {
            fs::create_directories(dataDir);
        }
    }

    bool Storage::saveCustomers(const std::vector<Customer> &customers)
    {
        const std::string tmpPath = dataDir + "/customers.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &c : customers)
        {
            ofs << c.serialize() << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/customers.dat");
        return true;
    }

    std::vector<Customer> Storage::loadCustomers()
    {
        std::vector<Customer> list;
        std::ifstream ifs(dataDir + "/customers.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty())
            {
                try
                {
                    Customer value = Customer::deserialize(line);
                    if (!value.customerId.empty())
                        list.push_back(std::move(value));
                }
                catch (...)
                {
                }
            }
        }
        return list;
    }

    bool Storage::saveStaff(const std::vector<Staff> &staffList)
    {
        const std::string tmpPath = dataDir + "/staff.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &s : staffList)
        {
            ofs << s.serialize() << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/staff.dat");
        return true;
    }

    std::vector<Staff> Storage::loadStaff()
    {
        std::vector<Staff> list;
        std::ifstream ifs(dataDir + "/staff.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty())
            {
                try
                {
                    Staff value = Staff::deserialize(line);
                    if (!value.staffId.empty())
                        list.push_back(std::move(value));
                }
                catch (...)
                {
                }
            }
        }
        return list;
    }

    bool Storage::saveAccounts(const std::vector<Account> &accounts)
    {
        const std::string tmpPath = dataDir + "/accounts.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &a : accounts)
        {
            ofs << a.serialize() << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/accounts.dat");
        return true;
    }

    std::vector<Account> Storage::loadAccounts()
    {
        std::vector<Account> list;
        std::ifstream ifs(dataDir + "/accounts.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty())
            {
                try
                {
                    Account value = Account::deserialize(line);
                    if (!value.accountNo.empty())
                        list.push_back(std::move(value));
                }
                catch (...)
                {
                }
            }
        }
        return list;
    }

    bool Storage::saveTransactions(const std::vector<Transaction> &txns)
    {
        const std::string tmpPath = dataDir + "/transactions.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &t : txns)
        {
            ofs << t.serialize() << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/transactions.dat");
        return true;
    }

    std::vector<Transaction> Storage::loadTransactions()
    {
        std::vector<Transaction> list;
        std::ifstream ifs(dataDir + "/transactions.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty())
            {
                try
                {
                    Transaction value = Transaction::deserialize(line);
                    if (!value.txnId.empty())
                        list.push_back(std::move(value));
                }
                catch (...)
                {
                }
            }
        }
        return list;
    }

    bool Storage::saveLoans(const std::vector<Loan> &loans)
    {
        const std::string tmpPath = dataDir + "/loans.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &l : loans)
        {
            ofs << l.serialize() << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/loans.dat");
        return true;
    }

    std::vector<Loan> Storage::loadLoans()
    {
        std::vector<Loan> list;
        std::ifstream ifs(dataDir + "/loans.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty())
            {
                try
                {
                    Loan value = Loan::deserialize(line);
                    if (!value.loanId.empty())
                        list.push_back(std::move(value));
                }
                catch (...)
                {
                }
            }
        }
        return list;
    }

    bool Storage::saveFixedDeposits(const std::vector<FixedDeposit> &fds)
    {
        const std::string tmpPath = dataDir + "/fd.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &f : fds)
        {
            ofs << f.serialize() << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/fd.dat");
        return true;
    }

    std::vector<FixedDeposit> Storage::loadFixedDeposits()
    {
        std::vector<FixedDeposit> list;
        std::ifstream ifs(dataDir + "/fd.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty())
            {
                try
                {
                    FixedDeposit value = FixedDeposit::deserialize(line);
                    if (!value.fdNo.empty())
                        list.push_back(std::move(value));
                }
                catch (...)
                {
                }
            }
        }
        return list;
    }

    bool Storage::saveRecurringDeposits(const std::vector<RecurringDeposit> &rds)
    {
        const std::string tmpPath = dataDir + "/rd.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &r : rds)
        {
            ofs << r.serialize() << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/rd.dat");
        return true;
    }

    std::vector<RecurringDeposit> Storage::loadRecurringDeposits()
    {
        std::vector<RecurringDeposit> list;
        std::ifstream ifs(dataDir + "/rd.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty())
            {
                try
                {
                    RecurringDeposit value = RecurringDeposit::deserialize(line);
                    if (!value.rdNo.empty())
                        list.push_back(std::move(value));
                }
                catch (...)
                {
                }
            }
        }
        return list;
    }

    bool Storage::saveInsurancePolicies(const std::vector<InsurancePolicy> &policies)
    {
        const std::string tmpPath = dataDir + "/insurance.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &p : policies)
        {
            ofs << p.serialize() << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/insurance.dat");
        return true;
    }

    std::vector<InsurancePolicy> Storage::loadInsurancePolicies()
    {
        std::vector<InsurancePolicy> list;
        std::ifstream ifs(dataDir + "/insurance.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty())
            {
                try
                {
                    InsurancePolicy value = InsurancePolicy::deserialize(line);
                    if (!value.policyNo.empty())
                        list.push_back(std::move(value));
                }
                catch (...)
                {
                }
            }
        }
        return list;
    }

    bool Storage::saveCreditCards(const std::vector<CreditCard> &cards)
    {
        const std::string tmpPath = dataDir + "/credit_cards.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &c : cards)
        {
            ofs << c.serialize() << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/credit_cards.dat");
        return true;
    }

    std::vector<CreditCard> Storage::loadCreditCards()
    {
        std::vector<CreditCard> list;
        std::ifstream ifs(dataDir + "/credit_cards.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty())
            {
                try
                {
                    CreditCard value = CreditCard::deserialize(line);
                    if (!value.cardNumber.empty())
                        list.push_back(std::move(value));
                }
                catch (...)
                {
                }
            }
        }
        return list;
    }

    bool Storage::saveBanks(const std::vector<BankInfo> &banks)
    {
        const std::string tmpPath = dataDir + "/banks.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &b : banks)
        {
            ofs << b.bankId << "|" << b.name << "|" << b.code << "|" << b.cmdStaffId << "|"
                << b.policies.minSavingsBalance << "|" << b.policies.savingsInterestRate << "|"
                << b.policies.homeLoanBaseRate << "|" << b.policies.personalLoanBaseRate << "|"
                << b.policies.carLoanBaseRate << "|" << b.policies.eduLoanBaseRate << "|"
                << b.policies.fd1YrRate << "|" << b.policies.fd3YrRate << "|" << b.policies.fd5YrRate << "|"
                << b.policies.seniorCitizenBonus << "|" << (b.isLicensed ? 1 : 0) << "|"
                << b.totalDeposits << "|" << b.totalLoansDisbursed << "|" << b.totalNPA << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/banks.dat");
        return true;
    }

    std::vector<BankInfo> Storage::loadBanks()
    {
        std::vector<BankInfo> list;
        std::ifstream ifs(dataDir + "/banks.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (line.empty())
                continue;
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(ss, token, '|'))
                tokens.push_back(token);
            if (tokens.size() >= 18)
            {
                try
                {
                    BankInfo b;
                    b.bankId = tokens[0];
                    b.name = tokens[1];
                    b.code = tokens[2];
                    b.cmdStaffId = tokens[3];
                    b.policies.minSavingsBalance = std::stod(tokens[4]);
                    b.policies.savingsInterestRate = std::stod(tokens[5]);
                    b.policies.homeLoanBaseRate = std::stod(tokens[6]);
                    b.policies.personalLoanBaseRate = std::stod(tokens[7]);
                    b.policies.carLoanBaseRate = std::stod(tokens[8]);
                    b.policies.eduLoanBaseRate = std::stod(tokens[9]);
                    b.policies.fd1YrRate = std::stod(tokens[10]);
                    b.policies.fd3YrRate = std::stod(tokens[11]);
                    b.policies.fd5YrRate = std::stod(tokens[12]);
                    b.policies.seniorCitizenBonus = std::stod(tokens[13]);
                    b.isLicensed = (tokens[14] == "1");
                    b.totalDeposits = std::stod(tokens[15]);
                    b.totalLoansDisbursed = std::stod(tokens[16]);
                    b.totalNPA = std::stod(tokens[17]);
                    list.push_back(b);
                }
                catch (...)
                {
                    // Corrupt records are rejected; a bad line must not abort startup.
                }
            }
        }
        return list;
    }

    bool Storage::saveRBIPolicy(const RBIPolicy &policy)
    {
        const std::string tmpPath = dataDir + "/rbi.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        ofs << policy.repoRate << "|" << policy.reverseRepoRate << "|" << policy.crr << "|"
            << policy.slr << "|" << policy.lastUpdatedDate << "\n";
        for (const auto &c : policy.circulars)
        {
            ofs << "CIRCULAR|" << c << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/rbi.dat");
        return true;
    }

    RBIPolicy Storage::loadRBIPolicy()
    {
        RBIPolicy policy;
        std::ifstream ifs(dataDir + "/rbi.dat");
        if (!ifs)
            return policy;
        std::string line;
        if (std::getline(ifs, line))
        {
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(ss, token, '|'))
                tokens.push_back(token);
            if (tokens.size() >= 5)
            {
                try
                {
                    policy.repoRate = std::stod(tokens[0]);
                    policy.reverseRepoRate = std::stod(tokens[1]);
                    policy.crr = std::stod(tokens[2]);
                    policy.slr = std::stod(tokens[3]);
                    policy.lastUpdatedDate = tokens[4];
                }
                catch (...)
                {
                    return RBIPolicy{};
                }
            }
        }
        while (std::getline(ifs, line))
        {
            if (line.rfind("CIRCULAR|", 0) == 0)
            {
                policy.circulars.push_back(line.substr(9));
            }
        }
        return policy;
    }

    bool Storage::saveAuditLogs(const std::vector<AuditLog> &logs)
    {
        const std::string tmpPath = dataDir + "/audit.dat.tmp";
        std::ofstream ofs(tmpPath, std::ios::trunc);
        if (!ofs)
            return false;
        for (const auto &l : logs)
        {
            ofs << l.serialize() << "\n";
        }
        ofs.close();
        if (!ofs)
            return false;
        std::filesystem::rename(tmpPath, dataDir + "/audit.dat");
        return true;
    }

    std::vector<AuditLog> Storage::loadAuditLogs()
    {
        std::vector<AuditLog> list;
        std::ifstream ifs(dataDir + "/audit.dat");
        if (!ifs)
            return list;
        std::string line;
        while (std::getline(ifs, line))
        {
            if (!line.empty())
            {
                try
                {
                    AuditLog value = AuditLog::deserialize(line);
                    if (!value.logId.empty())
                        list.push_back(std::move(value));
                }
                catch (...)
                {
                }
            }
        }
        return list;
    }

} // namespace BankingSystem
