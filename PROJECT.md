# Project: Reserve Bank of India (RBI) Banking Management System

## Architecture
- **Language & Standard**: Standard C++17 (`g++ -std=c++17`), strictly zero third-party external dependencies.
- **Pattern**: Layered domain-driven design with Object-Oriented polymorphism, Singleton patterns (`RBICentralBank`, `AuditLogger`), and Observer pattern (`IRateObserver`) for rate cascading.
- **Monetary Representation**: still under upgradation.
- **Folder Structure**:
  - `include/`: Standard C++ header files (`.hpp` / `.h`).
  - `src/`: Standard C++ implementation files (`.cpp`).
  - `data/`: Custom `.dat` / `.csv` file persistence directory.

## Code Layout
```
rbi_banking_system/
├── include/
│   ├── common/
│   │   ├── Types.hpp
│   │   ├── Money.hpp
│   │   ├── DateUtils.hpp
│   │   └── TableFormatter.hpp
│   ├── audit/
│   │   └── AuditLogger.hpp
│   ├── auth/
│   │   ├── RoleTier.hpp
│   │   ├── User.hpp
│   │   ├── PermissionManager.hpp
│   │   └── UserSession.hpp
│   ├── central/
│   │   ├── RBICentralBank.hpp
│   │   └── IRateObserver.hpp
│   ├── bank/
│   │   ├── ICommercialBank.hpp
│   │   ├── CommercialBankImpl.hpp
│   │   ├── BankRegistry.hpp
│   │   └── BankComparisonEngine.hpp
│   ├── products/
│   │   ├── Account.hpp
│   │   ├── SavingsAccount.hpp
│   │   ├── CurrentAccount.hpp
│   │   ├── NRIAccount.hpp
│   │   ├── JointAccount.hpp
│   │   ├── TermDeposit.hpp
│   │   ├── CreditCard.hpp
│   │   └── InsurancePolicy.hpp
│   ├── loan/
│   │   ├── LoanApplication.hpp
│   │   └── LoanEngine.hpp
│   ├── persistence/
│   │   ├── PersistenceEngine.hpp
│   │   └── DataSeeder.hpp
│   └── ui/
│       └── ConsoleUI.hpp
├── src/
│   ├── common/
│   ├── audit/
│   ├── auth/
│   ├── central/
│   ├── bank/
│   ├── products/
│   ├── loan/
│   ├── persistence/
│   ├── ui/
│   └── main.cpp
```

## Feature Inventory
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | RBI Central Singleton | Manages Repo Rate, Reverse Repo Rate, CRR, SLR with Governor authority | M2 | Survey |
| 2 | Rate Cascading Engine | Repo rate changes automatically propagate to commercial banks | M2 | Survey |
| 3 | Commercial Bank Registration | Registers 5 banks (SBI, HDFC, ICICI, Axis, PNB) with custom spreads | M2 | Survey |
| 4 | Bank Comparison Module | Public side-by-side comparison of rates, perks, FD yields, loan ROI, insurance | M2 | Survey |
| 5 | 8-Tier Authority Model | Enforces Tiers 0 (Governor) to 7 (Customer) with permission checks | M1, M4 | Survey |
| 6 | Loan Escalation Queue | Escalates <25L (BM), 25L-1Cr (ZM), >1Cr (CMD) through approval steps | M4 | Survey |
| 7 | Audit Logger Infrastructure | Records timestamps, actor ID, role, action, and details to memory & file | M1 | Survey |
| 8 | Core Accounts Suite | Savings, Current (overdraft), NRI (NRE/NRO), Joint (EoS/FoS/Jointly) | M3 | Survey |
| 9 | Financial Transactions | Deposit, Withdraw, NEFT, IMPS, balance checks, mini-statement ring buffer | M3 | Survey |
| 10 | Term Deposits (FD/RD) | Quarterly compounding, senior (+0.50%) perks, Section 194A TDS, premature penalty | M3 | Survey |
| 11 | Credit Card System | Tiers (Classic..Super Premium), CIBIL scoring, limits, statements, daily APR charges | M3 | Survey |
| 12 | Insurance Products | Life, Health, Vehicle policy premiums & claim disbursal workflow | M3 | Survey |
| 13 | File Persistence Engine | Pipe-delimited `.dat`/`.csv` serialization with atomic `.tmp` swap writes | M5 | Survey |
| 14 | Auto Pre-Seeding | Pre-seeds RBI policy, 5 banks, staff hierarchy, sample accounts & ₹50L loan on clean start | M5 | Survey |
| 15 | Interactive Console Application | Main entry point (`main.cpp`) with role menus, live operations, auto-save | M5 | Survey |
| 16 | E2E Test Suite & Hardening | 100% pass of Tiers 1-4 tests + Tier 5 white-box adversarial stress testing | M6 | Survey |

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Core Framework, Types & Audit Logger | Common types, Money (Paise), DateUtils, 8-Tier Role Definitions, AuditLogger Singleton | None | PLANNED |
| M2 | RBI Policy Singleton & Commercial Bank Cascading | RBICentralBank singleton, IRateObserver rate cascading, 5 Commercial Banks, BankComparisonEngine | M1 | PLANNED |
| M3 | Banking Products Engine | Accounts (Savings, Current, NRI, Joint), FD/RD with TDS & Senior perks, Credit Cards, Insurance | M1, M2 | PLANNED |
| M4 | 8-Tier Role Authority & Multi-Level Loan Escalation | User Sessions, Permission checks, LoanApplication queue (<25L BM, 25L-1Cr ZM, >1Cr CMD), role workflows | M1, M2, M3 | PLANNED |
| M5 | Persistence Engine, Data Seeder & Main Console UI | Serialization to `.dat`/`.csv`, DataSeeder, Console UI menus, main.cpp entry point | M1, M2, M3, M4 | PLANNED |
| M6 | Final E2E Suite Validation & Adversarial Hardening | Validate 100% E2E test suite pass (Tiers 1-4) & Adversarial Coverage Hardening (Tier 5) | M1..M5, TEST_READY | PLANNED |

## Interface Contracts

### RBICentralBank ↔ Commercial Bank Observers
- `RBICentralBank::registerObserver(std::shared_ptr<IRateObserver> bank)`
- `IRateObserver::onRepoRateChanged(double newRepoRate, double delta)`
- Rate Update Cascade: Repo Rate change automatically updates bank base lending rate (RLLR/MCLR) and interest rate schedules.

### UserSession ↔ PermissionManager
- `PermissionManager::hasPermission(RoleTier tier, Permission perm) -> bool`
- Access controls:
  - RBI Policy changes: Tier 0 (RBI Governor) only.
  - CMD Approval (>₹1Cr loans): Tier 1 only.
  - Zonal Manager Approval (₹25L–₹1Cr loans): Tier 2 or higher.
  - Branch Manager Approval (<₹25L loans): Tier 3 or higher.

### Accounts & Loans ↔ AuditLogger
- `AuditLogger::logAction(const std::string& actorId, RoleTier role, const std::string& action, const std::string& targetId, const std::string& details)`
- Every financial transaction, rate update, loan status escalation, or claim approval MUST invoke `AuditLogger`.
