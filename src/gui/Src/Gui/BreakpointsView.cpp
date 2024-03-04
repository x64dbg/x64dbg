#include <QClipboard>
#include <QRegularExpression>
#include "BreakpointsView.h"
#include "EditBreakpointDialog.h"
#include "Bridge.h"
#include "MenuBuilder.h"
#include "Breakpoints.h"
#include "DisassemblyPopup.h"

BreakpointsView::BreakpointsView(QWidget* parent)
    : StdTable(parent), mExceptionMaxLength(0)
{
    auto charWidth = [this](int count)
    {
        return getCharWidth() * count + 8;
    };
    addColumnAt(charWidth(9), tr("Type"), false);
    addColumnAt(charWidth(sizeof(duint) * 2), tr("Address"), true, "", StdTable::SortBy::AsHex);
    addColumnAt(charWidth(35), tr("Module/Label/Exception"), true);
    addColumnAt(charWidth(8), tr("State"), true);
    addColumnAt(charWidth(50), tr("Disassembly"), true);
    addColumnAt(charWidth(4), tr("Hits"), true, "", StdTable::SortBy::AsInt);
    addColumnAt(0, tr("Summary"), true);
    loadColumnFromConfig("BreakpointsView");

    mDisasm = new QZydis(ConfigUint("Disassembler", "MaxModuleSize"), Bridge::getArchitecture());
    mDisasm->UpdateConfig();
    enableMultiSelection(true);

    setupContextMenu();

    connect(Bridge::getBridge(), SIGNAL(updateBreakpoints()), this, SLOT(updateBreakpointsSlot()));
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(duint, duint)), this, SLOT(disassembleAtSlot(duint, duint)));
    connect(Config(), SIGNAL(tokenizerConfigUpdated()), this, SLOT(tokenizerConfigUpdatedSlot()));

    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(followBreakpointSlot()));
    connect(this, SIGNAL(enterPressedSignal()), this, SLOT(followBreakpointSlot()));

    Initialize();

    new DisassemblyPopup(this, Bridge::getArchitecture());
}

void BreakpointsView::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this, [](QMenu*)
    {
        return DbgIsDebugging();
    });

    auto validBp = [this](QMenu*)
    {
        return isValidBp();
    };
    mMenuBuilder->addAction(makeAction(DIcon(ArchValue("processor32", "processor64")), tr("Follow breakpoint"), SLOT(followBreakpointSlot())), [this](QMenu*)
    {
        if(!isValidBp())
            return false;
        if(selectedBp().type == bp_exception)
            return false;
        return true;
    });
    mMenuBuilder->addAction(makeShortcutAction(DIcon("breakpoint_remove"), tr("&Remove"), SLOT(removeBreakpointSlot()), "ActionDeleteBreakpoint"), validBp);
    QAction* enableDisableBreakpoint = makeShortcutAction(DIcon("breakpoint_disable"), tr("Disable"), SLOT(toggleBreakpointSlot()), "ActionEnableDisableBreakpoint");
    mMenuBuilder->addAction(enableDisableBreakpoint, [this, enableDisableBreakpoint](QMenu*)
    {
        if(!isValidBp() || !selectedBp().active)
            return false;
        if(selectedBp().enabled)
        {
            enableDisableBreakpoint->setIcon(DIcon("breakpoint_disable"));
            enableDisableBreakpoint->setText(tr("Disable"));
        }
        else
        {
            enableDisableBreakpoint->setIcon(DIcon("breakpoint_enable"));
            enableDisableBreakpoint->setText(tr("Enable"));
        }
        return true;
    });
    mMenuBuilder->addAction(makeShortcutAction(DIcon("breakpoint_edit_alt"), tr("&Edit"), SLOT(editBreakpointSlot()), "ActionEditBreakpoint"), validBp);
    mMenuBuilder->addAction(makeShortcutAction(DIcon("breakpoint_reset_hitcount"), tr("Reset hit count"), SLOT(resetHitCountBreakpointSlot()), "ActionResetHitCountBreakpoint"), [this](QMenu*)
    {
        if(!isValidBp())
            return false;
        return selectedBp().hitCount > 0;
    });
    mMenuBuilder->addSeparator();

    QAction* enableAll = makeShortcutAction(DIcon("breakpoint_enable_all"), QString(), SLOT(enableAllBreakpointsSlot()), "ActionEnableAllBreakpoints");
    mMenuBuilder->addAction(enableAll, [this, enableAll](QMenu*)
    {
        if(!isValidBp())
            return false;
        enableAll->setText(tr("Enable all (%1)").arg(bpTypeName(selectedBp().type)));
        return true;
    });
    QAction* disableAll = makeShortcutAction(DIcon("breakpoint_disable_all"), QString(), SLOT(disableAllBreakpointsSlot()), "ActionDisableAllBreakpoints");
    mMenuBuilder->addAction(disableAll, [this, disableAll](QMenu*)
    {
        if(!isValidBp())
            return false;
        disableAll->setText(tr("Disable all (%1)").arg(bpTypeName(selectedBp().type)));
        return true;
    });
    QAction* removeAll = makeShortcutAction(DIcon("breakpoint_remove_all"), QString(), SLOT(removeAllBreakpointsSlot()), "ActionRemoveAllBreakpoints");
    mMenuBuilder->addAction(removeAll, [this, removeAll](QMenu*)
    {
        if(!isValidBp())
            return false;
        removeAll->setText(tr("Remove all (%1)").arg(bpTypeName(selectedBp().type)));
        return true;
    });
    mMenuBuilder->addSeparator();

    mMenuBuilder->addAction(makeAction(DIcon("breakpoint_module_add"), tr("Add DLL breakpoint"), SLOT(addDllBreakpointSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("breakpoint_exception_add"), tr("Add exception breakpoint"), SLOT(addExceptionBreakpointSlot())));
    mMenuBuilder->addSeparator();

    mMenuBuilder->addAction(makeAction(tr("Copy breakpoint conditions"), SLOT(copyConditionalBreakpointSlot())));
    mMenuBuilder->addAction(makeAction(tr("Paste breakpoint conditions"), SLOT(pasteConditionalBreakpointSlot())), [](QMenu*)
    {
        return QApplication::clipboard()->text().size() > 10;
    });
    MenuBuilder* copyMenu = new MenuBuilder(this);
    setupCopyMenu(copyMenu);
    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), copyMenu);
}

