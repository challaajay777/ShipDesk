# Build Execution Security & Untrusted Command Safeguards

ShipDesk incorporates strict security boundaries to prevent malicious projects from executing arbitrary code or exfiltrating credentials.

## 1. Zero Automatic Script Execution
- ShipDesk **never executes project scripts automatically** when opening a folder or workspace.
- Detection of CMake, Make, Cargo, or Ninja profiles only records toolchain availability and does not run commands.

## 2. Command Approval & Fingerprinting
Before any external build command is launched:
1. The user is presented with an exact preview dialog containing:
   - Target executable
   - Explicit argument array (never concatenated via shell)
   - Working directory
   - Environment variable overrides
2. ShipDesk generates a cryptographic SHA-256 approval fingerprint:
   `SHA256(SRC_ID + EXE + ARGS + WORKING_DIR + ENV)`
3. The approval is bound strictly to that exact tuple. If any argument, directory, or environment variable changes, the approval is instantly invalidated.

## 3. Secret Pattern Redaction
`ProcessRunner` runs stdout and stderr through an automated regex redaction pipeline before logs are displayed or written to the SQLite event log:
- **GitHub Tokens**: `gho_[A-Za-z0-9_]{16,}`, `ghp_[A-Za-z0-9_]{16,}`, `github_pat_[A-Za-z0-9_]{22,}`
- **GitLab Tokens**: `glpat-[A-Za-z0-9_\-]{20,}`
- **Bearer Tokens**: `Bearer [A-Za-z0-9\-_.~+/=]{16,}`
- **AWS Secret Keys**: `aws_secret_access_key = [A-Za-z0-9/+=]{40}`
- **Credential URLs**: `https://user:password@host` -> `https://user:[REDACTED_PASSWORD]@host`
- **Private Keys**: `-----BEGIN [A-Z ]+ PRIVATE KEY----- ...`

## 4. Resource Limits & Process Cancellation
- Commands execute with enforced timeouts (default: 300 seconds).
- Buffer output is bounded (default: 2 MB) to prevent out-of-memory denial-of-service from infinite log loops.
- Asynchronous termination and kill semantics ensure child processes can be cancelled cleanly by the user.
