#include "AttachDialog.h"
#include "ui_AttachDialog.h"
#include <QMenu>

AttachDialog::AttachDialog(QWidget* parent) : QDialog(parent), ui(new Ui::AttachDialog)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size

    //setup actions
    mAttachAction = new QAction("Attach", this);
    mAttachAction->setShortcut(QKeySequence("enter"));
    connect(mAttachAction, SIGNAL(triggered()), this, SLOT(on_btnAttach_clicked()));

    mRefreshAction = new QAction("Refresh", this);
    mRefreshAction->setShortcut(QKeySequence("F5"));
    connect(mRefreshAction, SIGNAL(triggered()), this, SLOT(refresh()));
    this->addAction(mRefreshAction);


    //setup process list
    int charwidth = ui->listProcesses->getCharWidth();
    ui->listProcesses->addColumnAt(charwidth * sizeof(int) * 2 + 8, "PID", true);
    ui->listProcesses->addColumnAt(800, "Path", true);
    connect(ui->listProcesses, SIGNAL(enterPressedSignal()), this, SLOT(on_btnAttach_clicked()));
    connect(ui->listProcesses, SIGNAL(doubleClickedSignal()), this, SLOT(on_btnAttach_clicked()));
    connect(ui->listProcesses, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(processListContextMenu(QPoint)));

    refresh();
}

AttachDialog::~AttachDialog()
{
    delete ui;
}

void AttachDialog::refresh()
{
    ui->listProcesses->setRowCount(0);
    ui->listProcesses->setTableOffset(0);
    DBGPROCESSINFO* entries;
    int count;
    if(!DbgFunctions()->GetProcessList(&entries, &count))
        return;
    ui->listProcesses->setRowCount(count);
    for(int i = 0; i < count; i++)
    {
        ui->listProcesses->setCellContent(i, 0, QString().sprintf("%.8X", entries[i].dwProcessId));
        ui->listProcesses->setCellContent(i, 1, QString(entries[i].szExeFile));
    }
    ui->listProcesses->setSingleSelection(0);
    ui->listProcesses->reloadData();
}

void AttachDialog::on_btnAttach_clicked()
{
    QString pid = ui->listProcesses->getCellContent(ui->listProcesses->getInitialSelection(), 0);
    DbgCmdExec(QString("attach " + pid).toUtf8().constData());
    accept();
}

void AttachDialog::processListContextMenu(const QPoint & pos)
{
    QMenu* wMenu = new QMenu(this); //create context menu
    wMenu->addAction(mAttachAction);
    wMenu->addAction(mRefreshAction);
    QMenu wCopyMenu("&Copy", this);
    ui->listProcesses->setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu->addSeparator();
        wMenu->addMenu(&wCopyMenu);
    }
    wMenu->exec(mapToGlobal(pos)); //execute context menu
}