void BreakpointsView::updateColors()
{
    StdTable::updateColors();
    mDisasmBackgroundColor = ConfigColor("DisassemblyBackgroundColor");
    mDisasmSelectionColor = ConfigColor("DisassemblySelectionColor");
    mCipBackgroundColor = ConfigColor("ThreadCurrentBackgroundColor");
    mCipColor = ConfigColor("ThreadCurrentColor");
    mSummaryParenColor = ConfigColor("BreakpointSummaryParenColor");
    mSummaryKeywordColor = ConfigColor("BreakpointSummaryKeywordColor");
    mSummaryStringColor = ConfigColor("BreakpointSummaryStringColor");
    mDisasm->UpdateConfig();
    updateBreakpointsSlot();
}

void BreakpointsView::sortRows(duint column, bool ascending)
{
    std::stable_sort(mData.begin(), mData.end(), [this, column, ascending](const std::vector<CellData> & a, const std::vector<CellData> & b)
    {
        //this function sorts on header type first and then on column content
        auto aBp = &mBps.at(a.at(ColAddr).userdata), bBp = &mBps.at(b.at(ColAddr).userdata);
        auto aType = aBp->type, bType = bBp->type;
        auto aHeader = aBp->addr || aBp->active, bHeader = bBp->addr || bBp->active;
        struct Hax
        {
            const bool & greater;
            const QString & s;
            Hax(const bool & greater, const QString & s) : greater(greater), s(s) { }
            bool operator<(const Hax & b)
            {
                return greater ? s > b.s : s < b.s;
            }
        } aHax(!ascending, a.at(column).text), bHax(!ascending, b.at(column).text);
        return std::tie(aType, aHeader, aHax) < std::tie(bType, bHeader, bHax);
    });
}

