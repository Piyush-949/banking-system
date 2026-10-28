# Security

This is a simulation, not a production banking platform. The audit found legacy authentication, authorization, and monetary risks that require a larger migration.

Implemented hardening: modular money parsing/arithmetic checks overflow and malformed input; modular audit and user records use escaped persistence fields; audit details redact password/CVV/OTP content; the legacy card statement no longer prints CVV and new card persistence writes no CVV value; malformed legacy record lines are skipped rather than terminating the process.

Remaining blockers for production use: legacy password storage is not PBKDF2/Argon2id with per-user salt, OTP delivery/session expiry/lockout are absent, legacy manager methods do not uniformly enforce actor ownership, and the primary engine still uses `double` monetary values.

