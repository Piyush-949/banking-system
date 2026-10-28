# Threat model

In scope: malformed local persistence, delimiter injection, accidental secret disclosure, authorization bypass, replay/OTP misuse, arithmetic overflow, and concurrent balance updates.

The current hardening reduces malformed-record crashes and modular delimiter injection, but production deployment remains blocked by legacy authentication, authorization, monetary representation, persistence atomicity, and concurrency gaps documented in `SECURITY.md`.

