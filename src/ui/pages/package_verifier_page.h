#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>

namespace shipdesk::ui {

class PackageVerifierPage : public QWidget {
    Q_OBJECT
public:
    explicit PackageVerifierPage(QWidget* parent = nullptr);

private slots:
    void onBrowsePackageClicked();
    void onBrowseManifestClicked();
    void onVerifyClicked();

private:
    void setupUi();

    QLineEdit* m_packagePathEdit{nullptr};
    QPushButton* m_browsePkgBtn{nullptr};
    QLineEdit* m_manifestPathEdit{nullptr};
    QPushButton* m_browseManifestBtn{nullptr};
    QPushButton* m_verifyBtn{nullptr};

    QLabel* m_statusBadge{nullptr};
    QTextEdit* m_reportView{nullptr};
};

} // namespace shipdesk::ui
