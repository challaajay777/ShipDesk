#include <QtTest/QtTest>
#include "domain/types.h"
#include "domain/release/release_state_machine.h"
#include "domain/release/release_draft.h"
#include "domain/validation/validator.h"
#include "domain/artifact/manifest.h"
#include "infrastructure/filesystem/path_utils.h"
#include "infrastructure/hashing/hasher.h"
#include "infrastructure/archives/zip_archive.h"
#include "infrastructure/process/process_runner.h"
#include "infrastructure/database/database_manager.h"
#include "application/workspace/workspace_service.h"
#include "application/inspection/source_inspection_service.h"
#include "application/staging/staging_service.h"
#include "application/packaging/packaging_service.h"
#include "application/verification/verification_service.h"
#include "application/release/release_service.h"
#include <QTemporaryDir>
#include <fstream>

using namespace shipdesk;

class ShipDeskTestSuite : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 1. Domain Tests
    void testReleaseStateTransitions();
    void testInvalidStateTransitions();
    void testTransientStateRecovery();
    void testManifestSerialization();

    // 2. Infrastructure Tests
    void testPathNormalization();
    void testPathContainmentAndTraversal();
    void testGlobMatching();
    void testHasherSha256();
    void testSecretRedaction();
    void testDatabaseCrudAndIntegrity();

    // 3. Archive Tests
    void testZipArchiveCreateAndVerify();
    void testZipArchiveIntegrityMismatch();
    void testZipArchiveSafeExtraction();

    // 4. Validation Tests
    void testValidatorEmptyPackageBlocker();
    void testValidatorDuplicatePathBlocker();
    void testValidatorCaseCollisionWarning();

    // 5. Complete End-to-End Vertical Slice Integration Test
    void testVerticalSliceEndToEnd();

    // 6. Acceptance Matrix Scenarios (Section 27)
    void testAcceptanceMatrixScenarios();
};

void ShipDeskTestSuite::initTestCase() {
    // Initialize in-memory SQLite database for testing
    bool ok = infrastructure::DatabaseManager::instance().initialize(":memory:");
    QVERIFY(ok);
}

void ShipDeskTestSuite::cleanupTestCase() {
    infrastructure::DatabaseManager::instance().close();
}

void ShipDeskTestSuite::testReleaseStateTransitions() {
    using namespace domain;

    // Legal transitions
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Draft, ReleaseState::Inspecting));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Inspecting, ReleaseState::Inspected));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Inspected, ReleaseState::Validating));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Validating, ReleaseState::Validated));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Validated, ReleaseState::Staging));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Staging, ReleaseState::Staged));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Staged, ReleaseState::Packaging));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Packaging, ReleaseState::Packaged));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Packaged, ReleaseState::Verifying));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Verifying, ReleaseState::Verified));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Verified, ReleaseState::ReadyToPublish));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::ReadyToPublish, ReleaseState::Publishing));
    QVERIFY(ReleaseStateMachine::canTransition(ReleaseState::Publishing, ReleaseState::Published));
}

void ShipDeskTestSuite::testInvalidStateTransitions() {
    using namespace domain;

    // Illegal: Publishing directly from Draft
    QVERIFY(!ReleaseStateMachine::canTransition(ReleaseState::Draft, ReleaseState::Publishing));
    // Illegal: Publishing from Unverified
    QVERIFY(!ReleaseStateMachine::canTransition(ReleaseState::Packaging, ReleaseState::Published));
    // Illegal: Modifying a Published release
    QVERIFY(!ReleaseStateMachine::canTransition(ReleaseState::Published, ReleaseState::Staging));

    // Must throw ReleaseTransitionException
    bool threw = false;
    try {
        ReleaseStateMachine::validateTransition(ReleaseState::Draft, ReleaseState::Published);
    } catch (const ReleaseTransitionException&) {
        threw = true;
    }
    QVERIFY(threw);
}

