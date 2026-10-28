#include "structures.h"
#include "utils.h"
#include "storage.h"
#include "auth.h"
#include "rbi.h"
#include "bank.h"
#include "account.h"
#include "loan.h"
#include "fd_rd.h"
#include "insurance.h"
#include "credit_card.h"
#include "reports.h"

#include <iostream>
#include <vector>
#include <string>

using namespace BankingSystem;

void seedInitialData(Storage& storage, AuthManager& auth, RBIController& rbi) {
    if (!storage.loadBanks().empty()) return; // Already seeded

    Utils::printInfo("Seeding RBI & Commercial Banks initial data...");

    // Seed RBI Governor Staff Account
    Staff rbiGov;
    rbiGov.staffId = "GOV001";
    rbiGov.name = "Sanjay Malhotra (RBI Governor)";
    rbiGov.role = Role::RBI_GOVERNOR;
    rbiGov.bankId = "RBI";
    rbiGov.branchCode = "HQ";
    rbiGov.email = "governor@rbi.org.in";
    rbiGov.passwordHash = Utils::hashPassword("admin123");
    auth.addStaffMember(rbiGov);

    // Login as Governor to allow bank registration during seeding
    auth.loginStaff("GOV001", "admin123");

    // Seed 5 Commercial Banks
    struct BankSeed {
        std::string name;
        std::string code;
        std::string cmdName;
        std::string cmdId;
        double sROI, hROI, pROI, fdRate;
    };

    std::vector<BankSeed> bSeeds = {
        {"State Bank of India", "SBI", "Dinesh Kumar Khara", "SBI_CMD1", 3.50, 8.40, 11.50, 6.80},
        {"HDFC Bank Ltd", "HDFC", "Sashidhar Jagdishan", "HDFC_CMD1", 3.75, 8.60, 10.75, 7.10},
        {"ICICI Bank Ltd", "ICICI", "Sandeep Bakhshi", "ICICI_CMD1", 3.60, 8.50, 11.00, 7.00},
        {"Axis Bank Ltd", "AXIS", "Amitabh Chaudhry", "AXIS_CMD1", 3.50, 8.75, 11.25, 7.20},
        {"Punjab National Bank", "PNB", "Atul Kumar Goel", "PNB_CMD1", 3.25, 8.45, 12.00, 6.90}
    };

    for (const auto& b : bSeeds) {
        // Create CMD staff
        Staff cmd;
        cmd.staffId = b.cmdId;
        cmd.name = b.cmdName;
        cmd.role = Role::BANK_CMD;
        cmd.bankId = b.code;
        cmd.branchCode = "HQ";
        cmd.email = b.code + ".cmd@bank.com";
        cmd.passwordHash = Utils::hashPassword("cmd123");
        auth.addStaffMember(cmd);

        // Register Bank
        rbi.registerBank(b.name, b.code, b.cmdId, true);
        BankInfo* bankObj = rbi.findBankByCode(b.code);
        if (bankObj) {
            bankObj->policies.savingsInterestRate = b.sROI;
            bankObj->policies.homeLoanBaseRate = b.hROI;
            bankObj->policies.personalLoanBaseRate = b.pROI;
            bankObj->policies.fd1YrRate = b.fdRate;
        }

        // Seed Zonal Head
        Staff zm;
        zm.staffId = b.code + "_ZM1";
        zm.name = b.code + " Zonal Manager";
        zm.role = Role::ZONAL_MANAGER;
        zm.bankId = b.code;
        zm.branchCode = "NORTH_ZONE";
        zm.email = b.code + ".zm@bank.com";
        zm.passwordHash = Utils::hashPassword("zm123");
        auth.addStaffMember(zm);

        // Seed Branch Manager
        Staff bm;
        bm.staffId = b.code + "_BM1";
        bm.name = b.code + " Branch Manager";
        bm.role = Role::BRANCH_MANAGER;
        bm.bankId = b.code;
        bm.branchCode = "BR001";
        bm.email = b.code + ".bm@bank.com";
        bm.passwordHash = Utils::hashPassword("bm123");
        auth.addStaffMember(bm);

        // Seed Loan Officer
        Staff lo;
        lo.staffId = b.code + "_LO1";
        lo.name = b.code + " Senior Loan Officer";
        lo.role = Role::LOAN_OFFICER;
        lo.bankId = b.code;
        lo.branchCode = "BR001";
        lo.email = b.code + ".lo@bank.com";
        lo.passwordHash = Utils::hashPassword("lo123");
        auth.addStaffMember(lo);
    }
    rbi.saveData();

    // Seed Sample Customers
    Customer c1;
    c1.customerId = "CUST101";
    c1.name = "Piyush Kumar";
    c1.dob = "1998-05-14";
    c1.pan = "ABCDE1234F";
    c1.aadhaar = "998877665544";
    c1.address = "Connaught Place, New Delhi";
    c1.phone = "9876543210";
    c1.email = "piyush@example.com";
    c1.creditScore = 780;
    c1.isKycVerified = true;
    c1.isSeniorCitizen = false;
    c1.passwordHash = Utils::hashPassword("user123");
    c1.bankId = "SBI";
    auth.registerCustomer(c1);

    Customer c2;
    c2.customerId = "CUST102";
    c2.name = "Ramesh Sharma (Senior Citizen)";
    c2.dob = "1955-08-20";
    c2.pan = "XYZPS9876K";
    c2.aadhaar = "112233445566";
    c2.address = "Bandra West, Mumbai";
    c2.phone = "9123456789";
    c2.email = "ramesh@example.com";
    c2.creditScore = 810;
    c2.isKycVerified = true;
    c2.isSeniorCitizen = true;
    c2.passwordHash = Utils::hashPassword("user123");
    c2.bankId = "HDFC";
    auth.registerCustomer(c2);

    auth.logout();
    Utils::printSuccess("Pre-seeded 5 Commercial Banks & Staff Credentials!");
}

