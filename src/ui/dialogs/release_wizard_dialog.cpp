#include "ui/dialogs/release_wizard_dialog.h"
#include "application/workspace/workspace_service.h"
#include "application/inspection/source_inspection_service.h"
#include "domain/validation/validator.h"
#include "infrastructure/database/database_manager.h"
#include "infrastructure/filesystem/path_utils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QFileInfo>
#include <thread>

namespace shipdesk::ui {

ReleaseWizardDialog::ReleaseWizardDialog(const QString& initialWorkspaceId, QWidget* parent)
    : QDialog(parent)
    , m_selectedWorkspaceId(initialWorkspaceId.toStdString())
{
    setWindowTitle("Create Release — Guided Wizard");
    resize(760, 560);
    setMinimumSize(640, 480);
    setupUi();
}

ReleaseWizardDialog::~ReleaseWizardDialog() {
    m_cancelExecutionFlag.store(true);
}

void ReleaseWizardDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    m_stepTitleLabel = new QLabel("<b>Step 1 of 8: Choose Deliverable Category</b>", this);
    m_stepTitleLabel->setStyleSheet("font-size: 14px; padding-bottom: 4px;");
    mainLayout->addWidget(m_stepTitleLabel);

    m_stack = new QStackedWidget(this);
    setupStep1Category();
    setupStep2Source();
    setupStep3Inventory();
    setupStep4Metadata();
    setupStep5Validation();
    setupStep6Format();
    setupStep7Review();
    setupStep8Execute();
    mainLayout->addWidget(m_stack, 1);

    // Navigation buttons row
    auto* navRow = new QHBoxLayout();
    m_cancelBtn = new QPushButton("Cancel", this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_backBtn = new QPushButton("Back", this);
    connect(m_backBtn, &QPushButton::clicked, this, &ReleaseWizardDialog::onBackClicked);

    m_nextBtn = new QPushButton("Next", this);
    m_nextBtn->setDefault(true);
    connect(m_nextBtn, &QPushButton::clicked, this, &ReleaseWizardDialog::onNextClicked);

    navRow->addWidget(m_cancelBtn);
    navRow->addStretch();
    navRow->addWidget(m_backBtn);
    navRow->addWidget(m_nextBtn);
    mainLayout->addLayout(navRow);

    m_stack->setCurrentIndex(0);
    updateNavButtons();
}

void ReleaseWizardDialog::setupStep1Category() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(16);

    auto* desc = new QLabel(
        "Select the category that best describes your deliverable.<br>"
        "This applies recommended packaging settings, sensible exclusion patterns, and checks.",
        page
    );
    desc->setWordWrap(true);
    layout->addWidget(desc);

    auto* form = new QFormLayout();
    m_categoryCombo = new QComboBox(page);
    m_categoryCombo->addItems({
        "General File Bundle",
        "Document Bundle",
        "Educational Resources",
        "Media Collection",
        "Software Application",
        "Source Code Release",
        "Game Mod",
        "Resource Pack",
        "Plugin/Extension",
        "Static Website Output",
        "Custom"
    });
    form->addRow("Deliverable Type:", m_categoryCombo);
    layout->addLayout(form);
    layout->addStretch();

    m_stack->addWidget(page);
}

