#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QSplitter>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QCheckBox>
#include <QSpacerItem>

struct PatternVisitor
{
    int mNextId = 100;
    uint64_t mCurrentBase = 0;
    std::vector<char> mStringPool;
    std::map<int, std::pair<size_t, size_t>> mNames;

    bool valueCallback(const TYPEDESCRIPTOR* type, char* dest, size_t* destCount)
    {
        auto itr = mNames.find(type->id);
        if(itr == mNames.end())
            return false;

        const auto & str = itr->second;
        if(*destCount <= str.second)
        {
            *destCount = str.second + 1;
            return false;
        }
        strcpy_s(dest, *destCount, mStringPool.data() + str.first);
        return true;
    }

    TreeNode visit(TreeNode parent, const VisitInfo & info)
    {
        if(parent == nullptr)
        {
            mCurrentBase = info.offset;
        }

        auto id = mNextId++;
        {
            auto valueLen = strlen(info.value);
            auto valueIndex = mStringPool.size();
            mStringPool.resize(mStringPool.size() + valueLen + 1);
            memcpy(mStringPool.data() + valueIndex, info.value, valueLen + 1);
            mNames[id] = {valueIndex, valueLen};
        }

        std::string name = info.type_name;
        if(*info.variable_name)
        {
            name += " ";
            name += info.variable_name;
        }
        TYPEDESCRIPTOR td = {};
        td.magic = TYPEDESCRIPTOR_MAGIC;
        td.expanded = true;
        td.reverse = info.big_endian;
        td.name = name.c_str();
        td.addr = mCurrentBase;
        td.id = id;
        td.sizeBits = info.size * 8;
        // TODO: detect primitive types and exclude them from the toString callback
        td.offset = info.offset - mCurrentBase;
        td.callback = [](const TYPEDESCRIPTOR * type, char* dest, size_t* destCount)
        {
            return ((PatternVisitor*)type->userdata)->valueCallback(type, dest, destCount);
        };
        td.userdata = this;
        void* node = nullptr;
        emit Bridge::getBridge()->typeAddNode(parent, &td, &node);
        printf("[%p->%p] %s %s |%s| (address: 0x%llX, size: 0x%llX, line: %d)\n",
               parent,
               node,
               info.type_name,
               info.variable_name,
               info.value,
               info.offset,
               info.size,
               info.line
               );
        return node;
    }

    void clear()
    {
        mCurrentBase = 0;
        mStringPool.clear();
        mNames.clear();
    }
};


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupNavigation();
    setupWidgets();

    // Load the dump provided on the command line
    auto args = qApp->arguments();
    if(args.length() > 1)
    {
        loadFile(args.at(1));
    }

    mVisitor = new PatternVisitor;
}

MainWindow::~MainWindow()
{
    delete mVisitor;
    delete ui;
}

void MainWindow::loadFile(const QString & path)
{
    DbgSetMemoryProvider(nullptr);

    if(mFile != nullptr)
    {
        delete mFile;
        mFile = nullptr;
    }

    duint virtualBase = 0;
    try
    {
        mFile = new File(virtualBase, path);
    }
    catch(const std::exception & x)
    {
        QMessageBox::critical(this, tr("Error"), x.what());
    }

    DbgSetMemoryProvider(mFile);

    // Reload the views
    mHexDump->loadFile(mFile);
    emit mNavigation->gotoDump(virtualBase);
}

void MainWindow::setupNavigation()
{
    mNavigation = new Navigation(this);
    connect(mNavigation, &Navigation::focusWindow, [this](Navigation::Window window)
    {
        switch(window)
        {
        case Navigation::Dump:
            mHexDump->setFocus();
            break;
        default:
            qDebug() << "Unknown window: " << window;
            break;
        }
    });
    connect(mNavigation, &Navigation::gotoAddress, [this](Navigation::Window window, duint address)
    {
        switch(window)
        {
        case Navigation::Dump:
            qDebug() << "Dump at: " << address;
            mHexDump->printDumpAt(address);
            break;
        default:
            qDebug() << "Unknown window: " << window;
            break;
        }
    });
}

struct DefaultArchitecture : Architecture
{
    bool disasm64() const override
    {
        return true;
    }

    bool addr64() const override
    {
        return true;
    }
} gArchitecture;

Architecture* GlobalArchitecture()
{
    return &gArchitecture;
}

void MainWindow::setupWidgets()
{
    mHexDump = new MiniHexDump(mNavigation, GlobalArchitecture(), this);
    mCodeEditor = new CodeEditor(this);
    mCodeEditor->setFont(Config()->monospaceFont());
    mHighlighter = new PatternHighlighter(mCodeEditor, mCodeEditor->document());
    mLogBrowser = new QTextBrowser(this);
    mLogBrowser->setFont(Config()->monospaceFont());
    mLogBrowser->setOpenExternalLinks(true);
    mLogBrowser->setOpenLinks(false);
    connect(mLogBrowser, &QTextBrowser::anchorClicked, this, &MainWindow::onLogAnchorClicked);
    mStructWidget = new StructWidget(this);

    mDataTable = new DataTable(this);
    connect(mHexDump, &MiniHexDump::selectionUpdated, [this]()
    {
        mDataTable->selectionChanged(mHexDump->getSelectionStart(), mHexDump->getSelectionEnd());
    });

    auto hl = new QHBoxLayout();
    //hl->addSpacing()
    hl->addStretch(1);
    hl->addWidget(new QCheckBox("Auto"));
    auto runButton = new QPushButton("Run");
    connect(runButton, &QPushButton::pressed, this, &MainWindow::onButtonRunPressed);
    hl->addWidget(runButton);
    hl->setContentsMargins(4, 4, 4, 0);

    auto vl = new QVBoxLayout();
    vl->addWidget(mCodeEditor);
    vl->addLayout(hl);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    auto codeWidget = new QWidget();
    codeWidget->setLayout(vl);

    auto codeSplitter = new QSplitter(Qt::Vertical, this);
    codeSplitter->addWidget(codeWidget);
    codeSplitter->addWidget(mLogBrowser);
    codeSplitter->setStretchFactor(0, 80);
    codeSplitter->setStretchFactor(0, 20);

    mStructTabs = new QTabWidget(this);
    mStructTabs->addTab(mDataTable, "Data");
    mStructTabs->addTab(mStructWidget, "Struct");

    auto hexSplitter = new QSplitter(Qt::Vertical, this);
    hexSplitter->addWidget(mHexDump);
    hexSplitter->addWidget(mStructTabs);
    hexSplitter->setStretchFactor(0, 65);
    hexSplitter->setStretchFactor(1, 35);

    auto mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(hexSplitter);
    mainSplitter->addWidget(codeSplitter);
    mainSplitter->setStretchFactor(0, 58);
    mainSplitter->setStretchFactor(1, 42);

    setCentralWidget(mainSplitter);
    centralWidget()->setContentsMargins(3, 3, 3, 3);
}