QString BreakpointsView::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    if(isSelected(row))
        painter->fillRect(QRect(x, y, w, h), QBrush(col == ColDisasm ? mDisasmSelectionColor : mSelectionColor));
    else if(col == ColDisasm)
        painter->fillRect(QRect(x, y, w, h), QBrush(mDisasmBackgroundColor));
    auto index = bpIndex(row);
    auto & bp = mBps.at(index);
    auto cellContent = getCellContent(row, col);
    if(col > ColType && bp.addr == 0 && !bp.active)
    {
        auto mid = h / 2.0;
        painter->drawLine(QPointF(x, y + mid), QPointF(x + w, y + mid));
    }
    else if(col == ColAddr)
    {
        if(bp.type == bp_dll || bp.type == bp_exception)
            return cellContent;
        else if(bp.addr && bp.addr == mCip)
        {
            painter->fillRect(QRect(x, y, w, h), QBrush(mCipBackgroundColor));
            painter->setPen(QPen(mCipColor));
            painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, cellContent);
            return QString();
        }
    }
    else if(col == ColDisasm)
    {
        RichTextPainter::paintRichText(painter, x + 4, y, w - 4, h, 0, mRich.at(index).first, mFontMetrics);
        return QString();
    }
    else if(col == ColSummary)
    {
        RichTextPainter::paintRichText(painter, x + 4, y, w - 4, h, 0, mRich.at(index).second, mFontMetrics);
        return QString();
    }
    return cellContent;
}