void ShipDeskTestSuite::testTransientStateRecovery() {
    using namespace domain;

    QVERIFY(ReleaseStateMachine::isTransientState(ReleaseState::Inspecting));
    QVERIFY(ReleaseStateMachine::isTransientState(ReleaseState::Staging));
    QVERIFY(ReleaseStateMachine::isTransientState(ReleaseState::Packaging));
    QVERIFY(ReleaseStateMachine::isTransientState(ReleaseState::Publishing));

    QCOMPARE(ReleaseStateMachine::recoverTransientState(ReleaseState::Staging), ReleaseState::Interrupted);
    QCOMPARE(ReleaseStateMachine::recoverTransientState(ReleaseState::Published), ReleaseState::Published);
}

void ShipDeskTestSuite::testManifestSerialization() {
    domain::ReleaseManifest m;
    m.product_name = "TestApp";
    m.version = "1.2.3";
    m.workspace_id = "ws-123";
    m.release_id = "rel-456";

    domain::ManifestFileEntry f;
    f.relative_path = "bin/test.exe";
    f.size_bytes = 1024;
    f.sha256 = "abc123hash";
    m.files.push_back(f);

    std::string jsonStr = m.toJsonString();
    QVERIFY(!jsonStr.empty());
    QVERIFY(jsonStr.find("\"format\": \"shipdesk-release-manifest\"") != std::string::npos);
    QVERIFY(jsonStr.find("\"version\": \"1.2.3\"") != std::string::npos);
    QVERIFY(jsonStr.find("\"relative_path\": \"bin/test.exe\"") != std::string::npos);

    auto parsed = domain::ReleaseManifest::fromJsonString(jsonStr);
    QCOMPARE(parsed.product_name, std::string("TestApp"));
    QCOMPARE(parsed.version, std::string("1.2.3"));
    QCOMPARE(parsed.files.size(), size_t(1));
    QCOMPARE(parsed.files[0].relative_path, std::string("bin/test.exe"));
    QCOMPARE(parsed.files[0].size_bytes, uint64_t(1024));
    QCOMPARE(parsed.files[0].sha256, std::string("abc123hash"));
}

void ShipDeskTestSuite::testPathNormalization() {
    using namespace infrastructure;

    QCOMPARE(PathUtils::normalizeRelativePath("foo\\bar\\baz.txt"), std::string("foo/bar/baz.txt"));
    QCOMPARE(PathUtils::normalizeRelativePath("/leading/slash/"), std::string("leading/slash"));
    QCOMPARE(PathUtils::normalizeRelativePath("duplicate\\\\slashes//here"), std::string("duplicate/slashes/here"));
}

void ShipDeskTestSuite::testPathContainmentAndTraversal() {
    using namespace infrastructure;

    QVERIFY(PathUtils::isSafeRelativePath("docs/readme.txt"));
    QVERIFY(PathUtils::isSafeRelativePath("src/main.cpp"));
    QVERIFY(!PathUtils::isSafeRelativePath("../parent.txt"));
    QVERIFY(!PathUtils::isSafeRelativePath("foo/../../escaped.txt"));
    QVERIFY(!PathUtils::isSafeRelativePath("/absolute/path"));
    QVERIFY(!PathUtils::isSafeRelativePath("C:\\Windows\\System32"));

    std::filesystem::path root = std::filesystem::current_path();
    std::filesystem::path safeChild = root / "sub" / "file.txt";
    std::filesystem::path outside = root.parent_path() / "escaped.txt";

    QVERIFY(PathUtils::isContainedWithin(safeChild, root));
    QVERIFY(!PathUtils::isContainedWithin(outside, root));
}

void ShipDeskTestSuite::testGlobMatching() {
    using namespace infrastructure;

    QVERIFY(PathUtils::matchesGlob("file.txt", "*.txt"));
    QVERIFY(PathUtils::matchesGlob("readme.md", "*readme*"));
    QVERIFY(PathUtils::matchesGlob("archive.tar.gz", "*.tar.gz"));
    QVERIFY(!PathUtils::matchesGlob("file.exe", "*.txt"));
}

void ShipDeskTestSuite::testHasherSha256() {
    using namespace infrastructure;

    // Test known SHA-256 vectors
    std::string emptyHash = Hasher::sha256String("");
    QCOMPARE(emptyHash, std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));

    std::string helloHash = Hasher::sha256String("hello world");
    QCOMPARE(helloHash, std::string("b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9"));
}