void ReleaseWizardDialog::setupStep2Source() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(14);

    auto* desc = new QLabel(
        "Select the source workspace or choose a directory on disk to package.<br>"
        "<span style='color: #80868B;'>Original files will remain strictly unmodified.</span>",
        page
    );
    desc->setWordWrap(true);
    layout->addWidget(desc);

    auto* form = new QFormLayout();
    m_workspaceCombo = new QComboBox(page);

    auto workspaces = infrastructure::DatabaseManager::instance().listWorkspaces(false);
    for (const auto& ws : workspaces) {
        m_workspaceCombo->addItem(QString::fromStdString(ws.name()), QString::fromStdString(ws.id()));
    }
    m_workspaceCombo->addItem("(New Workspace from Folder...)", "");
    form->addRow("Workspace:", m_workspaceCombo);

    auto* sourceRow = new QHBoxLayout();
    m_sourcePathEdit = new QLineEdit(page);
    m_sourcePathEdit->setPlaceholderText("Select directory on disk...");
    m_browseSourceBtn = new QPushButton("Browse...", page);
    connect(m_browseSourceBtn, &QPushButton::clicked, this, &ReleaseWizardDialog::onBrowseSourceClicked);
    sourceRow->addWidget(m_sourcePathEdit, 1);
    sourceRow->addWidget(m_browseSourceBtn);
    form->addRow("Source Directory:", sourceRow);

    layout->addLayout(form);
    layout->addStretch();

    m_stack->addWidget(page);
}

void ReleaseWizardDialog::setupStep3Inventory() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(10);

    auto* searchRow = new QHBoxLayout();
    searchRow->addWidget(new QLabel("Filter:", page));
    m_inventorySearchEdit = new QLineEdit(page);
    m_inventorySearchEdit->setPlaceholderText("Search file names or paths...");
    connect(m_inventorySearchEdit, &QLineEdit::textChanged, this, &ReleaseWizardDialog::onSearchInventoryChanged);
    searchRow->addWidget(m_inventorySearchEdit, 1);
    layout->addLayout(searchRow);

    m_inventoryTable = new QTableWidget(page);
    m_inventoryTable->setColumnCount(4);
    m_inventoryTable->setHorizontalHeaderLabels({"Include", "Relative Path", "Type", "Size"});
    m_inventoryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_inventoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_inventoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_inventoryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_inventoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_inventoryTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem* item) {
        if (!item || item->column() != 0) return;
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < static_cast<int>(m_inventoryItems.size())) {
            m_inventoryItems[idx].included = (item->checkState() == Qt::Checked);
            uint64_t includedCount = 0;
            uint64_t includedBytes = 0;
            for (const auto& it : m_inventoryItems) {
                if (it.included && it.isFile()) {
                    includedCount++;
                    includedBytes += it.size_bytes;
                }
            }
            m_inventorySummaryLabel->setText(QString("Included Files: %1 | Estimated Uncompressed Size: %2")
                .arg(includedCount)
                .arg(QString::fromStdString(infrastructure::PathUtils::formatByteSize(includedBytes))));
            m_estimateSizeLabel->setText(QString::fromStdString(infrastructure::PathUtils::formatByteSize(includedBytes)));
        }
    });
    layout->addWidget(m_inventoryTable, 1);

    m_inventorySummaryLabel = new QLabel("0 files selected.", page);
    layout->addWidget(m_inventorySummaryLabel);

    m_stack->addWidget(page);
}

void ReleaseWizardDialog::setupStep4Metadata() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(12);

    auto* form = new QFormLayout();
    m_titleEdit = new QLineEdit("My-Release", page);
    m_versionEdit = new QLineEdit("0.1.0", page);
    m_channelCombo = new QComboBox(page);
    m_channelCombo->addItems({"Draft", "Preview", "Alpha", "Beta", "Stable"});

    m_descEdit = new QTextEdit(page);
    m_descEdit->setMaximumHeight(60);

    m_notesEdit = new QTextEdit(page);
    m_notesEdit->setMaximumHeight(90);

    form->addRow("Release Title:", m_titleEdit);
    form->addRow("Version Label:", m_versionEdit);
    form->addRow("Channel:", m_channelCombo);
    form->addRow("Description:", m_descEdit);
    form->addRow("Release Notes:", m_notesEdit);

    layout->addLayout(form);
    layout->addStretch();

    m_stack->addWidget(page);
}

