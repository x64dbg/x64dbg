#pragma once

#include <QWidget>

#include "PatternLanguage.h"

namespace Ui
{
class DataExplorerDialog;
}

class DataExplorerDialog : public QWidget
{
    Q_OBJECT

public:
    explicit DataExplorerDialog(QWidget* parent);
    ~DataExplorerDialog();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_buttonParse_pressed();
    void on_logEdit_anchorClicked(const QUrl &url);

private:
    void logHandler(LogLevel level, const char *message);
    void compileError(const CompileError& error);
    void evalError(const EvalError& error);

    Ui::DataExplorerDialog *ui;
    struct PatternVisitor* mVisitor = nullptr;
    class PatternHighlighter* mHighlighter = nullptr;
};
