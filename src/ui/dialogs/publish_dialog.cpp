#include "ui/dialogs/publish_dialog.h"
#include "application/publishing/publishing_service.h"
#include "infrastructure/database/database_manager.h"
#include "adapters/destinations/github_destination.h"
#include "domain/common/json_util.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

namespace shipdesk::ui {

PublishDialog::PublishDialog(const QString& releaseRecordId, QWidget* parent)
    : QDialog(parent)
    , m_releaseRecordId(releaseRecordId.toStdString())
{
    setWindowTitle("Publish Deliverable — Confirmation & Delivery");
    resize(640, 500);
    setupUi();
    loadReleaseData();
}

void PublishDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    auto* header = new QLabel(
        "<b>Publish Deliverable</b><br>"
        "<span style='color: #80868B;'>Deliver the verified package to your designated delivery channel. "
        "Explicit confirmation is required before any file is copied or published.</span>",
        this
    );
    mainLayout->addWidget(header);

    // Destination options
    auto* destBox = new QGroupBox("Destination Channel", this);
    auto* destForm = new QFormLayout(destBox);

    m_destTypeCombo = new QComboBox(destBox);
    m_destTypeCombo->addItems({"Local Directory / Removable Storage", "GitHub Releases (Advanced)"});
    connect(m_destTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PublishDialog::onDestinationTypeChanged);
    destForm->addRow("Target Destination:", m_destTypeCombo);

    // Local folder row
    auto* localRow = new QHBoxLayout();
    m_localFolderEdit = new QLineEdit(destBox);
    m_localFolderEdit->setPlaceholderText("Select delivery folder...");
    m_browseFolderBtn = new QPushButton("Browse...", destBox);
    connect(m_browseFolderBtn, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Delivery Folder");
        if (!dir.isEmpty()) m_localFolderEdit->setText(dir);
    });
    localRow->addWidget(m_localFolderEdit, 1);
    localRow->addWidget(m_browseFolderBtn);
    destForm->addRow("Delivery Folder:", localRow);

    // GitHub rows
    m_ghRepoEdit = new QLineEdit(destBox);
    m_ghRepoEdit->setPlaceholderText("e.g. challaajay777/ShipDesk");
    m_ghTagEdit = new QLineEdit("v0.1.0", destBox);
    m_ghDraftCheck = new QCheckBox("Create as Draft (Recommended)", destBox);
    m_ghDraftCheck->setChecked(true);
    m_ghPrereleaseCheck = new QCheckBox("Mark as Pre-release", destBox);

    destForm->addRow("GitHub Repository:", m_ghRepoEdit);
    destForm->addRow("Release Tag:", m_ghTagEdit);
    destForm->addRow("Options:", m_ghDraftCheck);
    destForm->addRow("", m_ghPrereleaseCheck);

    mainLayout->addWidget(destBox);

    // Preview Pane
    auto* previewBox = new QGroupBox("Pre-Publish Inspection & Verification", this);
    auto* prevLayout = new QVBoxLayout(previewBox);
    m_previewEdit = new QTextEdit(previewBox);
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setFontFamily("Courier New");
    prevLayout->addWidget(m_previewEdit);
    mainLayout->addWidget(previewBox, 1);

    // Confirmation Checkbox (Safety Boundary)
    m_confirmationCheck = new QCheckBox("I have reviewed this deliverable, verified the parameters, and explicitly authorize publication.", this);
    connect(m_confirmationCheck, &QCheckBox::toggled, this, &PublishDialog::onConfirmCheckChanged);
    mainLayout->addWidget(m_confirmationCheck);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    m_resultLabel = new QLabel("", this);
    m_resultLabel->setWordWrap(true);
    mainLayout->addWidget(m_resultLabel);

    // Action buttons
    auto* btnRow = new QHBoxLayout();
    m_cancelBtn = new QPushButton("Cancel", this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_publishBtn = new QPushButton("Publish Deliverable", this);
    m_publishBtn->setDefault(true);
    m_publishBtn->setEnabled(false); // Disabled until confirmation check
    connect(m_publishBtn, &QPushButton::clicked, this, &PublishDialog::onPublishClicked);

    btnRow->addWidget(m_cancelBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_publishBtn);
    mainLayout->addLayout(btnRow);

    onDestinationTypeChanged(0);
}

