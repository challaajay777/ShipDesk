#include "ui/pages/source_inspector_page.h"
#include "application/inspection/source_inspection_service.h"
#include "infrastructure/filesystem/path_utils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>

namespace shipdesk::ui {

SourceInspectorPage::SourceInspectorPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void SourceInspectorPage::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    auto* header = new QLabel(
        "<b>Source Inspector Tool</b><br>"
        "<span style='color: #80868B;'>Inspect any local directory tree safely without modifying original files or timestamps.</span>",
        this
    );
    mainLayout->addWidget(header);

    auto* inputRow = new QHBoxLayout();
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText("Select folder to inspect...");
    m_browseBtn = new QPushButton("Browse...", this);
    m_inspectBtn = new QPushButton("Inspect", this);
    m_inspectBtn->setDefault(true);

    connect(m_browseBtn, &QPushButton::clicked, this, &SourceInspectorPage::onBrowseClicked);
    connect(m_inspectBtn, &QPushButton::clicked, this, &SourceInspectorPage::onInspectClicked);

    inputRow->addWidget(m_pathEdit, 1);
    inputRow->addWidget(m_browseBtn);
    inputRow->addWidget(m_inspectBtn);
    mainLayout->addLayout(inputRow);

    m_summaryLabel = new QLabel("Ready to inspect.", this);
    mainLayout->addWidget(m_summaryLabel);

    m_resultsTable = new QTableWidget(this);
    m_resultsTable->setColumnCount(5);
    m_resultsTable->setHorizontalHeaderLabels({"Relative Path", "Type", "Size", "Hidden", "Status"});
    m_resultsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_resultsTable);
}

void SourceInspectorPage::onBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory to Inspect");
    if (!dir.isEmpty()) {
        m_pathEdit->setText(dir);
    }
}

void SourceInspectorPage::onInspectClicked() {
    QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty()) return;

    domain::ReleaseProfile defaultProfile = domain::ReleaseProfile::createDefault("General File Bundle");
    defaultProfile.setIncludeHiddenFiles(true);

    application::SourceInspectionService service;
    auto res = service.inspectRoot(path.toStdString(), "inspector_tool", defaultProfile);

    if (!res.success) {
        QMessageBox::critical(this, "Inspection Error", QString::fromStdString(res.error_message));
        return;
    }

    m_resultsTable->setRowCount(0);
    for (size_t i = 0; i < res.items.size(); ++i) {
        const auto& item = res.items[i];
        m_resultsTable->insertRow(static_cast<int>(i));

        m_resultsTable->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::fromStdString(item.relative_path)));
        m_resultsTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(domain::sourceItemTypeToString(item.item_type))));
        m_resultsTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(item.isFile() ? QString::fromStdString(infrastructure::PathUtils::formatByteSize(item.size_bytes)) : "-"));
        m_resultsTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(item.is_hidden ? "Yes" : "No"));
        m_resultsTable->setItem(static_cast<int>(i), 4, new QTableWidgetItem(item.included ? "Included" : QString::fromStdString(item.exclusion_reason)));
    }

    m_summaryLabel->setText(QString("Found %1 items (%2 files, %3 total) in %4 ms.")
        .arg(res.items.size())
        .arg(res.total_files_count)
        .arg(QString::fromStdString(infrastructure::PathUtils::formatByteSize(res.included_bytes)))
        .arg(res.duration_ms));
}

} // namespace shipdesk::ui
