# ShipDesk Package Formats

ShipDesk delivers two first-class package outputs built directly from private staging:

## 1. ZIP Archive (.zip)
- **Standard Compatibility**: Utilizes standard Deflate compression (level 6) conforming to RFC 1950 and RFC 1951.
- **Embedded Manifest**: Contains `shipdesk-release-manifest.json` at the root of the archive.
- **Sidecar Manifest**: Generates an adjacent `<package_name>.manifest.json` alongside the `.zip` for easy external inspection without unpacking.
- **ZIP64 Ready**: Supports large file payloads exceeding 4 GB.
- **Atomic Placement**: Writes to a temporary `.tmp.zip` file, finalizes, and renames to the target filename to prevent partial archive reads.
- **Security Protections**: Rejects archive entries with `..`, leading slashes, drive letters, or escaping characters.

## 2. Organized Directory Package
- **Clean Structure**: Preserves exact relative directory layouts within a dedicated target folder.
- **Embedded Manifest**: Places `shipdesk-release-manifest.json` directly within the directory root.
- **Ideal For**: Local software staging, website output directories, mod folders, and business document deliverables.
