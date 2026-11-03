#include "ui/pages/package_verifier_page.h"
#include "application/verification/verification_service.h"
#include "infrastructure/archives/zip_archive.h"
#include "domain/artifact/manifest.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <fstream>

namespace shipdesk::ui {

PackageVerifierPage::PackageVerifierPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void PackageVerifierPage::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    auto* header = new QLabel(
        "<b>Package Verifier Tool</b><br>"
        "<span style='color: #80868B;'>Cryptographically verify artifact integrity, archive structure, and manifest compliance.</span>",
        this
    );
    mainLayout->addWidget(header);

    auto* formLayout = new QFormLayout();

    // Package row
    auto* pkgRow = new QHBoxLayout();
    m_packagePathEdit = new QLineEdit(this);
    m_packagePathEdit->setPlaceholderText("Path to .zip or package folder...");
    m_browsePkgBtn = new QPushButton("Browse...", this);
    connect(m_browsePkgBtn, &QPushButton::clicked, this, &PackageVerifierPage::onBrowsePackageClicked);
    pkgRow->addWidget(m_packagePathEdit, 1);
    pkgRow->addWidget(m_browsePkgBtn);
    formLayout->addRow("Package Artifact:", pkgRow);

    // Manifest row
    auto* manRow = new QHBoxLayout();
    m_manifestPathEdit = new QLineEdit(this);
    m_manifestPathEdit->setPlaceholderText("Optional path to shipdesk-release-manifest.json...");
    m_browseManifestBtn = new QPushButton("Browse...", this);
    connect(m_browseManifestBtn, &QPushButton::clicked, this, &PackageVerifierPage::onBrowseManifestClicked);
    manRow->addWidget(m_manifestPathEdit, 1);
    manRow->addWidget(m_browseManifestBtn);
    formLayout->addRow("Manifest (Optional):", manRow);

    mainLayout->addLayout(formLayout);

    auto* btnRow = new QHBoxLayout();
    m_verifyBtn = new QPushButton("Run Verification", this);
    m_verifyBtn->setDefault(true);
    connect(m_verifyBtn, &QPushButton::clicked, this, &PackageVerifierPage::onVerifyClicked);
    btnRow->addWidget(m_verifyBtn);

    m_statusBadge = new QLabel("Status: Ready", this);
    btnRow->addWidget(m_statusBadge);
    btnRow->addStretch();
    mainLayout->addLayout(btnRow);

    m_reportView = new QTextEdit(this);
    m_reportView->setReadOnly(true);
    m_reportView->setFontFamily("Courier New");
    mainLayout->addWidget(m_reportView, 1);
}

void PackageVerifierPage::onBrowsePackageClicked() {
    QString file = QFileDialog::getOpenFileName(this, "Select Package Archive", "", "ZIP Archives (*.zip);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_packagePathEdit->setText(file);

        // Auto-detect adjacent manifest if exists
        QFileInfo fi(file);
        QString sidecar = fi.dir().filePath(fi.baseName() + ".manifest.json");
        if (QFile::exists(sidecar)) {
            m_manifestPathEdit->setText(sidecar);
        }
    }
}

void PackageVerifierPage::onBrowseManifestClicked() {
    QString file = QFileDialog::getOpenFileName(this, "Select Release Manifest", "", "JSON Files (*.json);;All Files (*.*)");
    if (!file.isEmpty()) {
        m_manifestPathEdit->setText(file);
    }
}

void PackageVerifierPage::onVerifyClicked() {
    QString pkgPath = m_packagePathEdit->text().trimmed();
    if (pkgPath.isEmpty() || !QFile::exists(pkgPath)) {
        QMessageBox::warning(this, "Missing File", "Please select an existing package artifact file.");
        return;
    }

    domain::ReleaseManifest manifest;
    QString manPath = m_manifestPathEdit->text().trimmed();
    if (!manPath.isEmpty() && QFile::exists(manPath)) {
        std::ifstream ifs(manPath.toStdString());
        if (ifs.is_open()) {
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            manifest = domain::ReleaseManifest::fromJsonString(content);
        }
    }

    auto report = application::VerificationService::verifyArtifact(pkgPath.toStdString(), manifest);

    QString details = "=== VERIFICATION REPORT ===\n";
    details += "Artifact: " + QString::fromStdString(report.artifact_path) + "\n";
    details += "Checked At (UTC): " + QString::fromStdString(report.checked_at_utc) + "\n";
    details += "Archive Structure / CRC32: " + QString(report.archive_integrity_ok ? "PASSED" : "FAILED") + "\n";
    details += "Expected File Count: " + QString::number(report.expected_files_count) + "\n";
    details += "Actual File Count: " + QString::number(report.actual_files_count) + "\n\n";

    if (!report.messages.empty()) {
        details += "Messages:\n";
        for (const auto& m : report.messages) details += "  - " + QString::fromStdString(m) + "\n";
        details += "\n";
    }

    if (!report.missing_files.empty()) {
        details += "Missing Files:\n";
        for (const auto& m : report.missing_files) details += "  - " + QString::fromStdString(m) + "\n";
        details += "\n";
    }

    if (!report.unexpected_files.empty()) {
        details += "Unexpected Files:\n";
        for (const auto& u : report.unexpected_files) details += "  - " + QString::fromStdString(u) + "\n";
        details += "\n";
    }

    if (!report.mismatched_checksums.empty()) {
        details += "Checksum / Size Mismatches:\n";
        for (const auto& [f, err] : report.mismatched_checksums) details += "  - " + QString::fromStdString(f) + ": " + QString::fromStdString(err) + "\n";
        details += "\n";
    }

    if (report.is_valid) {
        details += "RESULT: VERIFIED OK. All integrity checks passed.\n";
        m_statusBadge->setText("<span style='color: #10B981; font-weight: bold;'>Status: VERIFIED PASS</span>");
    } else {
        details += "RESULT: VERIFICATION FAILED.\n";
        m_statusBadge->setText("<span style='color: #EF4444; font-weight: bold;'>Status: VERIFICATION FAILED</span>");
    }

    m_reportView->setPlainText(details);
}

} // namespace shipdesk::ui