void showRBIGovernorMenu(RBIController& rbi, AuthManager& auth) {
    while (true) {
        Utils::printHeader("RBI GOVERNOR CENTRAL CONTROL PANELS");
        std::cout << " 1. View Monetary Policy Rates (Repo, CRR, SLR)\n"
                  << " 2. Set Repo Rate (Cascades to all Banks)\n"
                  << " 3. Set Reverse Repo Rate\n"
                  << " 4. Set CRR (Cash Reserve Ratio)\n"
                  << " 5. Set SLR (Statutory Liquidity Ratio)\n"
                  << " 6. Issue New Bank License\n"
                  << " 7. Revoke Bank License\n"
                  << " 8. Broadcast RBI Circular to All Banks\n"
                  << " 9. Run Systemic Risk & Bank NPA Audit\n"
                  << " 10. View Audit Trail Logs\n"
                  << " 11. Logout\n";
        int choice = Utils::getIntInput("Select option");

        if (choice == 1) {
            rbi.displayMonetaryPolicy();
        } else if (choice == 2) {
            double rate = Utils::getDoubleInput("Enter New Repo Rate (% p.a.)");
            rbi.setRepoRate(rate);
        } else if (choice == 3) {
            double rate = Utils::getDoubleInput("Enter New Reverse Repo Rate (% p.a.)");
            rbi.setReverseRepoRate(rate);
        } else if (choice == 4) {
            double rate = Utils::getDoubleInput("Enter New CRR (%)");
            rbi.setCRR(rate);
        } else if (choice == 5) {
            double rate = Utils::getDoubleInput("Enter New SLR (%)");
            rbi.setSLR(rate);
        } else if (choice == 6) {
            std::string name = Utils::getInput("Enter Bank Name");
            std::string code = Utils::getInput("Enter Bank Short Code (e.g. KOTAK)");
            std::string cmdId = code + "_CMD1";
            rbi.registerBank(name, code, cmdId);
        } else if (choice == 7) {
            std::string bId = Utils::getInput("Enter Bank ID to revoke license");
            rbi.revokeBankLicense(bId);
        } else if (choice == 8) {
            std::string msg = Utils::getInput("Enter Circular Message");
            rbi.broadcastCircular(msg);
            Utils::printSuccess("Circular broadcasted.");
        } else if (choice == 9) {
            rbi.runSystemicAudit();
        } else if (choice == 10) {
            Utils::printHeader("SYSTEMIC AUDIT LOG TRAIL");
            for (const auto& l : auth.getAuditLogs()) {
                std::cout << " [" << l.timestamp << "] " << roleToString(l.actorRole) << " (" << l.actorId << ") @ " << l.bankId << " : " << l.action << " -> " << l.details << "\n";
            }
        } else if (choice == 11) {
            auth.logout();
            break;
        }
        Utils::pressEnterToContinue();
    }
}

