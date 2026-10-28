# E2E Test Infra: RBI Banking Management System

## Test Philosophy
- Opaque-box, requirement-driven E2E testing using C++ test harness or automated script runner.
- Zero dependency on internal implementation structures — tests exercise the system CLI interface and verify outputs/persistence files.
- Methodology: Category-Partition + Boundary Value Analysis + Pairwise Combinatorial + Real-World Workload Testing.

## Feature Inventory & Test Coverage Blueprint
| # | Feature Area | Requirement Source | Tier 1 (Feature) | Tier 2 (Boundary) | Tier 3 (Pairwise) | Tier 4 (Real-World) |
|---|--------------|-------------------|:----------------:|:-----------------:|:-----------------:|:------------------:|
| 1 | RBI Policy & Rate Cascading | R1 (Repo, CRR, SLR) | 5 | 5 | ✓ | ✓ |
| 2 | Multi-Bank Comparison Module | R1 (SBI, HDFC...) | 5 | 5 | ✓ | ✓ |
| 3 | 8-Tier Authorization Model | R2 (Tiers 0..7) | 5 | 5 | ✓ | ✓ |
| 4 | Multi-Level Loan Approval | R2 (<25L, 25L-1Cr, >1Cr) | 5 | 5 | ✓ | ✓ |
| 5 | Audit Logging Infrastructure | R2 (Logs & timestamps) | 5 | 5 | ✓ | ✓ |
| 6 | Core Accounts & Transactions | R3 (Savings, Current, NRI, Joint) | 5 | 5 | ✓ | ✓ |
| 7 | Term Deposits (FD/RD & TDS) | R3 (Senior perks, TDS, penalty) | 5 | 5 | ✓ | ✓ |
| 8 | Credit Cards Engine | R3 (Tiers, CIBIL, finance charges) | 5 | 5 | ✓ | ✓ |
| 9 | Insurance & Claim Workflow | R3 (Life/Health/Vehicle, Claims) | 5 | 5 | ✓ | ✓ |
| 10 | File Persistence & Pre-Seeding | R4 (.dat/.csv save & restore) | 5 | 5 | ✓ | ✓ |

## Coverage Targets
- Tier 1: 50 test cases (5 per feature area)
- Tier 2: 50 test cases (5 per feature area)
- Tier 3: 10 cross-feature pairwise interaction test cases
- Tier 4: 5 comprehensive real-world banking workflow scenarios
- Total Target: 115 test cases
