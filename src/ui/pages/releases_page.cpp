#include "ui/pages/releases_page.h"
#include "infrastructure/database/database_manager.h"
#include "application/verification/verification_service.h"
#include "domain/common/json_util.h"
#include "domain/artifact/manifest.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <fstream>

namespace shipdesk::ui {

ReleasesPage::ReleasesPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refreshData();
}

void ReleasesPage::setupUi() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left pane: Table of Releases
    auto* leftWidget = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto* tableTitle = new QLabel("<b>Immutable Release History</b>", leftWidget);
    leftLayout->addWidget(tableTitle);

    m_releasesTable = new QTableWidget(leftWidget);
    m_releasesTable->setColumnCount(3);
    m_releasesTable->setHorizontalHeaderLabels({"Title", "Version", "Completed (UTC)"});
    m_releasesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_releasesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_releasesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_releasesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_releasesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_releasesTable, &QTableWidget::cellClicked, this, &ReleasesPage::onReleaseSelected);
    leftLayout->addWidget(m_releasesTable);

    splitter->addWidget(leftWidget);

    // Right pane: Release Details & Manifest
    auto* rightWidget = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(12, 0, 0, 0);
    rightLayout->setSpacing(16);

    auto* infoBox = new QGroupBox("Release Record Details", rightWidget);
    auto* formLayout = new QFormLayout(infoBox);

    m_titleLabel = new QLabel("-", infoBox);
    m_versionLabel = new QLabel("-", infoBox);
    m_dateLabel = new QLabel("-", infoBox);
    m_manifestPathLabel = new QLabel("-", infoBox);
    m_manifestPathLabel->setWordWrap(true);

    formLayout->addRow("Title:", m_titleLabel);
    formLayout->addRow("Version:", m_versionLabel);
    formLayout->addRow("Completed At:", m_dateLabel);
    formLayout->addRow("Manifest Path:", m_manifestPathLabel);

    rightLayout->addWidget(infoBox);

    auto* manifestBox = new QGroupBox("Release Manifest JSON", rightWidget);
    auto* manifestLayout = new QVBoxLayout(manifestBox);
    m_manifestView = new QTextEdit(manifestBox);
    m_manifestView->setReadOnly(true);
    m_manifestView->setFontFamily("Courier New");
    manifestLayout->addWidget(m_manifestView);
    rightLayout->addWidget(manifestBox, 1);

    auto* actionRow = new QHBoxLayout();
    m_openFolderBtn = new QPushButton("Open Artifact Location", rightWidget);
    m_reverifyBtn = new QPushButton("Re-verify Package", rightWidget);
    m_publishBtn = new QPushButton("Publish Release...", rightWidget);
    m_publishBtn->setDefault(true);

    connect(m_openFolderBtn, &QPushButton::clicked, this, &ReleasesPage::onOpenFolderClicked);
    connect(m_reverifyBtn, &QPushButton::clicked, this, &ReleasesPage::onReverifyClicked);
    connect(m_publishBtn, &QPushButton::clicked, this, &ReleasesPage::onPublishClicked);

    actionRow->addWidget(m_openFolderBtn);
    actionRow->addWidget(m_reverifyBtn);
    actionRow->addStretch();
    actionRow->addWidget(m_publishBtn);
    rightLayout->addLayout(actionRow);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
}

void ReleasesPage::refreshData() {
    auto records = infrastructure::DatabaseManager::instance().listReleaseRecords();

    std::string prevId = m_currentRecordId;
    m_releasesTable->setRowCount(0);

    int selectRow = -1;
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& rec = records[i];
        m_releasesTable->insertRow(static_cast<int>(i));

        std::string title = "Release";
        std::string version = "1.0.0";
        if (!rec.metadata_snapshot_json.empty()) {
            title = domain::json::extractStringField(rec.metadata_snapshot_json, "title", "Release");
            version = domain::json::extractStringField(rec.metadata_snapshot_json, "version", "1.0.0");
        }

        auto* itemTitle = new QTableWidgetItem(QString::fromStdString(title));
        itemTitle->setData(Qt::UserRole, QString::fromStdString(rec.id));

        m_releasesTable->setItem(static_cast<int>(i), 0, itemTitle);
        m_releasesTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(version)));
        m_releasesTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::fromStdString(rec.completed_at)));

        if (rec.id == prevId) {
            selectRow = static_cast<int>(i);
        }
    }

    if (selectRow >= 0) {
        m_releasesTable->selectRow(selectRow);
        onReleaseSelected(selectRow, 0);
    } else if (m_releasesTable->rowCount() > 0) {
        m_releasesTable->selectRow(0);
        onReleaseSelected(0, 0);
    } else {
        clearDetails();
    }
}

void ReleasesPage::selectRelease(const QString& releaseId) {
    for (int i = 0; i < m_releasesTable->rowCount(); ++i) {
        auto* item = m_releasesTable->item(i, 0);
        if (item && item->data(Qt::UserRole).toString() == releaseId) {
            m_releasesTable->selectRow(i);
            onReleaseSelected(i, 0);
            break;
        }
    }
}