void BreakpointsView::updateBreakpointsSlot()
{
    if(mExceptionMap.empty() && DbgFunctions()->EnumExceptions)
    {
        BridgeList<CONSTANTINFO> exceptions;
        DbgFunctions()->EnumExceptions(&exceptions);
        for(int i = 0; i < exceptions.Count(); i++)
        {
            mExceptionMap.insert({exceptions[i].value, exceptions[i].name});
            mExceptionList.append(QString(exceptions[i].name));
            mExceptionMaxLength = std::max(mExceptionMaxLength, int(strlen(exceptions[i].name)));
        }
        mExceptionList.sort();
    }

    if(DbgFunctions()->BpRefList == nullptr)
        return;

    duint count = 0;
    auto refList = DbgFunctions()->BpRefList(&count);
    setRowCount(count);
    mBps.clear();
    mBps.reserve(count + 5);
    mRich.clear();
    mRich.reserve(count + 5);
    BPXTYPE lasttype = bp_none;
    for(duint i = 0, row = 0; i < count; i++, row++)
    {
        Breakpoints::Data bp(refList[i]);

        if(lasttype != bp.type)
        {
            lasttype = bp.type;
            setRowCount(getRowCount() + 1);
            setCellContent(row, ColType, bpTypeName(bp.type));
            setCellUserdata(row, ColType, bp.type);
            setCellContent(row, ColHits, QString());
            setCellContent(row, ColAddr, QString());
            setCellUserdata(row, ColAddr, row);
            setCellContent(row, ColModLabel, QString());
            setCellContent(row, ColState, QString());
            setCellContent(row, ColDisasm, QString());
            setCellContent(row, ColSummary, QString());
            row++;

            Breakpoints::Data fakebp;
            fakebp.type = lasttype;
            mBps.push_back(fakebp);
            mRich.push_back(std::make_pair(RichTextPainter::List(), RichTextPainter::List()));
        }

        mBps.push_back(bp);

        RichTextPainter::List richSummary, richDisasm;

        auto addrText = [&]()
        {
            if(bp.type == bp_dll)
            {
                auto base = DbgModBaseFromName(bp.module.toUtf8().constData());
                if(!base)
                    base = -1;
                return ToPtrString(base);
            }
            else
                return ToPtrString(bp.addr);
        };
        auto modLabelText = [&]() -> QString
        {
            char label[MAX_LABEL_SIZE] = "";
            if(bp.type == bp_exception)
            {
                auto found = mExceptionMap.find(bp.addr);
                return found == mExceptionMap.end() ? "" : found->second;
            }
            else if(bp.type != bp_dll && DbgGetLabelAt(bp.addr, SEG_DEFAULT, label))
                return QString("<%1.%2>").arg(bp.module, label);
            else
                return bp.module;
        };
        auto stateName = [&]()
        {
            if(!bp.active)
                return tr("Inactive");
            if(bp.enabled)
                return bp.singleshoot ? tr("One-time") : tr("Enabled");
            else
                return tr("Disabled");
        };
        auto disasmText = [&]() -> QString
        {
            QString result;
            if(!bp.active || bp.type == bp_dll || bp.type == bp_exception)
                return result;
            byte_t data[MAX_DISASM_BUFFER];
            if(DbgMemRead(bp.addr, data, sizeof(data)))
            {
                auto instr = mDisasm->DisassembleAt(data, sizeof(data), 0, bp.addr);
                ZydisTokenizer::TokenToRichText(instr.tokens, richDisasm, 0);
                for(auto & token : richDisasm)
                    result += token.text;
            }
            else
            {
                RichTextPainter::CustomRichText_t err;
                err.text = "Failed to read: " + ToPtrString(bp.addr);
                richDisasm.push_back(err);
                return err.text;
            }
            return result;
        };
        //memory/hardware/dll/exception type, name, address comment, condition, log(text+condition), command(text+condition), logFile(path)
        auto summaryText = [&]()
        {
            auto colored = [&richSummary](QString text, QColor color)
            {
                RichTextPainter::CustomRichText_t token;
                token.underline = false;
                token.flags = RichTextPainter::FlagColor;
                token.textColor = color;
                token.text = text;
                richSummary.push_back(token);
            };
            auto text = [this, &richSummary](QString text)
            {
                RichTextPainter::CustomRichText_t token;
                token.underline = false;
                token.flags = RichTextPainter::FlagColor;
                token.textColor = this->mTextColor;
                token.text = text;
                richSummary.push_back(token);
            };
            auto next = [&richSummary, &text]()
            {
                if(!richSummary.empty())
                    text(", ");
            };

            char comment[MAX_COMMENT_SIZE];
            if(bp.type != bp_dll && bp.type != bp_exception && DbgGetCommentAt(bp.addr, comment) && *comment != '\1')
            {
                next();
                colored(comment, mSummaryStringColor);
            }
            else if(!bp.name.isEmpty())
            {
                next();
                colored(bp.name, mSummaryStringColor);
            }

            switch(bp.type)
            {
            case bp_normal:
                break;

            case bp_hardware:
            {
                auto size = [](BPHWSIZE size)
                {
                    switch(size)
                    {
                    case hw_byte:
                        return tr("byte");
                    case hw_word:
                        return tr("word");
                    case hw_dword:
                        return tr("dword");
                    case hw_qword:
                        return tr("qword");
                    default:
                        return QString();
                    }
                }(BPHWSIZE(bp.hwSize));

                switch(bp.typeEx)
                {
                case hw_access:
                    next();
                    colored(tr("access"), mSummaryKeywordColor);
                    colored("(", mSummaryParenColor);
                    text(size);
                    colored(")", mSummaryParenColor);
                    break;
                case hw_write:
                    next();
                    colored(tr("write"), mSummaryKeywordColor);
                    colored("(", mSummaryParenColor);
                    text(size);
                    colored(")", mSummaryParenColor);
                    break;
                case hw_execute:
                    next();
                    colored(tr("execute"), mSummaryKeywordColor);
                    colored("()", mSummaryParenColor);
                    break;
                }
            }
            break;

            case bp_memory:
            {
                auto op = [](BPMEMTYPE type)
                {
                    switch(type)
                    {
                    case mem_access:
                        return tr("access");
                    case mem_read:
                        return tr("read");
                    case mem_write:
                        return tr("write");
                    case mem_execute:
                        return tr("execute");
                    default:
                        return QString();
                    }
                }(BPMEMTYPE(bp.typeEx));
                next();
                colored(op, mSummaryKeywordColor);
                colored("(", mSummaryParenColor);
                text(ToHexString(DbgFunctions()->MemBpSize(bp.addr)));
                colored(")", mSummaryParenColor);
            }
            break;

            case bp_dll:
                switch(bp.typeEx)
                {
                case dll_load:
                    next();
                    colored(tr("load"), mSummaryKeywordColor);
                    colored("()", mSummaryParenColor);
                    break;
                case dll_unload:
                    next();
                    colored(tr("unload"), mSummaryKeywordColor);
                    colored("()", mSummaryParenColor);
                    break;
                case dll_all:
                    next();
                    colored(tr("all"), mSummaryKeywordColor);
                    colored("()", mSummaryParenColor);
                    break;
                }
                break;

            case bp_exception:
                switch(bp.typeEx)
                {
                case ex_firstchance:
                    next();
                    colored(tr("firstchance"), mSummaryKeywordColor);
                    colored("()", mSummaryParenColor);
                    break;
                case ex_secondchance:
                    next();
                    colored(tr("secondchance"), mSummaryKeywordColor);
                    colored("()", mSummaryParenColor);
                    break;
                case ex_all:
                    next();
                    colored(tr("anychance"), mSummaryKeywordColor);
                    colored("()", mSummaryParenColor);
                    break;
                }
                break;

            default:
                return QString();
            }

            if(!bp.breakCondition.isEmpty())
            {
                next();
                colored(tr("breakif"), mSummaryKeywordColor);
                colored("(", mSummaryParenColor);
                text(bp.breakCondition);
                colored(")", mSummaryParenColor);
            }

            if(bp.fastResume)
            {
                next();
                colored(tr("fastresume"), mSummaryKeywordColor);
                colored("()", mSummaryParenColor);
            }
            else //fast resume skips all other steps
            {
                if(!bp.logText.isEmpty())
                {
                    next();
                    if(!bp.logCondition.isEmpty())
                    {
                        colored(tr("logif"), mSummaryKeywordColor);
                        colored("(", mSummaryParenColor);
                        text(bp.logCondition);
                        colored(",", mSummaryParenColor);
                        text(" ");
                    }
                    else
                    {
                        colored(tr("log"), mSummaryKeywordColor);
                        colored("(", mSummaryParenColor);
                    }
                    colored(QString("\"%1\"").arg(bp.logText), mSummaryStringColor);
                    if(!bp.logFile.isEmpty())
                    {
                        colored(", ", mSummaryParenColor);
                        colored("file", mSummaryKeywordColor);
                        colored("(", mSummaryParenColor);
                        colored(bp.logFile, mSummaryStringColor);
                        colored(")", mSummaryParenColor);
                    }
                    colored(")", mSummaryParenColor);
                }

                if(!bp.commandText.isEmpty())
                {
                    next();
                    if(!bp.commandCondition.isEmpty())
                    {
                        colored(tr("cmdif"), mSummaryKeywordColor);
                        colored("(", mSummaryParenColor);
                        text(bp.commandCondition);
                        colored(",", mSummaryParenColor);
                        text(" ");
                    }
                    else
                    {
                        colored(tr("cmd"), mSummaryKeywordColor);
                        colored("(", mSummaryParenColor);
                    }
                    colored(QString("\"%1\"").arg(bp.commandText), mSummaryStringColor);
                    colored(")", mSummaryParenColor);
                }
            }
            QString result;
            for(auto & token : richSummary)
                result += token.text;
            return result;
        };

        setCellContent(row, ColType, QString());
        setCellUserdata(row, ColType, bp.type);
        setCellContent(row, ColAddr, addrText());
        setCellUserdata(row, ColAddr, row);
        setCellContent(row, ColModLabel, modLabelText());
        setCellContent(row, ColState, stateName());
        setCellContent(row, ColDisasm, disasmText());
        setCellContent(row, ColHits, QString("%1").arg(bp.hitCount));
        setCellContent(row, ColSummary, summaryText());

        mRich.push_back(std::make_pair(std::move(richDisasm), std::move(richSummary)));
    }
    BridgeFree(refList);
    if(count)
    {
        auto sel = getInitialSelection();
        auto rows = getRowCount();
        if(sel >= rows)
            setSingleSelection(rows - 1);
    }
    reloadData();
}

