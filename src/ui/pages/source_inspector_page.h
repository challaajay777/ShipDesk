#pragma once

#include "domain/source/inventory_item.h"
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>

namespace shipdesk::ui {

class SourceInspectorPage : public QWidget {
    Q_OBJECT
public:
    explicit SourceInspectorPage(QWidget* parent = nullptr);

private slots:
    void onBrowseClicked();
    void onInspectClicked();

private:
    void setupUi();

    QLineEdit* m_pathEdit{nullptr};
    QPushButton* m_browseBtn{nullptr};
    QPushButton* m_inspectBtn{nullptr};
    QLabel* m_summaryLabel{nullptr};
    QTableWidget* m_resultsTable{nullptr};
};

} // namespace shipdesk::ui
