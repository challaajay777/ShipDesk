#include "ui/pages/activity_page.h"
#include "infrastructure/database/database_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>

namespace shipdesk::ui {

ActivityPage::ActivityPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    refreshData();
}

void ActivityPage::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Filter row
    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Severity:", this));
    m_severityFilter = new QComboBox(this);
    m_severityFilter->addItems({"All", "Info", "Warning", "Error"});
    connect(m_severityFilter, &QComboBox::currentTextChanged, this, &ActivityPage::onFilterChanged);
    filterRow->addWidget(m_severityFilter);

    filterRow->addWidget(new QLabel("Search:", this));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Filter events...");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ActivityPage::onFilterChanged);
    filterRow->addWidget(m_searchEdit);

    m_refreshBtn = new QPushButton("Refresh", this);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ActivityPage::refreshData);
    filterRow->addWidget(m_refreshBtn);

    filterRow->addStretch();
    mainLayout->addLayout(filterRow);

    // Table
    m_eventsTable = new QTableWidget(this);
    m_eventsTable->setColumnCount(4);
    m_eventsTable->setHorizontalHeaderLabels({"Timestamp (UTC)", "Severity", "Event Type", "Message"});
    m_eventsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_eventsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_eventsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_eventsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_eventsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_eventsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_eventsTable);
}

void ActivityPage::refreshData() {
    onFilterChanged();
}

void ActivityPage::onFilterChanged() {
    auto events = infrastructure::DatabaseManager::instance().getEvents("", 500);

    QString filterSev = m_severityFilter->currentText();
    QString search = m_searchEdit->text().trimmed();

    m_eventsTable->setRowCount(0);
    int row = 0;
    for (const auto& ev : events) {
        if (filterSev != "All" && QString::fromStdString(ev.severity) != filterSev) {
            continue;
        }

        QString msg = QString::fromStdString(ev.message);
        QString type = QString::fromStdString(ev.event_type);
        if (!search.isEmpty() && !msg.contains(search, Qt::CaseInsensitive) && !type.contains(search, Qt::CaseInsensitive)) {
            continue;
        }

        m_eventsTable->insertRow(row);
        m_eventsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(ev.created_at_utc)));
        m_eventsTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(ev.severity)));
        m_eventsTable->setItem(row, 2, new QTableWidgetItem(type));
        m_eventsTable->setItem(row, 3, new QTableWidgetItem(msg));
        row++;
    }
}

} // namespace shipdesk::ui