void BreakpointsView::disassembleAtSlot(duint addr, duint cip)
{
    Q_UNUSED(addr);
    mCip = cip;
}

void BreakpointsView::tokenizerConfigUpdatedSlot()
{
    mDisasm->UpdateConfig();
    updateBreakpointsSlot();
}

void BreakpointsView::contextMenuSlot(const QPoint & pos)
{
    QMenu menu(this);
    mMenuBuilder->build(&menu);
    if(!menu.actions().isEmpty())
        menu.exec(mapToGlobal(pos));
}

void BreakpointsView::followBreakpointSlot()
{
    if(!isValidBp())
        return;
    auto & bp = selectedBp();
    if(bp.type == bp_exception || !bp.active)
    {
        GuiAddStatusBarMessage(tr("Cannot follow this breakpoint.\n").toUtf8().constData());
        return;
    }
    duint addr = bp.type == bp_dll ? DbgModBaseFromName(bp.module.toUtf8().constData()) : bp.addr;
    if(!DbgMemIsValidReadPtr(addr))
    {
        GuiAddStatusBarMessage(tr("Cannot follow this breakpoint.\n").toUtf8().constData());
        return;
    }
    if(DbgFunctions()->MemIsCodePage(addr, false))
        DbgCmdExecDirect(QString("disasm %1").arg(ToPtrString(addr)));
    else
    {
        DbgCmdExecDirect(QString("dump %1").arg(ToPtrString(addr)));
        emit Bridge::getBridge()->getDumpAttention();
    }
}

void BreakpointsView::removeBreakpointSlot()
{
    GuiDisableUpdateScope s;
    for(int i : getSelection())
    {
        if(isValidBp(i))
        {
            const auto & bp = selectedBp(i);
            Breakpoints::removeBP(bp);
        }
    }
}