void ReleaseWizardDialog::setupStep5Validation() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(10);

    m_validationStatusLabel = new QLabel("Validation in progress...", page);
    layout->addWidget(m_validationStatusLabel);

    m_validationTable = new QTableWidget(page);
    m_validationTable->setColumnCount(4);
    m_validationTable->setHorizontalHeaderLabels({"Severity", "Check", "Explanation", "Suggested Action"});
    m_validationTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_validationTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_validationTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_validationTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_validationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_validationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_validationTable, 1);

    m_stack->addWidget(page);
}

void ReleaseWizardDialog::setupStep6Format() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(14);

    auto* form = new QFormLayout();
    m_formatCombo = new QComboBox(page);
    m_formatCombo->addItems({"ZIP Archive (.zip)", "Organized Directory Package"});
    form->addRow("Package Output Format:", m_formatCombo);

    auto* destRow = new QHBoxLayout();
    QString defaultOut = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ShipDesk_Releases";
    m_destDirEdit = new QLineEdit(defaultOut, page);
    m_browseDestBtn = new QPushButton("Browse...", page);
    connect(m_browseDestBtn, &QPushButton::clicked, this, &ReleaseWizardDialog::onBrowseDestinationClicked);
    destRow->addWidget(m_destDirEdit, 1);
    destRow->addWidget(m_browseDestBtn);
    form->addRow("Destination Directory:", destRow);

    m_estimateSizeLabel = new QLabel("Estimated Size: Calculating...", page);
    form->addRow("Estimated Output:", m_estimateSizeLabel);

    layout->addLayout(form);
    layout->addStretch();

    m_stack->addWidget(page);
}

void ReleaseWizardDialog::setupStep7Review() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(10);

    auto* title = new QLabel("<b>Review Package Specifications</b><br>Please confirm release parameters before staging and packaging.", page);
    layout->addWidget(title);

    m_reviewSummaryEdit = new QTextEdit(page);
    m_reviewSummaryEdit->setReadOnly(true);
    m_reviewSummaryEdit->setFontFamily("Courier New");
    layout->addWidget(m_reviewSummaryEdit, 1);

    m_stack->addWidget(page);
}

void ReleaseWizardDialog::setupStep8Execute() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(12);

    auto* header = new QLabel("<b>Release Packaging & Verification</b>", page);
    layout->addWidget(header);

    m_phaseLabel = new QLabel("Ready to package.", page);
    layout->addWidget(m_phaseLabel);

    m_progressBar = new QProgressBar(page);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    layout->addWidget(m_progressBar);

    m_startExecBtn = new QPushButton("Start Release Execution", page);
    m_startExecBtn->setDefault(true);
    connect(m_startExecBtn, &QPushButton::clicked, this, &ReleaseWizardDialog::onStartExecutionClicked);
    layout->addWidget(m_startExecBtn);

    m_execLogEdit = new QTextEdit(page);
    m_execLogEdit->setReadOnly(true);
    m_execLogEdit->setFontFamily("Courier New");
    layout->addWidget(m_execLogEdit, 1);

    m_finalResultLabel = new QLabel("", page);
    m_finalResultLabel->setWordWrap(true);
    layout->addWidget(m_finalResultLabel);

    m_stack->addWidget(page);
}

void ReleaseWizardDialog::onBrowseSourceClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Source Directory");
    if (!dir.isEmpty()) {
        m_sourcePathEdit->setText(dir);
        QFileInfo fi(dir);
        if (m_titleEdit->text().trimmed().isEmpty() || m_titleEdit->text() == "My-Release") {
            m_titleEdit->setText(fi.baseName());
        }
    }
}

void ReleaseWizardDialog::onBrowseDestinationClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Destination");
    if (!dir.isEmpty()) {
        m_destDirEdit->setText(dir);
    }
}