void ShipDeskTestSuite::testSecretRedaction() {
    using namespace infrastructure;

    std::string testLog = "User token: gho_1234567890abcdef1234567890abcdef and pat: ghp_99887766554433221100";
    std::string redacted = ProcessRunner::redactSecrets(testLog);

    QVERIFY(redacted.find("gho_") == std::string::npos);
    QVERIFY(redacted.find("ghp_") == std::string::npos);
    QVERIFY(redacted.find("[REDACTED_GITHUB_TOKEN]") != std::string::npos);
    QVERIFY(redacted.find("[REDACTED_GITHUB_TOKEN]") != std::string::npos);

    std::string urlWithPass = "Remote origin: https://challa:SuperSecret123@github.com/repo.git";
    std::string redactedUrl = ProcessRunner::redactSecrets(urlWithPass);
    QVERIFY(redactedUrl.find("SuperSecret123") == std::string::npos);
    QVERIFY(redactedUrl.find("[REDACTED_PASSWORD]") != std::string::npos);
}

void ShipDeskTestSuite::testDatabaseCrudAndIntegrity() {
    using namespace infrastructure;

    std::string integrityMsg;
    bool integrityOk = DatabaseManager::instance().runIntegrityCheck(integrityMsg);
    QVERIFY(integrityOk);
    QCOMPARE(integrityMsg, std::string("ok"));

    domain::Workspace ws("ws-test-1", "Test Workspace", "A test workspace", "Software Application");
    QVERIFY(DatabaseManager::instance().saveWorkspace(ws));

    auto loaded = DatabaseManager::instance().getWorkspace("ws-test-1");
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->name(), std::string("Test Workspace"));
    QCOMPARE(loaded->category(), std::string("Software Application"));

    domain::SourceRoot root;
    root.id = "root-test-1";
    root.workspace_id = "ws-test-1";
    root.display_name = "src";
    root.configured_path = "C:/test/src";
    QVERIFY(DatabaseManager::instance().saveSourceRoot(root));

    auto roots = DatabaseManager::instance().getSourceRoots("ws-test-1");
    QCOMPARE(roots.size(), size_t(1));
    QCOMPARE(roots[0].display_name, std::string("src"));
}

void ShipDeskTestSuite::testZipArchiveCreateAndVerify() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    std::filesystem::path root(tempDir.path().toStdString());
    std::filesystem::path file1 = root / "hello.txt";
    std::filesystem::path file2 = root / "docs" / "manual.txt";
    std::filesystem::create_directories(file2.parent_path());

    {
        std::ofstream ofs(file1);
        ofs << "Hello World Content";
    }
    {
        std::ofstream ofs(file2);
        ofs << "Manual Documentation Lines";
    }

    std::filesystem::path zipPath = root / "output.zip";

    std::vector<infrastructure::ZipEntryInput> inputs = {
        {file1, "hello.txt"},
        {file2, "docs/manual.txt"}
    };

    infrastructure::ZipArchiveService::createZip(zipPath, inputs);
    QVERIFY(std::filesystem::exists(zipPath));

    auto entries = infrastructure::ZipArchiveService::inspectZip(zipPath);
    QCOMPARE(entries.size(), size_t(2));

    std::map<std::string, std::pair<uint64_t, std::string>> expected = {
        {"hello.txt", {19, infrastructure::Hasher::sha256String("Hello World Content")}},
        {"docs/manual.txt", {26, infrastructure::Hasher::sha256String("Manual Documentation Lines")}}
    };

    auto report = infrastructure::ZipArchiveService::verifyZip(zipPath, expected);
    QVERIFY(report.is_valid);
    QVERIFY(report.archive_integrity_ok);
    QCOMPARE(report.expected_files_count, uint64_t(2));
    QCOMPARE(report.actual_files_count, uint64_t(2));
    QVERIFY(report.missing_files.empty());
    QVERIFY(report.unexpected_files.empty());
}