void PublishDialog::loadReleaseData() {
    auto optRec = infrastructure::DatabaseManager::instance().getReleaseRecord(m_releaseRecordId);
    if (!optRec) return;

    m_releaseTitle = domain::json::extractStringField(optRec->metadata_snapshot_json, "title", "ShipDesk-Release");
    m_releaseVersion = domain::json::extractStringField(optRec->metadata_snapshot_json, "version", "0.1.0");

    m_ghTagEdit->setText("v" + QString::fromStdString(m_releaseVersion));

    // Find artifact
    QFileInfo fi(QString::fromStdString(optRec->manifest_path));
    QDir dir = fi.absoluteDir();
    QStringList zips = dir.entryList({"*.zip"}, QDir::Files);
    if (!zips.isEmpty()) {
        m_artifactPath = dir.absoluteFilePath(zips.first()).toStdString();
    } else {
        m_artifactPath = dir.absolutePath().toStdString();
    }

    QString preview = "Deliverable: " + QString::fromStdString(m_releaseTitle) + "\n";
    preview += "Version: " + QString::fromStdString(m_releaseVersion) + "\n";
    preview += "Artifact File: " + QString::fromStdString(m_artifactPath) + "\n";
    preview += "Local Manifest: " + QString::fromStdString(optRec->manifest_path) + "\n";

    m_previewEdit->setPlainText(preview);
}

void PublishDialog::onDestinationTypeChanged(int index) {
    bool isLocal = (index == 0);
    m_localFolderEdit->setVisible(isLocal);
    m_browseFolderBtn->setVisible(isLocal);
    m_ghRepoEdit->setVisible(!isLocal);
    m_ghTagEdit->setVisible(!isLocal);
    m_ghDraftCheck->setVisible(!isLocal);
    m_ghPrereleaseCheck->setVisible(!isLocal);
}

void PublishDialog::onConfirmCheckChanged(bool checked) {
    m_publishBtn->setEnabled(checked);
}

void PublishDialog::onPublishClicked() {
    m_publishBtn->setEnabled(false);
    m_confirmationCheck->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Indeterminate
    m_resultLabel->setText("Publishing deliverable...");
    QCoreApplication::processEvents();

    bool isLocal = (m_destTypeCombo->currentIndex() == 0);
    application::PublishResult res;

    if (isLocal) {
        std::string targetDir = m_localFolderEdit->text().trimmed().toStdString();
        res = application::PublishingService::publishArtifact(
            "dest_local", m_releaseRecordId, m_artifactPath,
            domain::DestinationKind::LocalFolder, targetDir,
            m_ghTagEdit->text().trimmed().toStdString(),
            m_releaseTitle, "", true
        );
    } else {
        std::string repo = m_ghRepoEdit->text().trimmed().toStdString();
        std::string configJson = "{\"repository\": \"" + domain::json::escapeString(repo) +
            "\", \"is_draft\": " + (m_ghDraftCheck->isChecked() ? "true" : "false") +
            ", \"is_prerelease\": " + (m_ghPrereleaseCheck->isChecked() ? "true" : "false") + "}";

        res = application::PublishingService::publishArtifact(
            "dest_github", m_releaseRecordId, m_artifactPath,
            domain::DestinationKind::GitHubReleases, configJson,
            m_ghTagEdit->text().trimmed().toStdString(),
            m_releaseTitle, "Release notes for " + m_releaseTitle, true
        );
    }

    m_progressBar->setVisible(false);

    if (res.success) {
        m_resultLabel->setText("<span style='color: #10B981; font-weight: bold;'>Successfully published!</span><br>Target: " +
            QString::fromStdString(res.external_url));
        m_cancelBtn->setText("Close");
    } else {
        m_resultLabel->setText("<span style='color: #EF4444; font-weight: bold;'>Publish Failed: " +
            QString::fromStdString(res.error_message) + "</span>");
        m_confirmationCheck->setEnabled(true);
        m_publishBtn->setEnabled(true);
    }
}

} // namespace shipdesk::ui
