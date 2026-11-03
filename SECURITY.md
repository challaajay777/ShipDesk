# Security Policy

## Threat Model & Security Architecture

ShipDesk is designed around strict defensive engineering boundaries to protect user source files, credentials, and package consumers.

### 1. Source Read-Only Boundary
- Source inspection and staging treat user source directories as strictly read-only.
- Original files are never modified, renamed, moved, or deleted during inspection, staging, or packaging.
- Staging copies files to an isolated, owned staging directory rather than moving or linking them.

### 2. Path Traversal & Archive Safeguards
- Component-aware path canonicalization (`PathUtils::isContainedWithin`) prevents path traversal attacks (`..`).
- Relative paths are normalized using strictly forward slashes; leading slashes, drive letters, and parent traversal components are rejected.
- Archive extraction and verification enforce strict bounds on maximum entry counts, uncompressed byte sizes, and decompression ratios.
- Extraction verifies that every target path remains contained within the owned destination root.

### 3. Untrusted Command Execution & Secret Redaction
- ShipDesk never executes build scripts automatically upon opening a project folder or workspace.
- In Advanced Mode, commands must be reviewed and approved based on an exact fingerprint bound to the executable, argument array, working directory, and environment overrides.
- Sensitive environment variables, personal access tokens (`gho_`, `ghp_`, `github_pat_`, `glpat-`), bearer tokens, and passwords in command outputs are automatically redacted from display logs and database event records.

### 4. Credential Handling
- ShipDesk does not store user passwords, private keys, or GitHub personal access tokens in its local SQLite database.
- GitHub operations use the authenticated session of the official GitHub CLI (`gh`) without prompting for or inspecting tokens.

## Reporting a Vulnerability

If you discover a security vulnerability in ShipDesk, please report it privately:
- **Email**: challaajay777@gmail.com
- Do not open public GitHub issues for security vulnerabilities until a fix has been prepared and released.
- You will receive a response within 48 hours.