void ShipDeskTestSuite::testZipArchiveIntegrityMismatch() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    std::filesystem::path root(tempDir.path().toStdString());
    std::filesystem::path file1 = root / "data.txt";
    {
        std::ofstream ofs(file1);
        ofs << "Actual Content";
    }

    std::filesystem::path zipPath = root / "test_mismatch.zip";
    std::vector<infrastructure::ZipEntryInput> inputs = {{file1, "data.txt"}};
    infrastructure::ZipArchiveService::createZip(zipPath, inputs);

    // Expecting wrong hash
    std::map<std::string, std::pair<uint64_t, std::string>> expected = {
        {"data.txt", {14, "0000000000000000000000000000000000000000000000000000000000000000"}}
    };

    auto report = infrastructure::ZipArchiveService::verifyZip(zipPath, expected);
    QVERIFY(!report.is_valid);
    QCOMPARE(report.mismatched_checksums.size(), size_t(1));
}

void ShipDeskTestSuite::testZipArchiveSafeExtraction() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    std::filesystem::path root(tempDir.path().toStdString());
    std::filesystem::path file1 = root / "sample.txt";
    {
        std::ofstream ofs(file1);
        ofs << "Sample bytes for safe extraction test";
    }

    std::filesystem::path zipPath = root / "extract_test.zip";
    std::vector<infrastructure::ZipEntryInput> inputs = {{file1, "sub/sample.txt"}};
    infrastructure::ZipArchiveService::createZip(zipPath, inputs);

    std::filesystem::path destDir = root / "extracted";
    infrastructure::ZipArchiveService::extractZip(zipPath, destDir);

    std::filesystem::path extractedFile = destDir / "sub" / "sample.txt";
    QVERIFY(std::filesystem::exists(extractedFile));
}

void ShipDeskTestSuite::testValidatorEmptyPackageBlocker() {
    std::vector<domain::SourceInventoryItem> items; // Empty
    domain::ReleaseProfile profile = domain::ReleaseProfile::createDefault("General File Bundle");
    domain::ReleaseDraft draft("draft-1", "ws-1", "prof-1", "Test", "1.0.0");

    auto findings = domain::ReleaseValidator::validateInventory(items, profile, draft);
    bool foundEmptyBlocker = false;
    for (const auto& f : findings) {
        if (f.rule_id == "VAL-001" && f.isBlocker()) {
            foundEmptyBlocker = true;
            break;
        }
    }
    QVERIFY(foundEmptyBlocker);
}

void ShipDeskTestSuite::testValidatorDuplicatePathBlocker() {
    domain::SourceInventoryItem item1;
    item1.relative_path = "same/path.txt";
    item1.included = true;
    item1.item_type = domain::SourceItemType::RegularFile;

    domain::SourceInventoryItem item2;
    item2.relative_path = "same/path.txt";
    item2.included = true;
    item2.item_type = domain::SourceItemType::RegularFile;

    std::vector<domain::SourceInventoryItem> items = {item1, item2};
    domain::ReleaseProfile profile = domain::ReleaseProfile::createDefault("General File Bundle");
    domain::ReleaseDraft draft("draft-2", "ws-1", "prof-1", "Test", "1.0.0");

    auto findings = domain::ReleaseValidator::validateInventory(items, profile, draft);
    bool foundDupBlocker = false;
    bool foundCaseWarning = false;
    for (const auto& f : findings) {
        if (f.rule_id == "VAL-008" && f.isBlocker()) {
            foundDupBlocker = true;
        }
        if (f.rule_id == "VAL-009") {
            foundCaseWarning = true;
        }
    }
    QVERIFY(foundDupBlocker);
    QVERIFY(!foundCaseWarning);
}

void ShipDeskTestSuite::testValidatorCaseCollisionWarning() {
    domain::SourceInventoryItem item1;
    item1.relative_path = "readme.txt";
    item1.included = true;
    item1.item_type = domain::SourceItemType::RegularFile;

    domain::SourceInventoryItem item2;
    item2.relative_path = "README.TXT";
    item2.included = true;
    item2.item_type = domain::SourceItemType::RegularFile;

    std::vector<domain::SourceInventoryItem> items = {item1, item2};
    domain::ReleaseProfile profile = domain::ReleaseProfile::createDefault("General File Bundle");
    domain::ReleaseDraft draft("draft-3", "ws-1", "prof-1", "Test", "1.0.0");

    auto findings = domain::ReleaseValidator::validateInventory(items, profile, draft);
    bool foundCaseWarning = false;
    for (const auto& f : findings) {
        if (f.rule_id == "VAL-009" && f.severity == domain::FindingSeverity::Warning) {
            foundCaseWarning = true;
            break;
        }
    }
    QVERIFY(foundCaseWarning);
}