void ReleaseWizardDialog::updateNavButtons() {
    int idx = m_stack->currentIndex();
    m_backBtn->setEnabled(idx > 0 && idx < 7);

    static const char* titles[] = {
        "Step 1 of 8: Choose Deliverable Category",
        "Step 2 of 8: Select Source Directory",
        "Step 3 of 8: Inventory & Inclusion Rules",
        "Step 4 of 8: Release Metadata & Versioning",
        "Step 5 of 8: Pre-Packaging Validation Checks",
        "Step 6 of 8: Package Format & Destination",
        "Step 7 of 8: Final Review",
        "Step 8 of 8: Execute Staging & Verification"
    };
    if (idx >= 0 && idx < 8) {
        m_stepTitleLabel->setText(QString("<b>%1</b>").arg(titles[idx]));
    }

    if (idx == 6) {
        m_nextBtn->setText("Proceed to Package");
    } else if (idx == 7) {
        m_nextBtn->setText("Done");
        m_nextBtn->setEnabled(!m_completedRecordId.empty());
    } else {
        m_nextBtn->setText("Next");
        m_nextBtn->setEnabled(true);
    }
}

void ReleaseWizardDialog::onNextClicked() {
    int idx = m_stack->currentIndex();

    if (idx == 1) {
        // Leaving step 2 -> ensure source path is valid and run inspection
        QString srcPath = m_sourcePathEdit->text().trimmed();
        if (srcPath.isEmpty() && m_workspaceCombo->currentData().toString().isEmpty()) {
            QMessageBox::warning(this, "Source Required", "Please select a valid source directory.");
            return;
        }
        runInspectionForStep3();
    } else if (idx == 2) {
        // Collect checkbox inclusion status from inventory table
        for (int row = 0; row < m_inventoryTable->rowCount(); ++row) {
            auto* chkItem = m_inventoryTable->item(row, 0);
            if (chkItem) {
                int originalIndex = chkItem->data(Qt::UserRole).toInt();
                if (originalIndex >= 0 && originalIndex < static_cast<int>(m_inventoryItems.size())) {
                    m_inventoryItems[originalIndex].included = (chkItem->checkState() == Qt::Checked);
                }
            }
        }
    } else if (idx == 4) {
        // Leaving step 4 -> run validation before step 5
        runValidationForStep5();
    } else if (idx == 5) {
        updateReviewForStep7();
    } else if (idx == 7) {
        accept();
        return;
    }

    if (idx < m_stack->count() - 1) {
        m_stack->setCurrentIndex(idx + 1);
        updateNavButtons();
    }
}

void ReleaseWizardDialog::onBackClicked() {
    int idx = m_stack->currentIndex();
    if (idx > 0) {
        m_stack->setCurrentIndex(idx - 1);
        updateNavButtons();
    }
}

void ReleaseWizardDialog::runInspectionForStep3() {
    QString srcPath = m_sourcePathEdit->text().trimmed();
    std::string cat = m_categoryCombo->currentText().toStdString();
    domain::ReleaseProfile profile = domain::ReleaseProfile::createDefault(cat);

    application::SourceInspectionService service;
    auto inspectRes = service.inspectRoot(srcPath.toStdString(), "wizard_source", profile);

    m_inventoryItems = inspectRes.items;
    onSearchInventoryChanged(m_inventorySearchEdit->text());
}

