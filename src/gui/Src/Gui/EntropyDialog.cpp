#include "EntropyDialog.h"
#include "ui_EntropyDialog.h"

EntropyDialog::EntropyDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::EntropyDialog)
{
    ui->setupUi(this);

    /* the graph now scales, so enable window resize */
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    //setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    //setFixedSize(this->size()); //fixed size

    ui->entropyView->hide(); /* RenderGraph() onto a QLabel instead */

    mBlockSize = 128;
    mPointCount = 300;
    mInitialized = false;
}

EntropyDialog::~EntropyDialog()
{
    delete ui;
}

void EntropyDialog::GraphMemory(const unsigned char* data, int dataSize, QColor color)
{
    initializeGraph();
    ui->entropyView->GraphMemory(data, dataSize, mBlockSize, mPointCount, color);
    RenderGraph();
}

void EntropyDialog::GraphFile(const QString & fileName, QColor color)
{
    initializeGraph();
    ui->entropyView->GraphFile(fileName, mBlockSize, mPointCount, color);
    RenderGraph();
}

void EntropyDialog::RenderGraph()
{
    const int gridDensity = 16;
    const double heightPercent = 0.95; /* dont use the entire graph height */

    QPixmap pixmap(ui->graphLabel->size());
    QPainter painter(&pixmap);

    const double width = pixmap.width();
    const double height = pixmap.height();

    painter.setRenderHints(QPainter::Antialiasing);
    painter.fillRect(pixmap.rect(), QColor(255, 255, 255));

    /* pen for grid */
    QPen gridPen = painter.pen();
    gridPen.setWidthF(0.25);
    gridPen.setColor(QColor(0, 0, 0));
    painter.setPen(gridPen);

    /* draw horizontal grid */
    for(int i = 0; i < gridDensity; ++i)
    {
        double fi = i / double(gridDensity - 1);
        double ypos = fi * height;
        painter.drawLine(0, ypos, width, ypos);
    }

    /* draw vertical grid */
    double aspectRatio = pixmap.width() / double(pixmap.height());
    int gridDenistyVertical = gridDensity * aspectRatio;
    for(int i = 0; i < gridDenistyVertical; ++i)
    {
        double fi = i / double(gridDenistyVertical - 1);
        double xpos = fi * width;
        painter.drawLine(xpos, 0, xpos, height);
    }

    /* get graph limits for scaling */
    std::vector<double> data = ui->entropyView->GetGraphData();
    double dataMin = std::numeric_limits<double>::max();
    double dataMax = -std::numeric_limits<double>::max();
    for(size_t i = 0; i < data.size(); ++i)
    {
        dataMin = std::min(dataMin, data[i]);
        dataMax = std::max(dataMax, data[i]);
    }

    /* use this pen for drawing the graph edge */
    QPen graphPen = painter.pen();
    graphPen.setWidthF(1.0);
    graphPen.setColor(QColor(0, 0, 0));
    painter.setPen(graphPen);

    /* filled slightly transparent so you can see the grid underneath */
    painter.setBrush(QColor(255, 128, 0, 128));

    /* setup polyon points */
    std::vector<QPointF> polygon;
    polygon.push_back(QPointF(0, height));
    for(size_t i = 0; i < data.size(); ++i)
    {
        double fi = i / double(data.size() - 1);
        double norm = (data[i] - dataMin) / (dataMax - dataMin);
        double ypos = 1 - (1 - norm) * heightPercent;
        polygon.push_back(QPointF(fi * width, ypos * height));
    }

    /* draw graph as a filled polygon */
    polygon.push_back(QPointF(width, height));
    painter.drawPolygon(&polygon[0], polygon.size());

    ui->graphLabel->setPixmap(pixmap);
}

void EntropyDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    RenderGraph();
}

void EntropyDialog::initializeGraph()
{
    if(mInitialized)
        return;
    mInitialized = true;
    ui->entropyView->InitializeGraph();
}

