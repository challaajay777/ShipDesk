# ShipDesk Product Specification

## 1. Vision & Core Promise
"Prepare, check, package, verify, and deliver digital work without losing control of originals."

ShipDesk is a native C++20 desktop application designed for individuals, educators, creators, developers, and teams who need a reliable, local-first release workflow. It eliminates manual errors, accidental source mutations, missing dependencies, and unverified distributions.

## 2. Target Audience
1. **Everyday Users**: Packaging files, folders, and archives for colleagues or clients.
2. **Students & Educators**: Assembling assignments, project packages, and course assets.
3. **Designers & Media Creators**: Bundling creative assets, audio, video, and design deliverables.
4. **Game & Mod Creators**: Packaging mods, resource packs, and game builds with manifest verification.
5. **Software Developers**: Producing tested, hashed binaries and source releases.
6. **Open Source Maintainers**: Generating verifiable release artifacts and automated GitHub releases.

## 3. Core Principles
- **Guided by Default; Advanced When Needed**: Intuitive 8-step wizard for immediate packaging; deep workspace controls for developers.
- **Local-First & Offline**: All staging, hashing, packaging, and record-keeping happen offline on the local CPU and drive.
- **Strict Read-Only Source**: Staging copies files; original sources are never mutated, moved, or deleted.
- **No Implicit Execution**: Untrusted build scripts or external executables are never run automatically.
- **Cryptographic Trust**: SHA-256 hashes establish bitwise consistency against recorded release manifests.
- **Zero Telemetry**: No analytics, background uploads, or tracking.