void MainWindow::on_action_Load_file_triggered()
{
    // TODO: remember the previous browse directory
    auto fileName = QFileDialog::getOpenFileName(this, "Load file", QString(), "All files (*)");
    if(!fileName.isEmpty())
    {
        loadFile(fileName);
    }
}

void MainWindow::logHandler(LogLevel level, const char* message)
{
    Q_UNUSED(level);

    auto html = QString(message);
    html += "\n";
    html.replace("\t", "    ");
    html = html.toHtmlEscaped();
    html.replace(QChar(' '), QString("&nbsp;"));
    html.replace(QString("\r\n"), QString("<br/>\n"));
    html.replace(QChar('\n'), QString("<br/>\n"));

    static QRegularExpression addressRegExp(R"((&lt;source&gt;:)(\d+:\d+))");
    html.replace(addressRegExp, "<a href=\"navigate://localhost/#\\2\">\\1\\2</a>");

    QTextCursor cursor(mLogBrowser->document());
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();
    cursor.insertBlock();
    // hack to not insert too many newlines: https://lists.qt-project.org/pipermail/qt-interest-old/2011-July/034725.html
    cursor.deletePreviousChar();
    cursor.insertHtml(html);
    cursor.endEditBlock();
}

void MainWindow::compileError(const CompileError & error)
{
    Q_UNUSED(error);
    // TODO: highlight error lines
}

void MainWindow::evalError(const EvalError & error)
{
    auto errorLine = error.location.line;
    auto errorColumn = error.location.column;
    if(errorLine > 0)
    {
        mCodeEditor->setErrorLine(errorLine);

        auto cursor = mCodeEditor->textCursor();
        cursor.clearSelection();
        cursor.setPosition(mCodeEditor->document()->findBlockByLineNumber(errorLine - 1).position() + errorColumn - 1);
        mCodeEditor->setTextCursor(cursor);
    }
}

void MainWindow::onLogAnchorClicked(const QUrl & url)
{
    if(url.scheme() == "navigate")
    {
        auto fragment = url.fragment(QUrl::FullyDecoded);
        QStringList split = fragment.split(':');
        auto line = split[0].toInt();
        auto column = split[1].toInt();

        auto cursor = mCodeEditor->textCursor();
        cursor.clearSelection();
        cursor.setPosition(mCodeEditor->document()->findBlockByLineNumber(line - 1).position() + column - 1);
        mCodeEditor->setTextCursor(cursor);

        mCodeEditor->setFocus();
    }
}

void MainWindow::onButtonRunPressed()
{
    // TODO: change this?
    QString includeDir = QApplication::applicationDirPath();
    includeDir += "\\includes";
    auto includeUtf8 = includeDir.toUtf8();
    const char* includes[1] = {includeUtf8.constData()};

    auto source = mCodeEditor->toPlainText().toUtf8();

    PatternRunArgs args = {};
    args.userdata = this;
    args.root = nullptr;
    args.source = source.constData();
    args.filename = "<source>";
    args.base = 0x0;
#ifdef _WIN64
    args.size = 0x7FFFFFFFFFFFFFFF;
#else
    args.size = 0x7FFFFFFF;
#endif // _WIN64
    args.includes_count = std::size(includes);
    args.includes_data = includes;
    args.log_handler = [](void* userdata, LogLevel level, const char* message)
    {
        ((MainWindow*)userdata)->logHandler(level, message);
    };
    args.compile_error = [](void* userdata, const CompileError * error)
    {
        ((MainWindow*)userdata)->compileError(*error);
    };
    args.eval_error = [](void* userdata, const EvalError * error)
    {
        ((MainWindow*)userdata)->evalError(*error);
    };
    args.data_source = [](void*, uint64_t address, void* buffer, size_t size)
    {
        // TODO: page cache?
        return DbgMemRead(address, buffer, size);
    };
    args.visit = [](void* userdata, TreeNode parent, const VisitInfo * info) -> TreeNode
    {
        return ((MainWindow*)userdata)->mVisitor->visit(parent, *info);
    };
    // TODO: support removing type nodes from the API?
    // TODO: callback on remove of type node
    emit Bridge::getBridge()->typeClear();
    mVisitor->clear();
    mLogBrowser->clear();
    mCodeEditor->setErrorLine(-1);
    auto status = PatternRun(&args);
    emit Bridge::getBridge()->typeUpdateWidget();
    if(status == PatternSuccess)
    {
        mStructTabs->setCurrentIndex(1);
    }
}


