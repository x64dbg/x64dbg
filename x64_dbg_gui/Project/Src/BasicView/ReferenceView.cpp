#include "ReferenceView.h"
#include "ui_ReferenceView.h"

ReferenceView::ReferenceView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ReferenceView)
{
    ui->setupUi(this);

    QFont wFont("Monospace", 8);
    wFont.setStyleHint(QFont::Monospace);
    wFont.setFixedPitch(true);

    int charwidth=QFontMetrics(wFont).width(QChar(' '));

    mReferenceList = new StdTable();
    mReferenceList->addColumnAt(charwidth*2*sizeof(int_t)+8, "Address", true);
    mReferenceList->addColumnAt(0,"Data", true);

    ui->mainSplitter->insertWidget(0, mReferenceList);

    mMainLayout = new QVBoxLayout();
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->addWidget(ui->mainSplitter);
    setLayout(mMainLayout);

    ui->mainSplitter->setStretchFactor(0, 1000);
    ui->mainSplitter->setStretchFactor(0, 1);
    for(int i=0; i<ui->mainSplitter->count(); i++)
        ui->mainSplitter->handle(i)->setEnabled(false);
}

ReferenceView::~ReferenceView()
{
    delete ui;
}
