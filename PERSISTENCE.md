# Persistence

Legacy files remain pipe-delimited for compatibility with the existing CLI and tests. The reader now preserves empty trailing fields and skips malformed records rather than allowing conversion exceptions to terminate startup. New modular audit/user records escape pipes, newlines, carriage returns, and escapes.

The legacy persistence format is not versioned and is not suitable for production records. It must be replaced by a versioned repository format with atomic transaction support before real use. Existing fixture directories are historical test data and were retained.