void BreakpointsView::toggleBreakpointSlot()
{
    GuiDisableUpdateScope s;
    for(int i : getSelection())
        if(isValidBp(i) && selectedBp(i).active)
            Breakpoints::toggleBPByDisabling(selectedBp(i));
}

void BreakpointsView::editBreakpointSlot()
{
    if(!isValidBp())
        return;
    const auto & bp = selectedBp();
    if(bp.type == bp_dll)
    {
        Breakpoints::editBP(bp_dll, bp.module, this);
    }
    else if(bp.active || bp.type == bp_exception)
    {
        Breakpoints::editBP(bp.type, ToPtrString(bp.addr), this);
    }
    else
    {
        QString addrText = QString().sprintf("\"%s\":$%X", bp.module, bp.addr);
        EditBreakpointDialog dialog(this, bp);
        if(dialog.exec() != QDialog::Accepted)
            return;
        auto exec = [](const QString & command)
        {
            DbgCmdExecDirect(command);
        };
        const auto & newBp = dialog.getBp();
        switch(bp.type)
        {
        case bp_normal:
            exec(QString("SetBreakpointName %1, \"%2\"").arg(addrText).arg(newBp.name));
            exec(QString("SetBreakpointCondition %1, \"%2\"").arg(addrText).arg(newBp.breakCondition));
            exec(QString("SetBreakpointLog %1, \"%2\"").arg(addrText).arg(newBp.logText));
            exec(QString("SetBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(newBp.logCondition));
            exec(QString("SetBreakpointCommand %1, \"%2\"").arg(addrText).arg(newBp.commandText));
            exec(QString("SetBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(newBp.commandCondition));
            exec(QString("ResetBreakpointHitCount %1, %2").arg(addrText).arg(ToPtrString(newBp.hitCount)));
            exec(QString("SetBreakpointFastResume %1, %2").arg(addrText).arg(newBp.fastResume));
            exec(QString("SetBreakpointSilent %1, %2").arg(addrText).arg(newBp.silent));
            exec(QString("SetBreakpointSingleshoot %1, %2").arg(addrText).arg(newBp.singleshoot));
            break;
        case bp_hardware:
            exec(QString("SetHardwareBreakpointName %1, \"%2\"").arg(addrText).arg(newBp.name));
            exec(QString("SetHardwareBreakpointCondition %1, \"%2\"").arg(addrText).arg(newBp.breakCondition));
            exec(QString("SetHardwareBreakpointLog %1, \"%2\"").arg(addrText).arg(newBp.logText));
            exec(QString("SetHardwareBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(newBp.logCondition));
            exec(QString("SetHardwareBreakpointCommand %1, \"%2\"").arg(addrText).arg(newBp.commandText));
            exec(QString("SetHardwareBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(newBp.commandCondition));
            exec(QString("ResetHardwareBreakpointHitCount %1, %2").arg(addrText).arg(ToPtrString(newBp.hitCount)));
            exec(QString("SetHardwareBreakpointFastResume %1, %2").arg(addrText).arg(newBp.fastResume));
            exec(QString("SetHardwareBreakpointSilent %1, %2").arg(addrText).arg(newBp.silent));
            exec(QString("SetHardwareBreakpointSingleshoot %1, %2").arg(addrText).arg(newBp.singleshoot));
            break;
        case bp_memory:
            exec(QString("SetMemoryBreakpointName %1, \"\"%2\"\"").arg(addrText).arg(newBp.name));
            exec(QString("SetMemoryBreakpointCondition %1, \"%2\"").arg(addrText).arg(newBp.breakCondition));
            exec(QString("SetMemoryBreakpointLog %1, \"%2\"").arg(addrText).arg(newBp.logText));
            exec(QString("SetMemoryBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(newBp.logCondition));
            exec(QString("SetMemoryBreakpointCommand %1, \"%2\"").arg(addrText).arg(newBp.commandText));
            exec(QString("SetMemoryBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(newBp.commandCondition));
            exec(QString("ResetMemoryBreakpointHitCount %1, %2").arg(addrText).arg(ToPtrString(newBp.hitCount)));
            exec(QString("SetMemoryBreakpointFastResume %1, %2").arg(addrText).arg(newBp.fastResume));
            exec(QString("SetMemoryBreakpointSilent %1, %2").arg(addrText).arg(newBp.silent));
            exec(QString("SetMemoryBreakpointSingleshoot %1, %2").arg(addrText).arg(newBp.singleshoot));
            break;
        default:
            break;
        }
    }
}

void BreakpointsView::resetHitCountBreakpointSlot()
{
    GuiDisableUpdateScope s;
    for(int i : getSelection())
    {
        if(!isValidBp(i))
            continue;
        auto & bp = selectedBp(i);
        DbgCmdExec([&bp]()
        {
            switch(bp.type)
            {
            case bp_normal:
                return QString("ResetBreakpointHitCount %1").arg(ToPtrString(bp.addr));
            case bp_hardware:
                return QString("ResetHardwareBreakpointHitCount %1").arg(ToPtrString(bp.addr));
            case bp_memory:
                return QString("ResetMemoryBreakpointHitCount %1").arg(ToPtrString(bp.addr));
            case bp_dll:
                return QString("ResetLibrarianBreakpointHitCount \"%1\"").arg(bp.module);
            case bp_exception:
                return QString("ResetExceptionBreakpointHitCount %1").arg(ToHexString(bp.addr));
            default:
                return QString("invalid");
            }
        }());
        QString cmd;

        DbgCmdExecDirect(cmd);
    }
}

void BreakpointsView::enableAllBreakpointsSlot()
{
    if(mBps.empty())
        return;
    DbgCmdExec([this]()
    {
        switch(selectedBp().type)
        {
        case bp_normal:
            return "bpe";
        case bp_hardware:
            return "bphwe";
        case bp_memory:
            return "bpme";
        case bp_dll:
            return "bpdll";
        case bp_exception:
            return "EnableExceptionBPX";
        default:
            return "invalid";
        }
    }());
}

void BreakpointsView::disableAllBreakpointsSlot()
{
    if(mBps.empty())
        return;
    DbgCmdExec([this]()
    {
        switch(selectedBp().type)
        {
        case bp_normal:
            return "bpd";
        case bp_hardware:
            return "bphwd";
        case bp_memory:
            return "bpmd";
        case bp_dll:
            return "bpddll";
        case bp_exception:
            return "DisableExceptionBPX";
        default:
            return "invalid";
        }
    }());
}

void BreakpointsView::removeAllBreakpointsSlot()
{
    if(mBps.empty())
        return;
    DbgCmdExec([this]()
    {
        switch(selectedBp().type)
        {
        case bp_normal:
            return "bc";
        case bp_hardware:
            return "bphwc";
        case bp_memory:
            return "bpmc";
        case bp_dll:
            return "bcdll";
        case bp_exception:
            return "DeleteExceptionBPX";
        default:
            return "invalid";
        }
    }());
}

void BreakpointsView::addDllBreakpointSlot()
{
    QString fileName;
    if(SimpleInputBox(this, tr("Enter the module name"), "", fileName, tr("Example: mydll.dll"), &DIcon("breakpoint")) && !fileName.isEmpty())
        DbgCmdExec(QString("bpdll \"%1\"").arg(fileName));
}

void BreakpointsView::addExceptionBreakpointSlot()
{
    QString exception;
    if(SimpleChoiceBox(this, tr("Enter the exception code"), "", mExceptionList, exception, true, tr("Example: EXCEPTION_ACCESS_VIOLATION"), &DIcon("breakpoint"), mExceptionMaxLength) && !exception.isEmpty())
        DbgCmdExec((QString("SetExceptionBPX ") + exception));
}

static QString escape(QString data)
{
    return DbgCmdEscape(std::move(data));
}

void BreakpointsView::copyConditionalBreakpointSlot()
{
    const auto & bp = selectedBp();
    const char* bpcnd;
    const char* bplog;
    const char* bpcmd;
    const char* bplogcnd;
    const char* bpcmdcnd;
    const char* bpfastresume;
    const char* bpsilent;
    const char* bpsingleshoot;
    switch(bp.type)
    {
    case bp_normal:
        bpcnd = "bpcnd %1, \"%2\"";
        bplog = "bpl %1, \"%2\"";
        bpcmd = "SetBreakpointCommand %1, \"%2\"";
        bplogcnd = "bplogcondition %1, \"%2\"";
        bpcmdcnd = "SetBreakpointCommandCondition %1, \"%2\"";
        bpfastresume = "SetBreakpointFastResume ";
        bpsilent = "SetBreakpointSilent ";
        bpsingleshoot = "SetBreakpointSingleshoot ";
        break;
    case bp_hardware:
        bpcnd = "bphwcond %1, \"%2\"";
        bplog = "bphwlog %1, \"%2\"";
        bpcmd = "SetHardwareBreakpointCommand %1, \"%2\"";
        bplogcnd = "bphwlogcondition %1, \"%2\"";
        bpcmdcnd = "SetHardwareBreakpointCommandCondition %1, \"%2\"";
        bpfastresume = "SetHardwareBreakpointFastResume ";
        bpsilent = "SetHardwareBreakpointSilent ";
        bpsingleshoot = "SetHardwareBreakpointSingleshoot ";
        break;
    case bp_memory:
        bpcnd = "bpmcond %1, \"%2\"";
        bplog = "bpml %1, \"%2\"";
        bpcmd = "SetMemoryBreakpointCommand %1, \"%2\"";
        bplogcnd = "bpmlogcondition %1, \"%2\"";
        bpcmdcnd = "SetMemoryBreakpointCommandCondition %1, \"%2\"";
        bpfastresume = "SetMemoryBreakpointFastResume ";
        bpsilent = "SetMemoryBreakpointSilent ";
        bpsingleshoot = "SetMemoryBreakpointSingleshoot ";
        break;
    case bp_dll:
        bpcnd = "SetLibrarianBreakpointCondition %1, \"%2\"";
        bplog = "SetLibrarianBreakpointLog %1, \"%2\"";
        bpcmd = "SetLibrarianBreakpointCommand %1, \"%2\"";
        bplogcnd = "SetLibrarianBreakpointLogCondition %1, \"%2\"";
        bpcmdcnd = "SetLibrarianBreakpointCommandCondition %1, \"%2\"";
        bpfastresume = "SetLibrarianBreakpointFastResume ";
        bpsilent = "SetLibrarianBreakpointSilent ";
        bpsingleshoot = "SetLibrarianBreakpointSingleshoot ";
        break;
    case bp_exception:
        bpcnd = "SetExceptionBreakpointCondition %1, \"%2\"";
        bplog = "SetExceptionBreakpointLog %1, \"%2\"";
        bpcmd = "SetExceptionBreakpointCommand %1, \"%2\"";
        bplogcnd = "SetExceptionBreakpointLogCondition %1, \"%2\"";
        bpcmdcnd = "SetExceptionBreakpointCommandCondition %1, \"%2\"";
        bpfastresume = "SetExceptionBreakpointFastResume ";
        bpsilent = "SetExceptionBreakpointSilent ";
        bpsingleshoot = "SetExceptionBreakpointSingleshoot ";
        break;
    default:
        return;
    }
    QString text;
    QString addr;
    if(bp.type != bp_dll)
        addr = ToPtrString(bp.addr);
    else
        addr = '"' + escape(bp.module) + '"';
    QTextStream s(&text, QIODevice::WriteOnly);
    s << QString(bpcnd).arg(addr).arg(escape(bp.breakCondition)) << "\r\n";
    s << QString(bplog).arg(addr).arg(escape(bp.logText)) << "\r\n";
    s << QString(bplogcnd).arg(addr).arg(escape(bp.logCondition)) << "\r\n";
    s << QString(bpcmd).arg(addr).arg(escape(bp.commandText)) << "\r\n";
    s << QString(bpcmdcnd).arg(addr).arg(escape(bp.commandCondition)) << "\r\n";
    addr += ", ";
    s << bpfastresume << addr << (bp.fastResume ? '1' : '0') << "\r\n";
    s << bpsilent << addr << (bp.silent ? '1' : '0') << "\r\n";
    s << bpsingleshoot << addr << (bp.singleshoot ? '1' : '0') << "\r\n";
    Bridge::CopyToClipboard(text);
}

void BreakpointsView::pasteConditionalBreakpointSlot()
{
    //TO DO perform a validation
    QClipboard* clipboard = QApplication::clipboard();
    QString text = clipboard->text();
    QRegExp regexp(ArchValue("(\\w+) ([\\dA-F]{8}),", "(\\w+) ([\\dA-F]{16}),"), Qt::CaseInsensitive);

    GuiDisableUpdateScope s;
    for(int i : getSelection())
    {
        if(!isValidBp(i))
            continue;
        auto & bp = selectedBp(i);
        QString text1 = text.replace(regexp, "\\1 " + ToPtrString(bp.addr) + ",");
        QList<QString> cmds;
        cmds = text1.split("\r\n");
        for(const auto & j : cmds)
            DbgCmdExecDirect(j.toUtf8().constData());
    }
}