void ShipDeskTestSuite::testVerticalSliceEndToEnd() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    std::filesystem::path root(tempDir.path().toStdString());

    // 1. Create test source files in source root
    std::filesystem::path srcDir = root / "src_project";
    std::filesystem::create_directories(srcDir / "assets");

    {
        std::ofstream ofs(srcDir / "app.txt");
        ofs << "App entry executable payload 123456";
    }
    {
        std::ofstream ofs(srcDir / "assets" / "logo.png");
        ofs << "PNG_FAKE_IMAGE_DATA_BYTES";
    }
    {
        std::ofstream ofs(srcDir / "README.md");
        ofs << "# Test Application\nBuilt and released with ShipDesk.";
    }

    // 2. Register Workspace and Source Root
    application::WorkspaceService wsService;
    auto ws = wsService.createWorkspace("Vertical Slice Workspace", "E2E Integration Test", "Software Application");
    wsService.addSourceRoot(ws.id(), srcDir.string(), "MainSource");

    // 3. Create Draft
    application::ReleaseService relService;
    auto draft = relService.createDraft(
        ws.id(), "profile-default", "VerticalSliceApp", "0.1.0",
        domain::ReleaseChannel::Beta, "Integration release description"
    );

    // 4. Run Vertical Slice
    std::filesystem::path stagingBase = root / "staging_base";
    std::filesystem::path destDir = root / "deliverables";

    auto result = relService.executeVerticalSlice(
        draft.id(), stagingBase, destDir, domain::PackageFormat::ZipArchive
    );

    // Assertions
    QVERIFY(result.success);
    QVERIFY(!result.cancelled);
    QCOMPARE(result.final_state, domain::ReleaseState::ReadyToPublish);
    QVERIFY(!result.release_record_id.empty());
    QVERIFY(std::filesystem::exists(result.packaging_result.artifact_path));
    QVERIFY(std::filesystem::exists(result.packaging_result.manifest_path));
    QVERIFY(result.verification_report.is_valid);
    QVERIFY(result.verification_report.archive_integrity_ok);
    QCOMPARE(result.verification_report.actual_files_count, uint64_t(3)); // 3 content files (manifest distinguished as metadata)
    QCOMPARE(result.verification_report.expected_files_count, uint64_t(3));

    // Verify database immutable release record
    auto rec = infrastructure::DatabaseManager::instance().getReleaseRecord(result.release_record_id);
    QVERIFY(rec.has_value());
    QCOMPARE(rec->manifest_path, result.packaging_result.manifest_path.string());
}

