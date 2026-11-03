#include "ui/pages/overview_page.h"
#include "infrastructure/database/database_manager.h"
#include "domain/common/json_util.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>

namespace shipdesk::ui {

OverviewPage::OverviewPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refreshData();
}

void OverviewPage::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    // Header / Banner
    auto* bannerBox = new QGroupBox(this);
    bannerBox->setTitle("ShipDesk — Native Release Workspace");
    auto* bannerLayout = new QVBoxLayout(bannerBox);
    bannerLayout->setContentsMargins(16, 16, 16, 16);

    auto* promiseLabel = new QLabel(
        "<b>Prepare, check, package, verify and deliver digital work without losing control of originals.</b><br>"
        "<span style='color: #80868B;'>Local-first, source-read-only packaging engine with cryptographic verification.</span>",
        bannerBox
    );
    promiseLabel->setWordWrap(true);
    bannerLayout->addWidget(promiseLabel);

    auto* btnRow = new QHBoxLayout();
    m_createReleaseBtn = new QPushButton("Create Release (Guided)", bannerBox);
    m_createReleaseBtn->setDefault(true);
    connect(m_createReleaseBtn, &QPushButton::clicked, this, &OverviewPage::createReleaseRequested);

    m_newWorkspaceBtn = new QPushButton("New Workspace", bannerBox);
    connect(m_newWorkspaceBtn, &QPushButton::clicked, this, &OverviewPage::newWorkspaceRequested);

    m_statsLabel = new QLabel("Loading workspace stats...", bannerBox);

    btnRow->addWidget(m_createReleaseBtn);
    btnRow->addWidget(m_newWorkspaceBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_statsLabel);
    bannerLayout->addLayout(btnRow);

    mainLayout->addWidget(bannerBox);

    // Split section for Recent Workspaces and Recent Releases
    auto* contentRow = new QHBoxLayout();
    contentRow->setSpacing(20);

    // Left: Recent Workspaces
    auto* wsBox = new QGroupBox("Workspaces", this);
    auto* wsLayout = new QVBoxLayout(wsBox);
    m_recentWorkspacesList = new QListWidget(wsBox);
    connect(m_recentWorkspacesList, &QListWidget::itemDoubleClicked, this, &OverviewPage::onWorkspaceDoubleClicked);
    wsLayout->addWidget(m_recentWorkspacesList);
    contentRow->addWidget(wsBox, 1);

    // Right: Recent Releases
    auto* relBox = new QGroupBox("Recent Releases & Deliverables", this);
    auto* relLayout = new QVBoxLayout(relBox);
    m_recentReleasesTable = new QTableWidget(relBox);
    m_recentReleasesTable->setColumnCount(4);
    m_recentReleasesTable->setHorizontalHeaderLabels({"Title / Package", "Version", "Date (UTC)", "Manifest Path"});
    m_recentReleasesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_recentReleasesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_recentReleasesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_recentReleasesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_recentReleasesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recentReleasesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_recentReleasesTable, &QTableWidget::itemDoubleClicked, this, &OverviewPage::onReleaseDoubleClicked);
    relLayout->addWidget(m_recentReleasesTable);
    contentRow->addWidget(relBox, 2);

    mainLayout->addLayout(contentRow, 1);
}

void OverviewPage::refreshData() {
    auto workspaces = infrastructure::DatabaseManager::instance().listWorkspaces(false);
    auto releaseRecords = infrastructure::DatabaseManager::instance().listReleaseRecords();

    // Workspaces
    m_recentWorkspacesList->clear();
    for (const auto& ws : workspaces) {
        auto* item = new QListWidgetItem(QString::fromStdString(ws.name() + " (" + ws.category() + ")"));
        item->setData(Qt::UserRole, QString::fromStdString(ws.id()));
        m_recentWorkspacesList->addItem(item);
    }
    if (workspaces.empty()) {
        auto* item = new QListWidgetItem("No workspaces yet. Click 'New Workspace' to create one.");
        item->setFlags(Qt::NoItemFlags);
        m_recentWorkspacesList->addItem(item);
    }

    // Releases
    m_recentReleasesTable->setRowCount(0);
    for (size_t i = 0; i < releaseRecords.size(); ++i) {
        const auto& rec = releaseRecords[i];
        m_recentReleasesTable->insertRow(static_cast<int>(i));

        std::string title = "Release";
        std::string version = "1.0.0";
        // Parse metadata snapshot
        if (!rec.metadata_snapshot_json.empty()) {
            title = domain::json::extractStringField(rec.metadata_snapshot_json, "title", "Release");
            version = domain::json::extractStringField(rec.metadata_snapshot_json, "version", "1.0.0");
        }

        auto* itemTitle = new QTableWidgetItem(QString::fromStdString(title));
        itemTitle->setData(Qt::UserRole, QString::fromStdString(rec.id));
        m_recentReleasesTable->setItem(static_cast<int>(i), 0, itemTitle);
        m_recentReleasesTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(version)));
        m_recentReleasesTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::fromStdString(rec.completed_at)));
        m_recentReleasesTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::fromStdString(rec.manifest_path)));
    }

    m_statsLabel->setText(QString("Workspaces: %1 | Releases: %2")
        .arg(workspaces.size())
        .arg(releaseRecords.size()));
}

void OverviewPage::onWorkspaceDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    QString id = item->data(Qt::UserRole).toString();
    if (!id.isEmpty()) {
        emit openWorkspaceRequested(id);
    }
}

void OverviewPage::onReleaseDoubleClicked(QTableWidgetItem* item) {
    if (!item) return;
    int row = item->row();
    auto* titleItem = m_recentReleasesTable->item(row, 0);
    if (titleItem) {
        QString id = titleItem->data(Qt::UserRole).toString();
        if (!id.isEmpty()) {
            emit openReleaseRequested(id);
        }
    }
}

} // namespace shipdesk::ui