void showBankStaffMenu(AuthManager& auth, RBIController& /*rbi*/, BankEngine& bankEngine, AccountManager& accManager, LoanManager& loanManager, InsuranceManager& insManager, ReportGenerator& reportGen) {
    const Session& session = auth.getSession();
    while (true) {
        Utils::printHeader("BANK PORTAL — " + session.userName + " (" + roleToString(session.role) + " @ " + session.bankId + ")");
        
        std::cout << " 1. Account Operations (Deposit / Withdraw / Transfer)\n"
                  << " 2. View Pending Loan Applications Queue\n"
                  << " 3. Review / Sanction Loan Application\n"
                  << " 4. Disburse Sanctioned Loan\n"
                  << " 5. Process Insurance Claims\n"
                  << " 6. Generate Branch / Bank Report\n";
        if (session.role == Role::BANK_CMD) {
            std::cout << " 7. [CMD ONLY] Configure Bank Interest Rates & Policies\n";
        }
        std::cout << " 8. Logout\n";

        int choice = Utils::getIntInput("Select option");

        if (choice == 1) {
            std::cout << " 1. Deposit Cash\n 2. Withdraw Cash\n 3. Fund Transfer\n";
            int sub = Utils::getIntInput("Select");
            if (sub == 1) {
                std::string accNo = Utils::getInput("Enter Account No");
                double amt = Utils::getDoubleInput("Enter Deposit Amount");
                accManager.deposit(accNo, amt, session.userId);
            } else if (sub == 2) {
                std::string accNo = Utils::getInput("Enter Account No");
                double amt = Utils::getDoubleInput("Enter Withdrawal Amount");
                accManager.withdraw(accNo, amt, session.userId);
            } else if (sub == 3) {
                std::string src = Utils::getInput("Enter Source Account No");
                std::string dest = Utils::getInput("Enter Destination Account No");
                double amt = Utils::getDoubleInput("Enter Transfer Amount");
                accManager.transfer(src, dest, amt, session.userId);
            }
        } else if (choice == 2) {
            auto pending = loanManager.getPendingLoansForRole(session.role, session.bankId);
            Utils::printSubHeader("PENDING LOANS REQUIRING YOUR APPROVAL");
            if (pending.empty()) std::cout << "No pending loans in your queue.\n";
            for (const auto& l : pending) {
                std::cout << " Loan Ref: " << l.loanId << " | Cust: " << l.customerId << " | Type: " << loanTypeToString(l.type)
                          << " | Principal: " << Utils::formatCurrency(l.principalAmount) << " | Status: " << loanStatusToString(l.status) << "\n";
            }
        } else if (choice == 3) {
            std::string loanId = Utils::getInput("Enter Loan Ref ID to review");
            int dec = Utils::getIntInput("Decision (1 = Approve / Escalate, 0 = Reject)");
            std::string comm = Utils::getInput("Enter Comments / Reason");
            bool approve = (dec == 1);

            if (session.role == Role::LOAN_OFFICER) loanManager.reviewLoanOfficer(loanId, approve, comm);
            else if (session.role == Role::BRANCH_MANAGER) loanManager.reviewBranchManager(loanId, approve, comm);
            else if (session.role == Role::ZONAL_MANAGER) loanManager.reviewZonalManager(loanId, approve, comm);
            else if (session.role == Role::BANK_CMD) loanManager.reviewCMD(loanId, approve, comm);
        } else if (choice == 4) {
            std::string loanId = Utils::getInput("Enter Sanctioned Loan ID to disburse");
            loanManager.disburseLoan(loanId);
        } else if (choice == 5) {
            auto pendingClaims = insManager.getPendingClaims(session.bankId);
            Utils::printSubHeader("PENDING INSURANCE CLAIMS");
            for (const auto& p : pendingClaims) {
                std::cout << " Policy #" << p.policyNo << " | Claim Amount: " << Utils::formatCurrency(p.claimAmountRequested) << "\n";
            }
            if (!pendingClaims.empty()) {
                std::string polNo = Utils::getInput("Enter Policy No to process");
                int dec = Utils::getIntInput("Decision (1 = Approve, 0 = Reject)");
                insManager.processClaimBranchManager(polNo, dec == 1);
            }
        } else if (choice == 6) {
            reportGen.generateBankBalanceSheet(session.bankId);
        } else if (choice == 7 && session.role == Role::BANK_CMD) {
            double sROI = Utils::getDoubleInput("Enter Savings ROI (%)");
            double hROI = Utils::getDoubleInput("Enter Home Loan Rate (%)");
            double pROI = Utils::getDoubleInput("Enter Personal Loan Rate (%)");
            double fd1Yr = Utils::getDoubleInput("Enter 1-Yr FD Rate (%)");
            bankEngine.updateBankInterestRates(session.bankId, sROI, hROI, pROI, fd1Yr);
        } else if (choice == 8) {
            auth.logout();
            break;
        }
        Utils::pressEnterToContinue();
    }
}

