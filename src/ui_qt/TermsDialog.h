#pragma once
#include <QDialog>
class QTextBrowser;
class QPushButton;

class TermsDialog : public QDialog {
    Q_OBJECT
public:
    explicit TermsDialog(QWidget* parent = nullptr);
    bool acceptedTerms() const { return m_ok; }
private:
    bool m_ok = false;
};
