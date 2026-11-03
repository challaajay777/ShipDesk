# ShipDesk Release Manifest Schema

Every deliverable package includes a standardized release manifest adhering to schema `shipdesk-release-manifest` v1.0.0.

## Schema Example

```json
{
  "format": "shipdesk-release-manifest",
  "schema_version": "1.0.0",
  "product_name": "DevTools-Suite",
  "workspace_id": "8f452495-2e9b-4c81-a2b8-a996927e05aa",
  "release_id": "rel-001",
  "version": "1.0.0",
  "channel": "Stable",
  "created_at_utc": "2026-11-03T10:00:00Z",
  "platform": "Windows-x64",
  "source_snapshot": {},
  "profile_snapshot": {
    "naming_template": "{product}-{version}",
    "default_format": "ZIP"
  },
  "validation_summary": {
    "findings_count": 0,
    "blockers_count": 0
  },
  "artifacts": [
    {
      "artifact_id": "art-001",
      "filename": "DevTools-Suite-v1.0.0.zip",
      "output_path": "C:/ShipDesk_Releases/DevTools-Suite-v1.0.0.zip",
      "size_bytes": 1048576,
      "sha256": "3a7bd3e2360a3d29eea436fcfb7e44c735d117c42d1c1835420b6b9942dd4f1b",
      "package_format": "ZIP",
      "created_at_utc": "2026-11-03T10:00:00Z"
    }
  ],
  "files": [
    {
      "relative_path": "bin/tool.exe",
      "size_bytes": 524288,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
      "is_executable": true,
      "source_relative_path": "bin/tool.exe",
      "classification": "regular",
      "inclusion_reason": "profile_rule_or_user_selection"
    }
  ]
}
```

## Field Specifications
- **`format`**: Constant identifier `"shipdesk-release-manifest"`.
- **`schema_version`**: Semantic version of the manifest schema (currently `"1.0.0"`).
- **`product_name`**: Human-readable name of the packaged product.
- **`version`**: Semantic version or user-specified version string.
- **`channel`**: Distribution channel (`Draft`, `Preview`, `Alpha`, `Beta`, `Stable`, `Archived`).
- **`created_at_utc`**: ISO 8601 UTC timestamp.
- **`platform`**: Operating system and target architecture (`Windows-x64`, `Linux-x64`).
- **`artifacts`**: Array of packaged deliverables generated for this release.
- **`files`**: Comprehensive inventory of all packaged files with relative paths, byte counts, and SHA-256 digests.