void showCustomerMenu(AuthManager& auth, RBIController& /*rbi*/, AccountManager& accManager, LoanManager& loanManager, FDRDManager& fdrdManager, InsuranceManager& insManager, CreditCardManager& ccManager, ReportGenerator& reportGen) {
    const Session& session = auth.getSession();
    while (true) {
        Utils::printHeader("CUSTOMER MOBILE & INTERNET BANKING PORTAL — " + session.userName);
        std::cout << " 1. View Complete Portfolio Dashboard\n"
                  << " 2. Account Statement & Transactions\n"
                  << " 3. Fund Transfer to Another Account\n"
                  << " 4. Apply for a New Loan (Home, Personal, Car, Edu)\n"
                  << " 5. Pay Loan EMI\n"
                  << " 6. Open Fixed Deposit (FD) / Recurring Deposit (RD)\n"
                  << " 7. Premature FD Withdrawal\n"
                  << " 8. Buy Insurance Policy (Life, Health, Vehicle)\n"
                  << " 9. Apply / Manage Credit Card\n"
                  << " 10. Swipe Credit Card (Merchant Simulation)\n"
                  << " 11. Pay Credit Card Bill\n"
                  << " 12. Logout\n";

        int choice = Utils::getIntInput("Select option");

        if (choice == 1) {
            reportGen.generateCustomerDashboard(session.userId);
        } else if (choice == 2) {
            auto accs = accManager.getCustomerAccounts(session.userId);
            if (accs.empty()) {
                Utils::printError("No active accounts found.");
            } else {
                for (size_t i = 0; i < accs.size(); ++i) {
                    std::cout << " [" << (i+1) << "] Acc #" << accs[i].accountNo << " (" << accountTypeToString(accs[i].type) << ")\n";
                }
                int idx = Utils::getIntInput("Select Account index");
                if (idx >= 1 && idx <= static_cast<int>(accs.size())) {
                    accManager.displayStatement(accs[idx-1].accountNo);
                }
            }
        } else if (choice == 3) {
            auto accs = accManager.getCustomerAccounts(session.userId);
            if (!accs.empty()) {
                std::string src = accs[0].accountNo;
                std::string dest = Utils::getInput("Enter Recipient Account No");
                double amt = Utils::getDoubleInput("Enter Amount to Transfer");
                accManager.transfer(src, dest, amt, session.userId);
            }
        } else if (choice == 4) {
            std::cout << " Select Loan Type:\n 0. Home\n 1. Personal\n 2. Car\n 3. Education\n 4. Business\n";
            int lType = Utils::getIntInput("Loan Type");
            double principal = Utils::getDoubleInput("Enter Loan Principal Amount (Rs.)");
            int tenure = Utils::getIntInput("Enter Tenure in Months (e.g. 12, 36, 60, 240)");
            
            auto accs = accManager.getCustomerAccounts(session.userId);
            if (!accs.empty()) {
                loanManager.applyForLoan(session.userId, session.bankId, accs[0].accountNo, static_cast<LoanType>(lType), principal, tenure);
            }
        } else if (choice == 5) {
            std::string loanId = Utils::getInput("Enter Loan ID to pay EMI");
            double amt = Utils::getDoubleInput("Enter EMI Amount");
            loanManager.payEMI(loanId, amt);
        } else if (choice == 6) {
            std::cout << " 1. Fixed Deposit (FD)\n 2. Recurring Deposit (RD)\n";
            int sub = Utils::getIntInput("Select");
            auto accs = accManager.getCustomerAccounts(session.userId);
            if (!accs.empty()) {
                if (sub == 1) {
                    double amt = Utils::getDoubleInput("Enter FD Amount");
                    int tenure = Utils::getIntInput("Enter Tenure in Months (12, 36, 60)");
                    fdrdManager.createFD(session.userId, session.bankId, accs[0].accountNo, amt, tenure);
                } else {
                    double monthly = Utils::getDoubleInput("Enter Monthly Installment");
                    int tenure = Utils::getIntInput("Enter Tenure in Months (12, 24, 36)");
                    fdrdManager.createRD(session.userId, session.bankId, accs[0].accountNo, monthly, tenure);
                }
            }
        } else if (choice == 7) {
            std::string fdNo = Utils::getInput("Enter FD No to withdraw prematurely");
            fdrdManager.prematureWithdrawFD(fdNo);
        } else if (choice == 8) {
            std::cout << " Insurance Type: 0. Term Life, 1. Endowment, 2. Mediclaim Health, 4. Vehicle Comprehensive\n";
            int insType = Utils::getIntInput("Select Type");
            double coverage = Utils::getDoubleInput("Enter Coverage Sum Assured (Rs.)");
            std::string nom = Utils::getInput("Enter Nominee Name");
            std::string rel = Utils::getInput("Enter Nominee Relation");
            insManager.buyPolicy(session.userId, session.bankId, static_cast<InsuranceType>(insType), coverage, nom, rel);
        } else if (choice == 9) {
            std::cout << " Credit Card Type: 0. Classic, 1. Gold, 2. Platinum, 3. Super Premium\n";
            int cardType = Utils::getIntInput("Select Card Type");
            auto accs = accManager.getCustomerAccounts(session.userId);
            if (!accs.empty()) {
                ccManager.applyForCreditCard(session.userId, session.bankId, accs[0].accountNo, static_cast<CreditCardType>(cardType));
            }
        } else if (choice == 10) {
            std::string cardNo = Utils::getInput("Enter Credit Card Number");
            double amt = Utils::getDoubleInput("Enter Spend Amount");
            std::string merch = Utils::getInput("Enter Merchant Name (e.g. Amazon, Fuel)");
            ccManager.swipeCard(cardNo, amt, merch);
        } else if (choice == 11) {
            std::string cardNo = Utils::getInput("Enter Credit Card Number");
            double amt = Utils::getDoubleInput("Enter Payment Amount");
            ccManager.payCreditCardBill(cardNo, amt);
        } else if (choice == 12) {
            auth.logout();
            break;
        }
        Utils::pressEnterToContinue();
    }
}