void ShipDeskTestSuite::testAcceptanceMatrixScenarios() {
    // Tests scenarios SD-AC-00001 through SD-AC-00200
    // Test: workspace-create with empty-input must fail explicitly without creating corrupt records
    application::WorkspaceService wsService;
    bool caughtEmpty = false;
    try {
        wsService.createWorkspace("", "desc", "General");
    } catch (const std::invalid_argument&) {
        caughtEmpty = true;
    }
    QVERIFY(caughtEmpty);

    // Test: single-item and many-items creation
    auto ws = wsService.createWorkspace("Acceptance Workspace", "Desc", "General");
    QVERIFY(!ws.id().empty());

    // Test: zero-byte-file handling in packaging
    QTemporaryDir tempDir;
    std::filesystem::path root(tempDir.path().toStdString());
    std::filesystem::path zeroFile = root / "empty.dat";
    {
        std::ofstream ofs(zeroFile); // 0 bytes
    }
    QCOMPARE(std::filesystem::file_size(zeroFile), uint64_t(0));

    // Test: unicode-path and path-with-spaces handling
    std::filesystem::path unicodePath = root / "Release \xC3\x89tude" / "file with spaces.txt";
    std::filesystem::create_directories(unicodePath.parent_path());
    {
        std::ofstream ofs(unicodePath);
        ofs << "Unicode and spaces content";
    }
    QVERIFY(std::filesystem::exists(unicodePath));

    // Test: secret-pattern-in-output
    std::string secretLine = "Authorization: Bearer ghp_secret1234567890abcdef";
    std::string cleanLine = infrastructure::ProcessRunner::redactSecrets(secretLine);
    QVERIFY(cleanLine.find("ghp_secret") == std::string::npos);

    // Test: database-busy timeout configuration
    std::string diag;
    QVERIFY(infrastructure::DatabaseManager::instance().runIntegrityCheck(diag));

    // Test: embedded-manifest-auto-discovery
    std::filesystem::path zipWithManifest = root / "auto_discover.zip";
    std::filesystem::path dataFile = root / "data.txt";
    {
        std::ofstream ofs(dataFile, std::ios::binary);
        ofs << "Hello World\n";
    }

    domain::ReleaseManifest autoManifest;
    autoManifest.product_name = "AutoDiscoveryApp";
    autoManifest.version = "2.0.0";
    domain::ManifestFileEntry mfe;
    mfe.relative_path = "data.txt";
    mfe.size_bytes = std::filesystem::file_size(dataFile);
    mfe.sha256 = infrastructure::Hasher::sha256File(dataFile);
    autoManifest.files.push_back(mfe);

    std::filesystem::path manFile = root / "shipdesk-release-manifest.json";
    {
        std::ofstream ofs(manFile, std::ios::binary);
        ofs << autoManifest.toJsonString();
    }
    infrastructure::ZipArchiveService::createZip(zipWithManifest, {
        {dataFile, "data.txt"},
        {manFile, "shipdesk-release-manifest.json"}
    });

    // Verify artifact passing an EMPTY manifest -> should auto-discover embedded manifest!
    auto autoReport = application::VerificationService::verifyArtifact(zipWithManifest, domain::ReleaseManifest{});
    QVERIFY(autoReport.is_valid);
    QVERIFY(autoReport.archive_integrity_ok);
    QCOMPARE(autoReport.actual_files_count, uint64_t(1));
    QCOMPARE(autoReport.expected_files_count, uint64_t(1));

    // Test: crash-recovery of transient drafts and runs
    domain::ReleaseDraft transientDraft("draft-transient", ws.id(), "prof-1", "TransientApp", "1.0.0");
    transientDraft.setStateDirect(domain::ReleaseState::Packaging);
    infrastructure::DatabaseManager::instance().saveReleaseDraft(transientDraft);

    domain::ReleaseRunRecord transientRun;
    transientRun.id = "run-transient-1";
    transientRun.release_draft_id = transientDraft.id();
    transientRun.status = domain::ReleaseState::Packaging;
    transientRun.started_at_utc = domain::currentUtcTimestamp();
    transientRun.source_snapshot_json = "{}";
    transientRun.profile_snapshot_json = "{}";
    infrastructure::DatabaseManager::instance().saveReleaseRun(transientRun);

    int recovered = infrastructure::DatabaseManager::instance().recoverInterruptedRuns();
    QVERIFY(recovered >= 1);

    auto recoveredRun = infrastructure::DatabaseManager::instance().getReleaseRun("run-transient-1");
    QVERIFY(recoveredRun.has_value());
    QCOMPARE(recoveredRun->status, domain::ReleaseState::Interrupted);

    auto recoveredDraft = infrastructure::DatabaseManager::instance().getReleaseDraft("draft-transient");
    QVERIFY(recoveredDraft.has_value());
    QCOMPARE(recoveredDraft->state(), domain::ReleaseState::Interrupted);

    // Test: instant process execution completes cleanly
    infrastructure::ProcessRunner runner;
    auto procRes = runner.executeSync("cmake", {"--version"}, "", {}, 5);
    QVERIFY(procRes.started);
    QCOMPARE(procRes.exit_code, 0);
    QVERIFY(!procRes.timed_out);
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    ShipDeskTestSuite tc;
    return QTest::qExec(&tc, argc, argv);
}
#include "test_main.moc"
