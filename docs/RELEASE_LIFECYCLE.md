# ShipDesk Release Lifecycle & State Machine

Every release run in ShipDesk progresses through an explicit, formal state machine implemented in `domain/release/release_state_machine.h`.

```
  Draft
    │
    ▼
 Inspecting ──────────► Inspected
                          │
                          ▼
                       Validating ────────► Validated
                                              │
                                              ▼
                                           Staging ────────► Staged
                                                              │
                                                              ▼
                                                           Packaging ────────► Packaged
                                                                                │
                                                                                ▼
                                                                             Verifying ────────► Verified
                                                                                                   │
                                                                                                   ▼
                                                                                            ReadyToPublish
                                                                                                   │
                                                                                                   ▼
                                                                                               Publishing
                                                                                                │      │
                                                                                   Published ◄──┘      └──► PublishFailed
```

## State Definitions
- **Draft**: Initial configuration state. Metadata, versioning, and profiles can be edited freely.
- **Inspecting**: Active scanning of source roots off the GUI thread. Original files are strictly read-only.
- **Inspected**: Directory scan completed; inventory items and exclusion reasons generated.
- **Validating**: Rule engine evaluates inventory against profile thresholds.
- **Validated**: Pre-flight checks passed or non-blocking warnings accepted.
- **Staging**: Streaming copy of selected files into an isolated private staging directory with `shipdesk.owner` marker and simultaneous SHA-256 calculation.
- **Staged**: Staged files verified against source metadata without mutation.
- **Packaging**: Construction of target archive (.zip) or structured directory package with embedded manifest.
- **Packaged**: Artifact file successfully written to disk.
- **Verifying**: Deep verification of package structure, CRC32 checksums, and manifest file hashes.
- **Verified**: Artifact passes 100% of cryptographic integrity checks.
- **ReadyToPublish**: Package is verified and staged for local delivery or remote upload.
- **Publishing**: Active file copy to delivery folder or upload via official GitHub CLI.
- **Published**: Final immutable release record persisted.
- **PublishFailed**: Remote upload failed or interrupted; local verified artifact is preserved on disk.
- **Interrupted**: State assigned during startup recovery to any job abandoned due to system power loss or process crash.
- **NeedsReview**: Blocked state requiring user intervention or rule adjustment.
