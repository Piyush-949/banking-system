# Changelog

## 2026-08-09

- Added the complete pre-refactor audit and baseline test record.
- Initialized legacy persisted domain fields to deterministic defaults.
- Preserved trailing empty persistence fields and skipped malformed legacy records safely.
- Hardened modular `Money`, modular audit/user serialization, and audit secret redaction.
- Removed CVV output and newly persisted CVV values from the legacy card flow.
- Stabilized account-vector capacity and corrected a temporary-view lifetime hazard exposed by the tests.

