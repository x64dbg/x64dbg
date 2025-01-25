#include <QMessageBox>
#include <QSettings>

#include "StructWidget.h"
#include "ui_StructWidget.h"
#include "Configuration.h"
#include "btparser/types.h"

StructWidget::StructWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::StructWidget)
{
    ui->setupUi(this);
    ui->plainTextEditDeclaration->setFont(Config()->monospaceFont());
    ui->plainTextEditMessages->setFont(Config()->monospaceFont());

    QSettings settings;
    auto declaration = settings.value("StructWidgetDeclaration").toString();
    ui->plainTextEditDeclaration->setPlainText(declaration);
    auto endCursor = ui->plainTextEditDeclaration->textCursor();
    endCursor.movePosition(QTextCursor::End);
    ui->plainTextEditDeclaration->setTextCursor(endCursor);
}

StructWidget::~StructWidget()
{
    delete ui;
}

void StructWidget::on_pushButtonParse_clicked()
{
    auto declaration = ui->plainTextEditDeclaration->toPlainText().toStdString();
    std::vector<std::string> errors;
    Types::TypeManager manager(8);
    ui->plainTextEditMessages->clear();
    if(!manager.ParseTypes(declaration, "StructWidget", errors))
    {
        QString message = "Parsing failed:";
        for(const auto & error : errors)
        {
            message += "\n";
            message += QString::fromStdString(error);
        }
        ui->plainTextEditMessages->setPlainText(message);
        return;
    }

    ui->plainTextEditMessages->setPlainText("Parsed successfully!");
}


void StructWidget::on_plainTextEditDeclaration_textChanged()
{
    QSettings settings;
    settings.setValue("StructWidgetDeclaration", ui->plainTextEditDeclaration->toPlainText());
    settings.sync();
}

