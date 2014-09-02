#include "changecommandline.h"
#include "ui_changecommandline.h"
#include <QMessageBox>
#include <QIcon>

ChangeCommandline::ChangeCommandline(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::ChangeCommandline)
{
    ui->setupUi(this);
    char* cmd_line;

    //set window flags
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);

    if(! DbgFunctions()->GetCmdline(& cmd_line))
        ui->lneditCommandline->setText("Cant get remote command line use getcmdline command for more information");
    else
    {
        ui->lneditCommandline->setText(QString(cmd_line));
        ui->lneditCommandline->setCursorPosition(0);
        free(cmd_line);
    }
}

ChangeCommandline::~ChangeCommandline()
{
    delete ui;
}

void ChangeCommandline::on_buttonBox_accepted()
{
    if(!DbgFunctions()->SetCmdline((char*)ui->lneditCommandline->text().toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Warning, "ERROR CANT SET COMMAND LINE", "ERROR SETTING COMMAND LINE TRY SETCOMMANDLINE COMMAND");
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }
    else
        GuiAddStatusBarMessage(QString("New command line: " + ui->lneditCommandline->text() + "\n").toUtf8().constData());
}
