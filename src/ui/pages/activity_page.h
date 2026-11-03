#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

namespace shipdesk::ui {

class ActivityPage : public QWidget {
    Q_OBJECT
public:
    explicit ActivityPage(QWidget* parent = nullptr);

    void refreshData();

private slots:
    void onFilterChanged();

private:
    void setupUi();

    QComboBox* m_severityFilter{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    QPushButton* m_refreshBtn{nullptr};
    QTableWidget* m_eventsTable{nullptr};
};

} // namespace shipdesk::ui