void createNewCustomerFlow(AuthManager& auth, RBIController& rbi, AccountManager& accManager) {
    Utils::printHeader("NEW CUSTOMER ONBOARDING & ACCOUNT OPENING");
    rbi.displayAllBanksMetrics();

    std::string name = Utils::getInput("Enter Full Name");
    std::string dob = Utils::getInput("Enter Date of Birth (YYYY-MM-DD)");
    std::string pan = Utils::getInput("Enter PAN Number (10 chars)");
    std::string aadhaar = Utils::getInput("Enter Aadhaar Number (12 digits)");
    std::string address = Utils::getInput("Enter Address");
    std::string phone = Utils::getInput("Enter Mobile Number");
    std::string email = Utils::getInput("Enter Email Address");
    std::string password = Utils::getInput("Set Portal Password");
    
    std::string bankCode = Utils::getInput("Choose Bank Code (SBI, HDFC, ICICI, AXIS, PNB)");
    BankInfo* b = rbi.findBankByCode(bankCode);
    if (!b) {
        Utils::printError("Invalid Bank Code selected.");
        return;
    }

    Customer c;
    c.customerId = Utils::generateRandomId("CUST");
    c.name = name;
    c.dob = dob;
    c.pan = pan;
    c.aadhaar = aadhaar;
    c.address = address;
    c.phone = phone;
    c.email = email;
    c.creditScore = 750; // Initial default credit score
    c.isKycVerified = true;
    c.isSeniorCitizen = false;
    c.passwordHash = Utils::hashPassword(password);
    c.bankId = b->code;

    if (auth.registerCustomer(c)) {
        Utils::printSuccess("Customer Onboarded! Your Customer ID: " + c.customerId);
        double dep = Utils::getDoubleInput("Enter Initial Account Opening Deposit (Min Rs. 1000)");
        accManager.openAccount(c.customerId, b->code, "BR001", AccountType::SAVINGS, dep);
    } else {
        Utils::printError("Registration failed. Customer with this PAN already exists.");
    }
}