void ReleaseWizardDialog::onSearchInventoryChanged(const QString& text) {
    m_inventoryTable->blockSignals(true);
    m_inventoryTable->setRowCount(0);
    uint64_t includedCount = 0;
    uint64_t includedBytes = 0;

    int row = 0;
    for (size_t i = 0; i < m_inventoryItems.size(); ++i) {
        const auto& item = m_inventoryItems[i];
        QString relPath = QString::fromStdString(item.relative_path);

        if (!text.isEmpty() && !relPath.contains(text, Qt::CaseInsensitive)) {
            continue;
        }

        m_inventoryTable->insertRow(row);

        auto* chkItem = new QTableWidgetItem();
        chkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        chkItem->setCheckState(item.included ? Qt::Checked : Qt::Unchecked);
        chkItem->setData(Qt::UserRole, static_cast<int>(i));

        m_inventoryTable->setItem(row, 0, chkItem);
        m_inventoryTable->setItem(row, 1, new QTableWidgetItem(relPath));
        m_inventoryTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(domain::sourceItemTypeToString(item.item_type))));
        m_inventoryTable->setItem(row, 3, new QTableWidgetItem(item.isFile() ? QString::fromStdString(infrastructure::PathUtils::formatByteSize(item.size_bytes)) : "-"));

        if (item.included && item.isFile()) {
            includedCount++;
            includedBytes += item.size_bytes;
        }
        row++;
    }

    m_inventoryTable->blockSignals(false);

    m_inventorySummaryLabel->setText(QString("Included Files: %1 | Estimated Uncompressed Size: %2")
        .arg(includedCount)
        .arg(QString::fromStdString(infrastructure::PathUtils::formatByteSize(includedBytes))));

    m_estimateSizeLabel->setText(QString::fromStdString(infrastructure::PathUtils::formatByteSize(includedBytes)));
}

void ReleaseWizardDialog::runValidationForStep5() {
    std::string cat = m_categoryCombo->currentText().toStdString();
    domain::ReleaseProfile profile = domain::ReleaseProfile::createDefault(cat);
    domain::ReleaseDraft draft("temp", "temp", "temp", m_titleEdit->text().toStdString(), m_versionEdit->text().toStdString());

    m_currentFindings = domain::ReleaseValidator::validateInventory(m_inventoryItems, profile, draft);

    m_validationTable->setRowCount(0);
    bool hasBlocker = false;

    for (size_t i = 0; i < m_currentFindings.size(); ++i) {
        const auto& f = m_currentFindings[i];
        m_validationTable->insertRow(static_cast<int>(i));

        m_validationTable->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::fromStdString(domain::findingSeverityToString(f.severity))));
        m_validationTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(f.title)));
        m_validationTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::fromStdString(f.explanation)));
        m_validationTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::fromStdString(f.suggested_action)));

        if (f.isBlocker()) {
            hasBlocker = true;
        }
    }

    if (hasBlocker) {
        m_validationStatusLabel->setText("<span style='color: #EF4444; font-weight: bold;'>Blocker detected. You must resolve blockers before packaging.</span>");
    } else if (!m_currentFindings.empty()) {
        m_validationStatusLabel->setText("<span style='color: #F59E0B; font-weight: bold;'>Checks passed with warnings / recommendations.</span>");
    } else {
        m_validationStatusLabel->setText("<span style='color: #10B981; font-weight: bold;'>All pre-packaging checks passed cleanly.</span>");
    }
}

void ReleaseWizardDialog::updateReviewForStep7() {
    uint64_t totalFiles = 0;
    uint64_t totalBytes = 0;
    for (const auto& item : m_inventoryItems) {
        if (item.included && item.isFile()) {
            totalFiles++;
            totalBytes += item.size_bytes;
        }
    }

    QString summary = "================================================================================\n";
    summary += "RELEASE SPECIFICATION REVIEW\n";
    summary += "================================================================================\n\n";
    summary += "Title: " + m_titleEdit->text().trimmed() + "\n";
    summary += "Version: " + m_versionEdit->text().trimmed() + "\n";
    summary += "Category: " + m_categoryCombo->currentText() + "\n";
    summary += "Channel: " + m_channelCombo->currentText() + "\n";
    summary += "Source: " + m_sourcePathEdit->text().trimmed() + "\n";
    summary += "Package Format: " + m_formatCombo->currentText() + "\n";
    summary += "Destination Directory: " + m_destDirEdit->text().trimmed() + "\n\n";
    summary += "Included Files Count: " + QString::number(totalFiles) + "\n";
    summary += "Estimated Content Size: " + QString::fromStdString(infrastructure::PathUtils::formatByteSize(totalBytes)) + "\n";
    summary += "Validation Findings: " + QString::number(m_currentFindings.size()) + "\n";

    m_reviewSummaryEdit->setPlainText(summary);
}

