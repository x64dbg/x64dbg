#include "DataExplorerDialog.h"
#include "ui_DataExplorerDialog.h"
#include <QMessageBox>
#include <QDir>
#include <QTextBlock>
#include <QSettings>
#include "PatternHighlighter.h"
#include "Bridge.h"

#include <unordered_map>

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

        const auto& str = itr->second;
        if(*destCount <= str.second)
        {
            *destCount = str.second + 1;
            return false;
        }
        strcpy_s(dest, *destCount, mStringPool.data() + str.first);
        return true;
    }

    TreeNode visit(TreeNode parent, const VisitInfo& info)
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
        td.expanded = true;
        td.reverse = info.big_endian;
        td.name = name.c_str();
        td.addr = mCurrentBase;
        td.id = id;
        td.sizeBits = info.size * 8;
        // TODO: detect primitive types and exclude them from the toString callback
        td.offset = info.offset - mCurrentBase;
        td.callback = [](const TYPEDESCRIPTOR *type, char* dest, size_t* destCount)
        {
            return ((PatternVisitor*)type->userdata)->valueCallback(type, dest, destCount);
        };
        td.userdata = this;
        auto node = nullptr; // TODO
        //auto node = GuiTypeAddNode(parent, &td);
#ifdef _DEBUG
        _plugin_logprintf("[%p->%p] %s %s |%s| (address: 0x%llX, size: 0x%llX, line: %d)\n",
                          parent,
                          node,
                          info.type_name,
                          info.variable_name,
                          info.value,
                          info.offset,
                          info.size,
                          info.line
                          );
#endif
        return node;
    }

    void clear()
    {
        mCurrentBase = 0;
        mStringPool.clear();
        mNames.clear();
    }
};

DataExplorerDialog::DataExplorerDialog(QWidget* parent) : QDialog(parent), ui(new Ui::DataExplorerDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(size());

    ui->codeEdit->setFont(QFont("Courier New", 8));

    auto action = new QAction(ui->codeEdit);
    action->setShortcut(QKeySequence("Ctrl+R"));
    ui->codeEdit->addAction(action);
    connect(action, &QAction::triggered, ui->buttonParse, &QPushButton::click);

    mVisitor = new PatternVisitor;
    mHighlighter = new PatternHighlighter(ui->codeEdit, ui->codeEdit->document());

    // Restore settings
    {
        QSettings settings("DataExplorer");

        auto cursor = ui->codeEdit->textCursor();
        auto savedPosition = settings.value("cursor", cursor.position()).toInt();
        auto savedCode = settings.value("code", ui->codeEdit->toPlainText()).toString();
        ui->codeEdit->setPlainText(savedCode);
        cursor.setPosition(savedPosition);
        ui->codeEdit->setTextCursor(cursor);

        restoreGeometry(settings.value("geometry").toByteArray());
        if(settings.value("visible").toBool())
            show();
    }
}

DataExplorerDialog::~DataExplorerDialog()
{
    delete ui;
    delete mVisitor;
    delete mHighlighter;
}

void DataExplorerDialog::closeEvent(QCloseEvent* event)
{
    QSettings settings("DataExplorer");
    settings.setValue("code", ui->codeEdit->toPlainText());
    settings.setValue("cursor", ui->codeEdit->textCursor().position());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("visible", isVisible());

    QDialog::closeEvent(event);
}

void DataExplorerDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::StyleChange)
    {
        duint dark = 0;
        BridgeSettingGetUint("Colors", "DarkTitleBar", &dark);
        //_plugin_logprintf("[changeEvent] dark=%d\n", dark != 0);
        if (mHighlighter)
        {
            ensurePolished();
            mHighlighter->refreshColors(ui->codeEdit);
            mHighlighter->setDocument(nullptr);
            mHighlighter->setDocument(ui->codeEdit->document());
        }
    }
    QDialog::changeEvent(event);
}

void DataExplorerDialog::on_buttonParse_pressed()
{
    // TODO: change this?
    QString includeDir = QApplication::applicationDirPath();
    includeDir += "\\includes";
    auto includeUtf8 = includeDir.toUtf8();
    const char* includes[1] = {includeUtf8.constData()};

    auto source = ui->codeEdit->toPlainText().toUtf8();

    PatternRunArgs args = {};
    args.userdata = this;
    args.root = nullptr;
    args.source = source.constData();
    args.filename = "<source>";
    args.base = 0x10000;
#ifdef _WIN64
    args.size = 0x7FFFFFFFFFFFFFFF;
#else
    args.size = 0x7FFFFFFF;
#endif // _WIN64
    args.includes_count = std::size(includes);
    args.includes_data = includes;
    args.log_handler = [](void* userdata, LogLevel level, const char* message)
    {
        ((DataExplorerDialog*)userdata)->logHandler(level, message);
    };
    args.compile_error = [](void* userdata, const CompileError* error)
    {
        ((DataExplorerDialog*)userdata)->compileError(*error);
    };
    args.eval_error = [](void *userdata, const EvalError* error)
    {
        ((DataExplorerDialog*)userdata)->evalError(*error);
    };
    args.data_source = [](void*, uint64_t address, void* buffer, size_t size)
    {
        // TODO: page cache?
        return DbgMemRead(address, buffer, size);
    };
    args.visit = [](void *userdata, TreeNode parent, const VisitInfo *info) -> TreeNode
    {
        return ((DataExplorerDialog*)userdata)->mVisitor->visit(parent, *info);
    };
    // TODO: support removing type nodes from the API?
    // TODO: callback on remove of type node
    //GuiTypeClear();
    mVisitor->clear();
    ui->logEdit->clear();
    ui->codeEdit->setErrorLine(-1);
    auto status = PatternRun(&args);
    //GuiUpdateTypeWidget();
    if(status == PatternSuccess)
    {
        logHandler(LogLevelInfo, "Open the struct widget to see the results!\n");
    }
}

void DataExplorerDialog::logHandler(LogLevel level, const char* message)
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

    QTextCursor cursor(ui->logEdit->document());
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();
    cursor.insertBlock();
    // hack to not insert too many newlines: https://lists.qt-project.org/pipermail/qt-interest-old/2011-July/034725.html
    cursor.deletePreviousChar();
    cursor.insertHtml(html);
    cursor.endEditBlock();
}

void DataExplorerDialog::compileError(const CompileError& error)
{
    Q_UNUSED(error);
    // TODO: highlight error lines
}

void DataExplorerDialog::evalError(const EvalError& error)
{
    auto errorLine = error.location.line;
    auto errorColumn = error.location.column;
    if(errorLine > 0)
    {
        ui->codeEdit->setErrorLine(errorLine);

        auto cursor = ui->codeEdit->textCursor();
        cursor.clearSelection();
        cursor.setPosition(ui->codeEdit->document()->findBlockByLineNumber(errorLine - 1).position() + errorColumn - 1);
        ui->codeEdit->setTextCursor(cursor);
    }
}


void DataExplorerDialog::on_logEdit_anchorClicked(const QUrl &url)
{
    if(url.scheme() == "navigate")
    {
        auto fragment = url.fragment(QUrl::FullyDecoded);
        QStringList split = fragment.split(':');
        auto line = split[0].toInt();
        auto column = split[1].toInt();

        auto cursor = ui->codeEdit->textCursor();
        cursor.clearSelection();
        cursor.setPosition(ui->codeEdit->document()->findBlockByLineNumber(line - 1).position() + column - 1);
        ui->codeEdit->setTextCursor(cursor);

        ui->codeEdit->setFocus();
    }
}