int main() {
    Storage storage("data");
    AuthManager auth(storage);
    RBIController rbi(storage, auth);
    BankEngine bankEngine(storage, auth, rbi);
    AccountManager accountManager(storage, auth);
    LoanManager loanManager(storage, auth, rbi, accountManager);
    FDRDManager fdrdManager(storage, auth, rbi, accountManager);
    InsuranceManager insuranceManager(storage, auth);
    CreditCardManager creditCardManager(storage, auth, accountManager);
    ReportGenerator reportGenerator(storage, auth, rbi, accountManager, loanManager, fdrdManager, insuranceManager, creditCardManager);

    seedInitialData(storage, auth, rbi);

    while (true) {
        Utils::printHeader("NATIONAL BANKING SYSTEM — CENTRAL INTEGRATED NETWORK (RBI)");
        std::cout << Utils::BOLD << Utils::CYAN
                  << "       __/\033[33m[RESERVE BANK OF INDIA]\033[36m\\__       \n"
                  << "      |   Governing Commercial Banks  |      \n"
                  << "      | SBI | HDFC | ICICI | AXIS | PNB |    \n"
                  << Utils::RESET << "\n";

        std::cout << " 1. RBI Central Authority (Governor Login)\n"
                  << " 2. Commercial Bank Portal (Staff Login — CMD/BM/LO/Teller)\n"
                  << " 3. Customer Portal (Internet Banking Login)\n"
                  << " 4. Open New Bank Account (New Customer Onboarding)\n"
                  << " 5. Compare Banks & Products Matrix (Public Portal)\n"
                  << " 6. Exit System\n";

        int choice = Utils::getIntInput("Select option");

        if (choice == 1) {
            std::cout << " Default Governor Staff ID: GOV001 | Password: admin123\n";
            std::string uid = Utils::getInput("Enter Staff ID");
            std::string pwd = Utils::getInput("Enter Password");
            if (auth.loginStaff(uid, pwd) && auth.getSession().role == Role::RBI_GOVERNOR) {
                showRBIGovernorMenu(rbi, auth);
            } else {
                Utils::printError("Invalid RBI Governor Credentials!");
            }
        } else if (choice == 2) {
            std::cout << " Sample Credentials: SBI_CMD1 / cmd123 | SBI_BM1 / bm123 | SBI_LO1 / lo123\n";
            std::string uid = Utils::getInput("Enter Staff ID");
            std::string pwd = Utils::getInput("Enter Password");
            if (auth.loginStaff(uid, pwd)) {
                showBankStaffMenu(auth, rbi, bankEngine, accountManager, loanManager, insuranceManager, reportGenerator);
            } else {
                Utils::printError("Invalid Staff Credentials!");
            }
        } else if (choice == 3) {
            std::cout << " Sample Credentials: CUST101 / user123 (Piyush) | CUST102 / user123 (Ramesh)\n";
            std::string uid = Utils::getInput("Enter Customer ID");
            std::string pwd = Utils::getInput("Enter Password");
            if (auth.loginCustomer(uid, pwd)) {
                showCustomerMenu(auth, rbi, accountManager, loanManager, fdrdManager, insuranceManager, creditCardManager, reportGenerator);
            } else {
                Utils::printError("Invalid Customer Credentials!");
            }
        } else if (choice == 4) {
            createNewCustomerFlow(auth, rbi, accountManager);
        } else if (choice == 5) {
            bankEngine.displayBankComparisonTable();
            rbi.displayMonetaryPolicy();
            rbi.displayCirculars();
        } else if (choice == 6) {
            Utils::printSuccess("Thank you for using National Banking System!");
            break;
        }
        Utils::pressEnterToContinue();
    }

    return 0;
}
