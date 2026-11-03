#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

namespace shipdesk::ui {

class DiagnosticsPage : public QWidget {
    Q_OBJECT
public:
    explicit DiagnosticsPage(QWidget* parent = nullptr);

    void refreshDiagnostics();

private slots:
    void onRunDbCheckClicked();
    void onExportClicked();

private:
    void setupUi();

    QTextEdit* m_diagView{nullptr};
    QPushButton* m_refreshBtn{nullptr};
    QPushButton* m_checkDbBtn{nullptr};
    QPushButton* m_exportBtn{nullptr};
};

} // namespace shipdesk::ui