void ReleaseWizardDialog::onStartExecutionClicked() {
    m_startExecBtn->setEnabled(false);
    m_cancelExecutionFlag.store(false);
    m_execLogEdit->clear();
    m_finalResultLabel->clear();

    // 1. Ensure workspace exists
    application::WorkspaceService wsService;
    std::string wsId = m_workspaceCombo->currentData().toString().toStdString();
    if (wsId.empty()) {
        auto newWs = wsService.createWorkspace(
            m_titleEdit->text().trimmed().toStdString() + " Workspace",
            "Created via Release Wizard",
            m_categoryCombo->currentText().toStdString()
        );
        wsId = newWs.id();
        wsService.addSourceRoot(wsId, m_sourcePathEdit->text().trimmed().toStdString(), m_titleEdit->text().trimmed().toStdString());
    }

    // 2. Create release draft
    application::ReleaseService relService;
    auto draft = relService.createDraft(
        wsId, "profile-default",
        m_titleEdit->text().trimmed().toStdString(),
        m_versionEdit->text().trimmed().toStdString(),
        domain::releaseChannelFromString(m_channelCombo->currentText().toStdString()),
        m_descEdit->toPlainText().toStdString(),
        m_notesEdit->toPlainText().toStdString()
    );

    // 3. Prepare paths
    std::filesystem::path stagingBase = std::filesystem::path(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString()
    ) / "staging";
    std::filesystem::path destDir(m_destDirEdit->text().trimmed().toStdString());
    domain::PackageFormat fmt = (m_formatCombo->currentIndex() == 0) ? domain::PackageFormat::ZipArchive : domain::PackageFormat::Directory;

    m_execLogEdit->append("Starting Release Pipeline for '" + QString::fromStdString(draft.title()) + "' v" + QString::fromStdString(draft.version()) + "...");
    QCoreApplication::processEvents();

    auto result = relService.executeVerticalSlice(
        draft.id(), stagingBase, destDir, fmt, &m_cancelExecutionFlag,
        [this](const std::string& phase, int progressPct, const std::string& detail) {
            m_phaseLabel->setText(QString("Phase: %1 — %2").arg(QString::fromStdString(phase)).arg(QString::fromStdString(detail)));
            m_progressBar->setValue(progressPct);
            m_execLogEdit->append(QString("[%1] %2").arg(QString::fromStdString(phase)).arg(QString::fromStdString(detail)));
            QCoreApplication::processEvents();
        }
    );

    if (result.success) {
        m_completedRecordId = result.release_record_id;
        m_phaseLabel->setText("Release successfully completed and cryptographically verified!");
        m_progressBar->setValue(100);

        QString okMsg = "<span style='color: #10B981; font-weight: bold;'>RELEASE SUCCEEDED & VERIFIED!</span><br>";
        okMsg += "<b>Artifact:</b> " + QString::fromStdString(result.packaging_result.artifact_path.string()) + "<br>";
        okMsg += "<b>Size:</b> " + QString::fromStdString(infrastructure::PathUtils::formatByteSize(result.packaging_result.artifact_size_bytes)) + "<br>";
        okMsg += "<b>SHA-256 Digest:</b> <code>" + QString::fromStdString(result.packaging_result.artifact_sha256) + "</code><br>";
        okMsg += "<b>Manifest:</b> " + QString::fromStdString(result.packaging_result.manifest_path.string());
        m_finalResultLabel->setText(okMsg);

        m_nextBtn->setEnabled(true);
        m_nextBtn->setText("Close");
    } else {
        m_phaseLabel->setText("Release execution stopped.");
        m_finalResultLabel->setText("<span style='color: #EF4444; font-weight: bold;'>Execution Error: " + QString::fromStdString(result.error_message) + "</span>");
        m_startExecBtn->setEnabled(true);
    }
}

} // namespace shipdesk::ui
