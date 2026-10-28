# Architecture

The current executable is a C++17 console simulation built from root-level managers (`AuthManager`, `Controller`, `AccountManager`, `LoanManager`, `FDRDManager`, `CreditCardManager`, `InsuranceManager`, `ReportGenerator`) and `Storage`.

The `include/` and `src/` tree contains a separate common/auth/audit foundation. It is tested independently and is not yet the executable’s domain layer. This split is a known migration limitation, not a completed modular architecture.

The intended migration path is to move the root manager interfaces behind services that use the shared fixed-point `Money`, authenticated actor context, repository interfaces, and a clock abstraction. No new business logic should be added to console menus.