void ReleasesPage::onReleaseSelected(int row, int /*column*/) {
    if (row < 0 || row >= m_releasesTable->rowCount()) return;

    auto* titleItem = m_releasesTable->item(row, 0);
    if (!titleItem) return;

    std::string recId = titleItem->data(Qt::UserRole).toString().toStdString();
    auto optRec = infrastructure::DatabaseManager::instance().getReleaseRecord(recId);
    if (optRec) {
        loadReleaseDetails(*optRec);
    }
}

void ReleasesPage::loadReleaseDetails(const domain::ImmutableReleaseRecord& rec) {
    m_currentRecordId = rec.id;
    m_currentManifestPath = rec.manifest_path;

    std::string title = domain::json::extractStringField(rec.metadata_snapshot_json, "title", "Release");
    std::string version = domain::json::extractStringField(rec.metadata_snapshot_json, "version", "1.0.0");

    m_titleLabel->setText(QString::fromStdString(title));
    m_versionLabel->setText(QString::fromStdString(version));
    m_dateLabel->setText(QString::fromStdString(rec.completed_at));
    m_manifestPathLabel->setText(QString::fromStdString(rec.manifest_path));

    // Read manifest content
    std::ifstream ifs(rec.manifest_path);
    if (ifs.is_open()) {
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        m_manifestView->setPlainText(QString::fromStdString(content));
    } else {
        m_manifestView->setPlainText("(Manifest file could not be read from disk)");
    }
}

void ReleasesPage::clearDetails() {
    m_currentRecordId.clear();
    m_currentManifestPath.clear();
    m_titleLabel->setText("-");
    m_versionLabel->setText("-");
    m_dateLabel->setText("-");
    m_manifestPathLabel->setText("-");
    m_manifestView->clear();
}

void ReleasesPage::onOpenFolderClicked() {
    if (m_currentManifestPath.empty()) return;
    QFileInfo fi(QString::fromStdString(m_currentManifestPath));
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
}

void ReleasesPage::onReverifyClicked() {
    if (m_currentManifestPath.empty()) return;

    std::ifstream ifs(m_currentManifestPath);
    if (!ifs.is_open()) {
        QMessageBox::critical(this, "Verification Error", "Cannot open manifest file for re-verification.");
        return;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto manifest = domain::ReleaseManifest::fromJsonString(content);

    // Look for artifact file in the manifest's directory or recorded artifact entry
    QFileInfo fi(QString::fromStdString(m_currentManifestPath));
    QDir dir = fi.absoluteDir();

    QString artifactPath;
    if (!manifest.artifacts.empty()) {
        QString candidate = QString::fromStdString(manifest.artifacts.front().output_path);
        if (QFile::exists(candidate)) {
            artifactPath = candidate;
        } else {
            QString candInDir = dir.absoluteFilePath(QString::fromStdString(manifest.artifacts.front().filename));
            if (QFile::exists(candInDir)) {
                artifactPath = candInDir;
            }
        }
    }

    if (artifactPath.isEmpty()) {
        QStringList zips = dir.entryList({"*.zip"}, QDir::Files);
        if (!zips.isEmpty()) {
            artifactPath = dir.absoluteFilePath(zips.first());
        } else {
            QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            if (!subdirs.isEmpty()) {
                artifactPath = dir.absoluteFilePath(subdirs.first());
            }
        }
    }

    if (artifactPath.isEmpty()) {
        QMessageBox::warning(this, "Artifact Not Found", "Could not locate deliverable package artifact in: " + dir.absolutePath());
        return;
    }

    auto report = application::VerificationService::verifyArtifact(artifactPath.toStdString(), manifest);

    if (report.is_valid) {
        QMessageBox::information(this, "Verification Succeeded",
            "Artifact integrity verified!\n"
            "Archive Structure: OK\n"
            "Expected Files: " + QString::number(report.expected_files_count) + "\n"
            "Actual Files: " + QString::number(report.actual_files_count) + "\n"
            "Digests: All match.");
    } else {
        QString msg = "Verification FAILED:\n";
        for (const auto& m : report.messages) msg += "- " + QString::fromStdString(m) + "\n";
        for (const auto& mf : report.missing_files) msg += "- Missing: " + QString::fromStdString(mf) + "\n";
        for (const auto& [f, err] : report.mismatched_checksums) msg += "- Checksum mismatch: " + QString::fromStdString(f) + " (" + QString::fromStdString(err) + ")\n";
        QMessageBox::critical(this, "Verification Failed", msg);
    }
}

void ReleasesPage::onPublishClicked() {
    if (m_currentRecordId.empty()) return;
    emit publishRequested(QString::fromStdString(m_currentRecordId));
}

} // namespace shipdesk::ui
