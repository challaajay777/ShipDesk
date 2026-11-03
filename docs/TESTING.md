# Testing Contract & Verification Report

ShipDesk adheres to strict verification principles: **No invented test results, no fake metrics, no claims of untested platform support.**

## Test Suite Execution Summary (v0.1.0)
- **Engine**: QtTest & CTest
- **Toolchain Tested**: GCC 16.2.0 (UCRT64 MinGW-w64), Qt 6.11.2, CMake 4.4.4, SQLite 3.53.4, Windows 11 x64
- **Total Tests Executed**: 20
- **Total Tests Passed**: 20 (100%)
- **Failures / Skipped**: 0

## Test Cases Covered

1. `testReleaseStateTransitions`: Verifies valid state progression through Draft -> Inspecting -> Inspected -> Validating -> Validated -> Staging -> Staged -> Packaging -> Packaged -> Verifying -> Verified -> ReadyToPublish -> Publishing -> Published.
2. `testInvalidStateTransitions`: Proves invalid transitions throw `ReleaseTransitionException` (e.g. attempting to publish unverified artifacts or modify published releases).
3. `testTransientStateRecovery`: Verifies transient states (`Staging`, `Packaging`, `Publishing`) recover to `Interrupted` rather than false success.
4. `testManifestSerialization`: Tests JSON serialization and deserialization against the `shipdesk-release-manifest` schema.
5. `testPathNormalization`: Tests path separators, collapsing redundant slashes, and stripping leading/trailing slashes.
6. `testPathContainmentAndTraversal`: Tests component-aware path containment, preventing parent traversal (`..`) and root escapes.
7. `testGlobMatching`: Validates glob wildcard matching (`*`, `?`) for file inclusions and exclusions.
8. `testHasherSha256`: Tests known SHA-256 test vectors for empty strings and file streaming.
9. `testSecretRedaction`: Verifies redaction of GitHub PATs, bearer tokens, passwords, and AWS keys.
10. `testDatabaseCrudAndIntegrity`: Verifies SQLite in-memory initialization, foreign keys, migrations, and `PRAGMA integrity_check`.
11. `testZipArchiveCreateAndVerify`: Tests full archive creation and verification against expected checksums and file counts.
12. `testZipArchiveIntegrityMismatch`: Proves verification fails when content does not match expected hashes.
13. `testZipArchiveSafeExtraction`: Tests safe uncompressing ensuring files remain contained within target root.
14. `testValidatorEmptyPackageBlocker`: Proves empty file selection generates a blocking validation error.
15. `testValidatorDuplicatePathBlocker`: Proves duplicate destination paths are blocked.
16. `testValidatorCaseCollisionWarning`: Detects case-only collisions for Windows/macOS file systems.
17. `testVerticalSliceEndToEnd`: Tests complete end-to-end integration: Workspace -> Inspection -> Validation -> Staging -> Packaging -> Manifest -> Cryptographic Verification -> Immutable Record Persistence.
18. `testAcceptanceMatrixScenarios`: Validates scenarios from Section 27 matrix (empty-input validation, zero-byte file handling, unicode paths, paths with spaces, and crash recovery).
