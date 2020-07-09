#include "TraceBrowser.h"
#include "TraceFileReader.h"
#include "TraceFileSearch.h"
#include "RichTextPainter.h"
#include "main.h"
#include "BrowseDialog.h"
#include "QBeaEngine.h"
#include "GotoDialog.h"
#include "LineEditDialog.h"
#include "WordEditDialog.h"
#include "CachedFontMetrics.h"
#include "BreakpointMenu.h"
#include "MRUList.h"
#include <QFileDialog>

TraceBrowser::TraceBrowser(QWidget* parent) : AbstractTableView(parent)
{
    mTraceFile = nullptr;
    addColumnAt(getCharWidth() * 2 * 8 + 8, "", false); //index
    addColumnAt(getCharWidth() * 2 * sizeof(dsint) + 8, "", false); //address
    addColumnAt(getCharWidth() * 2 * 12 + 8, "", false); //bytes
    addColumnAt(getCharWidth() * 40, "", false); //disassembly
    addColumnAt(getCharWidth() * 50, "", false); //registers
    addColumnAt(getCharWidth() * 50, "", false); //memory
    addColumnAt(1000, "", false); //comments

    setShowHeader(false); //hide header

    mSelection.firstSelectedIndex = 0;
    mSelection.fromIndex = 0;
    mSelection.toIndex = 0;
    setRowCount(0);
    mRvaDisplayBase = 0;
    mRvaDisplayEnabled = false;

    mAutoDisassemblyFollowSelection = false;

    int maxModuleSize = (int)ConfigUint("Disassembler", "MaxModuleSize");
    mDisasm = new QBeaEngine(maxModuleSize);
    mHighlightingMode = false;
    mPermanentHighlightingMode = false;

    mMRUList = new MRUList(this, "Recent Trace Files");
    connect(mMRUList, SIGNAL(openFile(QString)), this, SLOT(openSlot(QString)));
    mMRUList->load();

    setupRightClickContextMenu();

    Initialize();

    connect(Bridge::getBridge(), SIGNAL(updateTraceBrowser()), this, SLOT(updateSlot()));
    connect(Bridge::getBridge(), SIGNAL(openTraceFile(const QString &)), this, SLOT(openSlot(const QString &)));
    connect(Config(), SIGNAL(tokenizerConfigUpdated()), this, SLOT(tokenizerConfigUpdatedSlot()));
}

TraceBrowser::~TraceBrowser()
{
    delete mDisasm;
}

QString TraceBrowser::getAddrText(dsint cur_addr, char label[MAX_LABEL_SIZE], bool getLabel)
{
    QString addrText = "";
    if(mRvaDisplayEnabled) //RVA display
    {
        dsint rva = cur_addr - mRvaDisplayBase;
        if(rva == 0)
        {
#ifdef _WIN64
            addrText = "$ ==>            ";
#else
            addrText = "$ ==>    ";
#endif //_WIN64
        }
        else if(rva > 0)
        {
#ifdef _WIN64
            addrText = "$+" + QString("%1").arg(rva, -15, 16, QChar(' ')).toUpper();
#else
            addrText = "$+" + QString("%1").arg(rva, -7, 16, QChar(' ')).toUpper();
#endif //_WIN64
        }
        else if(rva < 0)
        {
#ifdef _WIN64
            addrText = "$-" + QString("%1").arg(-rva, -15, 16, QChar(' ')).toUpper();
#else
            addrText = "$-" + QString("%1").arg(-rva, -7, 16, QChar(' ')).toUpper();
#endif //_WIN64
        }
    }
    addrText += ToPtrString(cur_addr);
    char label_[MAX_LABEL_SIZE] = "";
    if(getLabel && DbgGetLabelAt(cur_addr, SEG_DEFAULT, label_)) //has label
    {
        char module[MAX_MODULE_SIZE] = "";
        if(DbgGetModuleAt(cur_addr, module) && !QString(label_).startsWith("JMP.&"))
            addrText += " <" + QString(module) + "." + QString(label_) + ">";
        else
            addrText += " <" + QString(label_) + ">";
    }
    else
        *label_ = 0;
    if(label)
        strcpy_s(label, MAX_LABEL_SIZE, label_);
    return addrText;
}

//The following function is modified from "RichTextPainter::List Disassembly::getRichBytes(const Instruction_t & instr) const"
//with patch and code folding features removed.
RichTextPainter::List TraceBrowser::getRichBytes(const Instruction_t & instr) const
{
    RichTextPainter::List richBytes;
    std::vector<std::pair<size_t, bool>> realBytes;
    formatOpcodeString(instr, richBytes, realBytes);
    const duint cur_addr = instr.rva;

    if(!richBytes.empty() && richBytes.back().text.endsWith(' '))
        richBytes.back().text.chop(1); //remove trailing space if exists

    for(size_t i = 0; i < richBytes.size(); i++)
    {
        auto byteIdx = realBytes[i].first;
        auto isReal = realBytes[i].second;
        RichTextPainter::CustomRichText_t & curByte = richBytes.at(i);
        DBGRELOCATIONINFO relocInfo;
        curByte.highlightColor = mDisassemblyRelocationUnderlineColor;
        if(DbgIsDebugging() && DbgFunctions()->ModRelocationAtAddr(cur_addr + byteIdx, &relocInfo))
        {
            bool prevInSameReloc = relocInfo.rva < cur_addr + byteIdx - DbgFunctions()->ModBaseFromAddr(cur_addr + byteIdx);
            curByte.highlight = isReal;
            curByte.highlightConnectPrev = i > 0 && prevInSameReloc;
        }
        else
        {
            curByte.highlight = false;
            curByte.highlightConnectPrev = false;
        }

        curByte.textColor = mBytesColor;
        curByte.textBackground = mBytesBackgroundColor;
    }
    return richBytes;
}

QString TraceBrowser::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    if(!mTraceFile || mTraceFile->Progress() != 100)
    {
        return "";
    }
    if(mTraceFile->isError())
    {
        GuiAddLogMessage(tr("An error occured when reading trace file.\r\n").toUtf8().constData());
        mTraceFile->Close();
        delete mTraceFile;
        mTraceFile = nullptr;
        setRowCount(0);
        return "";
    }
    if(mHighlightingMode)
    {
        QPen pen(mInstructionHighlightColor);
        pen.setWidth(2);
        painter->setPen(pen);
        QRect rect = viewport()->rect();
        rect.adjust(1, 1, -1, -1);
        painter->drawRect(rect);
    }

    duint index = rowBase + rowOffset;
    duint cur_addr;
    cur_addr = mTraceFile->Registers(index).regcontext.cip;
    bool wIsSelected = (index >= mSelection.fromIndex && index <= mSelection.toIndex);
    if(wIsSelected)
    {
        painter->fillRect(QRect(x, y, w, h), QBrush(mSelectionColor));
    }
    if(index >= mTraceFile->Length())
        return "";
    switch(static_cast<TableColumnIndex>(col))
    {
    case Index:
    {
        return mTraceFile->getIndexText(index);
    }

    case Address:
    {
        QString addrText;
        char label[MAX_LABEL_SIZE] = "";
        if(!DbgIsDebugging())
        {
            addrText = ToPtrString(cur_addr);
            goto NotDebuggingLabel;
        }
        else
            addrText = getAddrText(cur_addr, label, true);
        BPXTYPE bpxtype = DbgGetBpxTypeAt(cur_addr);
        bool isbookmark = DbgGetBookmarkAt(cur_addr);
        //todo: cip
        {
            if(!isbookmark) //no bookmark
            {
                if(*label) //label
                {
                    if(bpxtype == bp_none) //label only : fill label background
                    {
                        painter->setPen(mLabelColor); //red -> address + label text
                        painter->fillRect(QRect(x, y, w, h), QBrush(mLabelBackgroundColor)); //fill label background
                    }
                    else //label + breakpoint
                    {
                        if(bpxtype & bp_normal) //label + normal breakpoint
                        {
                            painter->setPen(mBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + hardware breakpoint only
                        {
                            painter->setPen(mHardwareBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill ?
                        }
                        else //other cases -> do as normal
                        {
                            painter->setPen(mLabelColor); //red -> address + label text
                            painter->fillRect(QRect(x, y, w, h), QBrush(mLabelBackgroundColor)); //fill label background
                        }
                    }
                }
                else //no label
                {
                    if(bpxtype == bp_none) //no label, no breakpoint
                    {
NotDebuggingLabel:
                        QColor background;
                        if(wIsSelected)
                        {
                            background = mSelectedAddressBackgroundColor;
                            painter->setPen(mSelectedAddressColor); //black address (DisassemblySelectedAddressColor)
                        }
                        else
                        {
                            background = mAddressBackgroundColor;
                            painter->setPen(mAddressColor); //DisassemblyAddressColor
                        }
                        if(background.alpha())
                            painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background
                    }
                    else //breakpoint only
                    {
                        if(bpxtype & bp_normal) //normal breakpoint
                        {
                            painter->setPen(mBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //hardware breakpoint only
                        {
                            painter->setPen(mHardwareBreakpointColor);
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill red
                        }
                        else //other cases (memory breakpoint in disassembly) -> do as normal
                        {
                            QColor background;
                            if(wIsSelected)
                            {
                                background = mSelectedAddressBackgroundColor;
                                painter->setPen(mSelectedAddressColor); //black address (DisassemblySelectedAddressColor)
                            }
                            else
                            {
                                background = mAddressBackgroundColor;
                                painter->setPen(mAddressColor);
                            }
                            if(background.alpha())
                                painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background
                        }
                    }
                }
            }
            else //bookmark
            {
                if(*label) //label + bookmark
                {
                    if(bpxtype == bp_none) //label + bookmark
                    {
                        painter->setPen(mLabelColor); //red -> address + label text
                        painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill label background
                    }
                    else //label + breakpoint + bookmark
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        painter->setPen(color);
                        if(bpxtype & bp_normal) //label + bookmark + normal breakpoint
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //label + bookmark + hardware breakpoint only
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill ?
                        }
                    }
                }
                else //bookmark, no label
                {
                    if(bpxtype == bp_none) //bookmark only
                    {
                        painter->setPen(mBookmarkColor); //black address
                        painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill bookmark color
                    }
                    else //bookmark + breakpoint
                    {
                        QColor color = mBookmarkBackgroundColor;
                        if(!color.alpha()) //we don't want transparent text
                            color = mAddressColor;
                        painter->setPen(color);
                        if(bpxtype & bp_normal) //bookmark + normal breakpoint
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBreakpointBackgroundColor)); //fill red
                        }
                        else if(bpxtype & bp_hardware) //bookmark + hardware breakpoint only
                        {
                            painter->fillRect(QRect(x, y, w, h), QBrush(mHardwareBreakpointBackgroundColor)); //fill red
                        }
                        else //other cases (bookmark + memory breakpoint in disassembly) -> do as normal
                        {
                            painter->setPen(mBookmarkColor); //black address
                            painter->fillRect(QRect(x, y, w, h), QBrush(mBookmarkBackgroundColor)); //fill bookmark color
                        }
                    }
                }
            }
        }
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, addrText);
    }
    return "";

    case Opcode:
    {
        unsigned char opcodes[16];
        int opcodeSize = 0;
        mTraceFile->OpCode(index, opcodes, &opcodeSize);
        Instruction_t inst = mDisasm->DisassembleAt(opcodes, opcodeSize, 0, cur_addr, false);
        RichTextPainter::paintRichText(painter, x, y, getColumnWidth(col), getRowHeight(), 4, getRichBytes(inst), mFontMetrics);
        return "";
    }

    case Disassembly:
    {
        RichTextPainter::List richText;
        unsigned char opcodes[16];
        int opcodeSize = 0;
        mTraceFile->OpCode(index, opcodes, &opcodeSize);

        Instruction_t inst = mDisasm->DisassembleAt(opcodes, opcodeSize, 0, cur_addr, false);

        if(mHighlightToken.text.length())
            ZydisTokenizer::TokenToRichText(inst.tokens, richText, &mHighlightToken);
        else
            ZydisTokenizer::TokenToRichText(inst.tokens, richText, 0);
        RichTextPainter::paintRichText(painter, x + 0, y, getColumnWidth(col) - 0, getRowHeight(), 4, richText, mFontMetrics);
        return "";
    }

    case Registers:
    {
        RichTextPainter::List richText;
        auto fakeInstruction = registersTokens(index);
        if(mHighlightToken.text.length())
            ZydisTokenizer::TokenToRichText(fakeInstruction, richText, &mHighlightToken);
        else
            ZydisTokenizer::TokenToRichText(fakeInstruction, richText, 0);
        RichTextPainter::paintRichText(painter, x + 0, y, getColumnWidth(col) - 0, getRowHeight(), 4, richText, mFontMetrics);

        return "";
    }
    case Memory:
    {
        auto fakeInstruction = memoryTokens(index);
        RichTextPainter::List richText;
        if(mHighlightToken.text.length())
            ZydisTokenizer::TokenToRichText(fakeInstruction, richText, &mHighlightToken);
        else
            ZydisTokenizer::TokenToRichText(fakeInstruction, richText, nullptr);
        RichTextPainter::paintRichText(painter, x + 0, y, getColumnWidth(col) - 0, getRowHeight(), 4, richText, mFontMetrics);

        return "";
    }
    case Comments:
    {
        int xinc = 3;
        if(DbgIsDebugging())
        {
            //TODO: draw arguments
            QString comment;
            bool autoComment = false;
            char label[MAX_LABEL_SIZE] = "";
            if(GetCommentFormat(cur_addr, comment, &autoComment))
            {
                QColor backgroundColor;
                if(autoComment)
                {
                    painter->setPen(mAutoCommentColor);
                    backgroundColor = mAutoCommentBackgroundColor;
                }
                else //user comment
                {
                    painter->setPen(mCommentColor);
                    backgroundColor = mCommentBackgroundColor;
                }

                int width = mFontMetrics->width(comment);
                if(width > w)
                    width = w;
                if(width)
                    painter->fillRect(QRect(x + xinc, y, width, h), QBrush(backgroundColor)); //fill comment color
                painter->drawText(QRect(x + xinc, y, width, h), Qt::AlignVCenter | Qt::AlignLeft, comment);
            }
            else if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label)) // label but no comment
            {
                QString labelText(label);
                QColor backgroundColor;
                painter->setPen(mLabelColor);
                backgroundColor = mLabelBackgroundColor;

                int width = mFontMetrics->width(labelText);
                if(width > w)
                    width = w;
                if(width)
                    painter->fillRect(QRect(x + xinc, y, width, h), QBrush(backgroundColor)); //fill comment color
                painter->drawText(QRect(x + xinc, y, width, h), Qt::AlignVCenter | Qt::AlignLeft, labelText);
            }
        }
        return "";
    }

    default:
        return "";
    }
}

ZydisTokenizer::InstructionToken TraceBrowser::memoryTokens(unsigned long long atIndex)
{
    duint MemoryAddress[MAX_MEMORY_OPERANDS];
    duint MemoryOldContent[MAX_MEMORY_OPERANDS];
    duint MemoryNewContent[MAX_MEMORY_OPERANDS];
    bool MemoryIsValid[MAX_MEMORY_OPERANDS];
    int MemoryOperandsCount;
    ZydisTokenizer::InstructionToken fakeInstruction = ZydisTokenizer::InstructionToken();

    MemoryOperandsCount = mTraceFile->MemoryAccessCount(atIndex);
    if(MemoryOperandsCount > 0)
    {
        mTraceFile->MemoryAccessInfo(atIndex, MemoryAddress, MemoryOldContent, MemoryNewContent, MemoryIsValid);
        std::vector<ZydisTokenizer::SingleToken> tokens;

        for(int i = 0; i < MemoryOperandsCount; i++)
        {
            ZydisTokenizer::TokenizeTraceMemory(MemoryAddress[i], MemoryOldContent[i], MemoryNewContent[i], tokens);
        }


        fakeInstruction.tokens.insert(fakeInstruction.tokens.begin(), tokens.begin(), tokens.end());
    }
    return  fakeInstruction;
}

ZydisTokenizer::InstructionToken TraceBrowser::registersTokens(unsigned long long atIndex)
{
    ZydisTokenizer::InstructionToken fakeInstruction = ZydisTokenizer::InstructionToken();
    REGDUMP now = mTraceFile->Registers(atIndex);
    REGDUMP next = (atIndex + 1 < mTraceFile->Length()) ? mTraceFile->Registers(atIndex + 1) : now;
    std::vector<ZydisTokenizer::SingleToken> tokens;

#define addRegValues(str, reg) if (atIndex ==0 || now.regcontext.##reg != next.regcontext.##reg) { \
    ZydisTokenizer::TokenizeTraceRegister(str, now.regcontext.##reg, next.regcontext.##reg, tokens);};

    addRegValues(ArchValue("eax", "rax"), cax)
    addRegValues(ArchValue("ebx", "rbx"), cbx)
    addRegValues(ArchValue("ecx", "rcx"), ccx)
    addRegValues(ArchValue("edx", "rdx"), cdx)
    addRegValues(ArchValue("esp", "rsp"), csp)
    addRegValues(ArchValue("ebp", "rbp"), cbp)
    addRegValues(ArchValue("esi", "rsi"), csi)
    addRegValues(ArchValue("edi", "rdi"), cdi)
#ifdef _WIN64
    addRegValues("r8", r8)
    addRegValues("r9", r9)
    addRegValues("r10", r10)
    addRegValues("r11", r11)
    addRegValues("r12", r12)
    addRegValues("r13", r13)
    addRegValues("r14", r14)
    addRegValues("r15", r15)
#endif //_WIN64

    fakeInstruction.tokens.insert(fakeInstruction.tokens.begin(), tokens.begin(), tokens.end());
    return fakeInstruction;
}

void TraceBrowser::prepareData()
{
    auto viewables = getViewableRowsCount();
    int lines = 0;
    if(mTraceFile != nullptr)
    {
        if(mTraceFile->Progress() == 100)
        {
            if(mTraceFile->Length() < getTableOffset() + viewables)
                lines = mTraceFile->Length() - getTableOffset();
            else
                lines = viewables;
        }
    }
    setNbrOfLineToPrint(lines);
}

void TraceBrowser::setupRightClickContextMenu()
{
    mMenuBuilder = new MenuBuilder(this);
    QAction* toggleRunTrace = makeShortcutAction(DIcon("trace.png"), tr("Start Run Trace"), SLOT(toggleRunTraceSlot()), "ActionToggleRunTrace");
    mMenuBuilder->addAction(toggleRunTrace, [toggleRunTrace](QMenu*)
    {
        if(!DbgIsDebugging())
            return false;
        if(DbgValFromString("tr.runtraceenabled()") == 1)
            toggleRunTrace->setText(tr("Stop Run Trace"));
        else
            toggleRunTrace->setText(tr("Start Run Trace"));
        return true;
    });
    auto mTraceFileIsNull = [this](QMenu*)
    {
        return mTraceFile == nullptr;
    };

    mMenuBuilder->addAction(makeAction(DIcon("folder-horizontal-open.png"), tr("Open"), SLOT(openFileSlot())), mTraceFileIsNull);
    mMenuBuilder->addMenu(makeMenu(DIcon("recentfiles.png"), tr("Recent Files")), [this](QMenu * menu)
    {
        if(mTraceFile == nullptr)
        {
            mMRUList->appendMenu(menu);
            return true;
        }
        else
            return false;
    });
    mMenuBuilder->addAction(makeAction(DIcon("fatal-error.png"), tr("Close"), SLOT(closeFileSlot())), [this](QMenu*)
    {
        return mTraceFile != nullptr;
    });
    mMenuBuilder->addAction(makeAction(DIcon("fatal-error.png"), tr("Close and delete"), SLOT(closeDeleteSlot())), [this](QMenu*)
    {
        return mTraceFile != nullptr;
    });
    mMenuBuilder->addSeparator();
    auto isValid = [this](QMenu*)
    {
        return mTraceFile != nullptr && mTraceFile->Progress() == 100 && mTraceFile->Length() > 0;
    };
    auto isDebugging = [this](QMenu*)
    {
        return mTraceFile != nullptr && mTraceFile->Progress() == 100 && mTraceFile->Length() > 0 && DbgIsDebugging();
    };

    MenuBuilder* copyMenu = new MenuBuilder(this, isValid);
    copyMenu->addAction(makeShortcutAction(DIcon("copy_selection.png"), tr("&Selection"), SLOT(copySelectionSlot()), "ActionCopy"));
    copyMenu->addAction(makeAction(DIcon("copy_selection.png"), tr("Selection to &File"), SLOT(copySelectionToFileSlot())));
    copyMenu->addAction(makeAction(DIcon("copy_selection_no_bytes.png"), tr("Selection (&No Bytes)"), SLOT(copySelectionNoBytesSlot())));
    copyMenu->addAction(makeAction(DIcon("copy_selection_no_bytes.png"), tr("Selection to File (No Bytes)"), SLOT(copySelectionToFileNoBytesSlot())));
    copyMenu->addAction(makeShortcutAction(DIcon("copy_address.png"), tr("Address"), SLOT(copyCipSlot()), "ActionCopyAddress"));
    copyMenu->addAction(makeShortcutAction(DIcon("copy_address.png"), tr("&RVA"), SLOT(copyRvaSlot()), "ActionCopyRva"), isDebugging);
    copyMenu->addAction(makeAction(DIcon("fileoffset.png"), tr("&File Offset"), SLOT(copyFileOffsetSlot())), isDebugging);
    copyMenu->addAction(makeAction(DIcon("copy_disassembly.png"), tr("Disassembly"), SLOT(copyDisassemblySlot())));
    copyMenu->addAction(makeAction(DIcon("copy_address.png"), tr("Index"), SLOT(copyIndexSlot())));

    mMenuBuilder->addMenu(makeMenu(DIcon("copy.png"), tr("&Copy")), copyMenu);
    mMenuBuilder->addAction(makeShortcutAction(DIcon(ArchValue("processor32.png", "processor64.png")), tr("&Follow in Disassembler"), SLOT(followDisassemblySlot()), "ActionFollowDisasm"), isValid);

    mBreakpointMenu = new BreakpointMenu(this, getActionHelperFuncs(), [this, isValid]()
    {
        if(isValid(nullptr))
            return mTraceFile->Registers(getInitialSelection()).regcontext.cip;
        else
            return (duint)0;
    });
    mBreakpointMenu->build(mMenuBuilder);
    mMenuBuilder->addAction(makeShortcutAction(DIcon("label.png"), tr("Label Current Address"), SLOT(setLabelSlot()), "ActionSetLabel"), isDebugging);
    mMenuBuilder->addAction(makeShortcutAction(DIcon("comment.png"), tr("&Comment"), SLOT(setCommentSlot()), "ActionSetComment"), isDebugging);
    mMenuBuilder->addAction(makeShortcutAction(DIcon("highlight.png"), tr("&Highlighting mode"), SLOT(enableHighlightingModeSlot()), "ActionHighlightingMode"), isValid);
    MenuBuilder* gotoMenu = new MenuBuilder(this, isValid);
    gotoMenu->addAction(makeShortcutAction(DIcon("goto.png"), tr("Expression"), SLOT(gotoSlot()), "ActionGotoExpression"), isValid);
    gotoMenu->addAction(makeShortcutAction(DIcon("previous.png"), tr("Previous"), SLOT(gotoPreviousSlot()), "ActionGotoPrevious"), [this](QMenu*)
    {
        return mHistory.historyHasPrev();
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("next.png"), tr("Next"), SLOT(gotoNextSlot()), "ActionGotoNext"), [this](QMenu*)
    {
        return mHistory.historyHasNext();
    });
    mMenuBuilder->addMenu(makeMenu(DIcon("goto.png"), tr("Go to")), gotoMenu);

    MenuBuilder* searchMenu = new MenuBuilder(this, isValid);
    searchMenu->addAction(makeAction(DIcon("search_for_constant.png"), tr("Constant"), SLOT(searchConstantSlot())));
    searchMenu->addAction(makeAction(DIcon("memory-map.png"), tr("Memory Reference"), SLOT(searchMemRefSlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("search.png"), tr("&Search")), searchMenu);

    // The following code adds a menu to view the information about currently selected instruction. When info box is completed, remove me.
    MenuBuilder* infoMenu = new MenuBuilder(this, [this, isValid](QMenu * menu)
    {
        duint MemoryAddress[MAX_MEMORY_OPERANDS];
        duint MemoryOldContent[MAX_MEMORY_OPERANDS];
        duint MemoryNewContent[MAX_MEMORY_OPERANDS];
        bool MemoryIsValid[MAX_MEMORY_OPERANDS];
        int MemoryOperandsCount;
        unsigned long long index;

        if(!isValid(nullptr))
            return false;
        index = getInitialSelection();
        MemoryOperandsCount = mTraceFile->MemoryAccessCount(index);
        if(MemoryOperandsCount > 0)
        {
            mTraceFile->MemoryAccessInfo(index, MemoryAddress, MemoryOldContent, MemoryNewContent, MemoryIsValid);
            bool RvaDisplayEnabled = mRvaDisplayEnabled;
            char nolabel[MAX_LABEL_SIZE];
            mRvaDisplayEnabled = false;
            for(int i = 0; i < MemoryOperandsCount; i++)
            {
                menu->addAction(QString("%1: %2 -> %3").arg(getAddrText(MemoryAddress[i], nolabel, false)).arg(ToPtrString(MemoryOldContent[i])).arg(ToPtrString(MemoryNewContent[i])));
            }
            mRvaDisplayEnabled = RvaDisplayEnabled;
            menu->addSeparator();
        }
        menu->addAction(QString("ThreadID: %1").arg(mTraceFile->ThreadId(index)));
        if(index + 1 < mTraceFile->Length())
        {
            menu->addAction(QString("LastError: %1 -> %2").arg(ToPtrString(mTraceFile->Registers(index).lastError.code)).arg(ToPtrString(mTraceFile->Registers(index + 1).lastError.code)));
        }
        else
        {
            menu->addAction(QString("LastError: %1").arg(ToPtrString(mTraceFile->Registers(index).lastError.code)));
        }
        return true;
    });
    mMenuBuilder->addMenu(makeMenu(tr("Information")), infoMenu);


    QAction* toggleAutoDisassemblyFollowSelection = makeAction(tr("Toggle Auto Disassembly Scroll (off)"), SLOT(toggleAutoDisassemblyFollowSelectionSlot()));
    mMenuBuilder->addAction(toggleAutoDisassemblyFollowSelection, [this, toggleAutoDisassemblyFollowSelection](QMenu*)
    {
        if(!DbgIsDebugging())
            return false;
        if(mAutoDisassemblyFollowSelection)
            toggleAutoDisassemblyFollowSelection->setText(tr("Toggle Auto Disassembly Scroll (on)"));
        else
            toggleAutoDisassemblyFollowSelection->setText(tr("Toggle Auto Disassembly Scroll (off)"));
        return true;
    });
}

void TraceBrowser::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    mMenuBuilder->build(&menu);
    menu.exec(event->globalPos());
}

void TraceBrowser::mousePressEvent(QMouseEvent* event)
{
    duint index = getIndexOffsetFromY(transY(event->y())) + getTableOffset();
    if(getGuiState() != AbstractTableView::NoState || !mTraceFile || mTraceFile->Progress() < 100)
    {
        AbstractTableView::mousePressEvent(event);
        return;
    }
    switch(event->button())
    {
    case Qt::LeftButton:
        if(index < getRowCount())
        {
            if(mHighlightingMode || mPermanentHighlightingMode)
            {
                ZydisTokenizer::InstructionToken tokens;
                int columnPosition = 0;
                if(getColumnIndexFromX(event->x()) == Disassembly)
                {
                    Instruction_t inst;
                    unsigned char opcode[16];
                    int opcodeSize;
                    mTraceFile->OpCode(index, opcode, &opcodeSize);
                    tokens = mDisasm->DisassembleAt(opcode, opcodeSize, mTraceFile->Registers(index).regcontext.cip, 0).tokens;
                    columnPosition = getColumnPosition(Disassembly);
                }
                else if(getColumnIndexFromX(event->x()) == TableColumnIndex::Registers)
                {
                    tokens = registersTokens(index);
                    columnPosition = getColumnPosition(Registers);
                }
                else if(getColumnIndexFromX(event->x()) == Memory)
                {
                    tokens = memoryTokens(index);
                    columnPosition = getColumnPosition(Memory);
                }
                ZydisTokenizer::SingleToken token;
                if(ZydisTokenizer::TokenFromX(tokens, token, event->x() - columnPosition, mFontMetrics))
                {
                    if(ZydisTokenizer::IsHighlightableToken(token))
                    {
                        if(!ZydisTokenizer::TokenEquals(&token, &mHighlightToken) || event->button() == Qt::RightButton)
                            mHighlightToken = token;
                        else
                            mHighlightToken = ZydisTokenizer::SingleToken();
                    }
                    else if(!mPermanentHighlightingMode)
                    {
                        mHighlightToken = ZydisTokenizer::SingleToken();
                    }
                }
                else if(!mPermanentHighlightingMode)
                {
                    mHighlightToken = ZydisTokenizer::SingleToken();
                }
            }
            if(mHighlightingMode) //disable highlighting mode after clicked
            {
                mHighlightingMode = false;
                reloadData();
            }
        }
        if(event->modifiers() & Qt::ShiftModifier)
            expandSelectionUpTo(index);
        else
            setSingleSelection(index);
        mHistory.addVaToHistory(index);
        updateViewport();
        selectionChanged();
        return;

        break;
    case Qt::MiddleButton:
        copyCipSlot();
        MessageBeep(MB_OK);
        break;
    case Qt::BackButton:
        gotoPreviousSlot();
        break;
    case Qt::ForwardButton:
        gotoNextSlot();
        break;
    }

    AbstractTableView::mousePressEvent(event);
}

void TraceBrowser::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() == Qt::LeftButton && mTraceFile != nullptr && mTraceFile->Progress() == 100)
    {
        switch(getColumnIndexFromX(event->x()))
        {
        case Index://Index: follow
            followDisassemblySlot();
            break;
        case Address://Address: set RVA
            if(mRvaDisplayEnabled && mTraceFile->Registers(getInitialSelection()).regcontext.cip == mRvaDisplayBase)
                mRvaDisplayEnabled = false;
            else
            {
                mRvaDisplayEnabled = true;
                mRvaDisplayBase = mTraceFile->Registers(getInitialSelection()).regcontext.cip;
            }
            reloadData();
            break;
        case Opcode: //Opcode: Breakpoint
            mBreakpointMenu->toggleInt3BPActionSlot();
            break;
        case Disassembly: //Instructions: follow
            followDisassemblySlot();
            break;
        case Comments: //Comment
            setCommentSlot();
            break;
        }
    }
    AbstractTableView::mouseDoubleClickEvent(event);
}

void TraceBrowser::mouseMoveEvent(QMouseEvent* event)
{
    dsint index = getIndexOffsetFromY(transY(event->y())) + getTableOffset();
    if((event->buttons() & Qt::LeftButton) != 0 && getGuiState() == AbstractTableView::NoState && mTraceFile != nullptr && mTraceFile->Progress() == 100)
    {
        if(index < getRowCount())
        {
            setSingleSelection(getInitialSelection());
            expandSelectionUpTo(index);
        }
        if(transY(event->y()) > this->height())
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
        }
        else if(transY(event->y()) < 0)
        {
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        }
        updateViewport();
    }
    AbstractTableView::mouseMoveEvent(event);
}

void TraceBrowser::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    auto curindex = getInitialSelection();
    auto visibleindex = curindex;
    if((key == Qt::Key_Up || key == Qt::Key_Down) && mTraceFile && mTraceFile->Progress() == 100)
    {
        if(key == Qt::Key_Up)
        {
            if(event->modifiers() == Qt::ShiftModifier)
            {
                if(curindex == getSelectionStart())
                {
                    if(getSelectionEnd() > 0)
                    {
                        visibleindex = getSelectionEnd() - 1;
                        expandSelectionUpTo(visibleindex);
                    }
                }
                else
                {
                    if(getSelectionStart() > 0)
                    {
                        visibleindex = getSelectionStart() - 1;
                        expandSelectionUpTo(visibleindex);
                    }
                }
            }
            else
            {
                if(curindex > 0)
                {
                    visibleindex = curindex - 1;
                    setSingleSelection(visibleindex);
                }
            }
        }
        else
        {
            if(getSelectionEnd() + 1 < mTraceFile->Length())
            {
                if(event->modifiers() == Qt::ShiftModifier)
                {
                    visibleindex = getSelectionEnd() + 1;
                    expandSelectionUpTo(visibleindex);
                }
                else
                {
                    visibleindex = getSelectionEnd() + 1;
                    setSingleSelection(visibleindex);
                }
            }
        }
        makeVisible(visibleindex);
        mHistory.addVaToHistory(visibleindex);
        updateViewport();

        selectionChanged();
    }
    else
        AbstractTableView::keyPressEvent(event);
}

void TraceBrowser::selectionChanged()
{
    if(mAutoDisassemblyFollowSelection)
        followDisassemblySlot();

    REGDUMP temp;
    temp = mTraceFile->Registers(getInitialSelection());
    emit updateTraceRegistersView(&temp);
}

void TraceBrowser::tokenizerConfigUpdatedSlot()
{
    mDisasm->UpdateConfig();
    mPermanentHighlightingMode = ConfigBool("Disassembler", "PermanentHighlightingMode");
}

void TraceBrowser::expandSelectionUpTo(duint to)
{
    if(to < mSelection.firstSelectedIndex)
    {
        mSelection.fromIndex = to;
    }
    else if(to > mSelection.firstSelectedIndex)
    {
        mSelection.toIndex = to;
    }
    else if(to == mSelection.firstSelectedIndex)
    {
        setSingleSelection(to);
    }
}

void TraceBrowser::setSingleSelection(duint index)
{
    mSelection.firstSelectedIndex = index;
    mSelection.fromIndex = index;
    mSelection.toIndex = index;
}

duint TraceBrowser::getInitialSelection()
{
    return mSelection.firstSelectedIndex;
}

duint TraceBrowser::getSelectionSize()
{
    return mSelection.toIndex - mSelection.fromIndex + 1;
}

duint TraceBrowser::getSelectionStart()
{
    return mSelection.fromIndex;
}

duint TraceBrowser::getSelectionEnd()
{
    return mSelection.toIndex;
}

void TraceBrowser::makeVisible(duint index)
{
    if(index < getTableOffset())
        setTableOffset(index);
    else if(index + 2 > getTableOffset() + getViewableRowsCount())
        setTableOffset(index - getViewableRowsCount() + 2);
}

void TraceBrowser::updateColors()
{
    AbstractTableView::updateColors();
    //ZydisTokenizer::UpdateColors(); //Already called in disassembly
    mDisasm->UpdateConfig();
    mBackgroundColor = ConfigColor("DisassemblyBackgroundColor");

    mInstructionHighlightColor = ConfigColor("InstructionHighlightColor");
    mSelectionColor = ConfigColor("DisassemblySelectionColor");
    mCipBackgroundColor = ConfigColor("DisassemblyCipBackgroundColor");
    mCipColor = ConfigColor("DisassemblyCipColor");
    mBreakpointBackgroundColor = ConfigColor("DisassemblyBreakpointBackgroundColor");
    mBreakpointColor = ConfigColor("DisassemblyBreakpointColor");
    mHardwareBreakpointBackgroundColor = ConfigColor("DisassemblyHardwareBreakpointBackgroundColor");
    mHardwareBreakpointColor = ConfigColor("DisassemblyHardwareBreakpointColor");
    mBookmarkBackgroundColor = ConfigColor("DisassemblyBookmarkBackgroundColor");
    mBookmarkColor = ConfigColor("DisassemblyBookmarkColor");
    mLabelColor = ConfigColor("DisassemblyLabelColor");
    mLabelBackgroundColor = ConfigColor("DisassemblyLabelBackgroundColor");
    mSelectedAddressBackgroundColor = ConfigColor("DisassemblySelectedAddressBackgroundColor");
    mTracedAddressBackgroundColor = ConfigColor("DisassemblyTracedBackgroundColor");
    mSelectedAddressColor = ConfigColor("DisassemblySelectedAddressColor");
    mAddressBackgroundColor = ConfigColor("DisassemblyAddressBackgroundColor");
    mAddressColor = ConfigColor("DisassemblyAddressColor");
    mBytesColor = ConfigColor("DisassemblyBytesColor");
    mBytesBackgroundColor = ConfigColor("DisassemblyBytesBackgroundColor");
    mAutoCommentColor = ConfigColor("DisassemblyAutoCommentColor");
    mAutoCommentBackgroundColor = ConfigColor("DisassemblyAutoCommentBackgroundColor");
    mCommentColor = ConfigColor("DisassemblyCommentColor");
    mCommentBackgroundColor = ConfigColor("DisassemblyCommentBackgroundColor");
    mDisassemblyRelocationUnderlineColor = ConfigColor("DisassemblyRelocationUnderlineColor");
}

void TraceBrowser::openFileSlot()
{
    BrowseDialog browse(this, tr("Open run trace file"), tr("Open trace file"), tr("Run trace files (*.%1);;All files (*.*)").arg(ArchValue("trace32", "trace64")), QApplication::applicationDirPath() + QDir::separator() + "db", false);
    if(browse.exec() != QDialog::Accepted)
        return;
    emit openSlot(browse.path);
}

void TraceBrowser::openSlot(const QString & fileName)
{
    if(mTraceFile != nullptr)
    {
        mTraceFile->Close();
        delete mTraceFile;
    }
    mTraceFile = new TraceFileReader(this);
    connect(mTraceFile, SIGNAL(parseFinished()), this, SLOT(parseFinishedSlot()));
    mFileName = fileName;
    mTraceFile->Open(fileName);
}

void TraceBrowser::toggleRunTraceSlot()
{
    if(!DbgIsDebugging())
        return;
    if(DbgValFromString("tr.runtraceenabled()") == 1)
        DbgCmdExec("StopRunTrace");
    else
    {
        QString defaultFileName;
        char moduleName[MAX_MODULE_SIZE];
        QDateTime currentTime = QDateTime::currentDateTime();
        duint defaultModule = DbgValFromString("mod.main()");
        if(DbgFunctions()->ModNameFromAddr(defaultModule, moduleName, false))
        {
            defaultFileName = QString::fromUtf8(moduleName);
        }
        defaultFileName += "-" + QLocale(QString(currentLocale)).toString(currentTime.date()) + " " + currentTime.time().toString("hh-mm-ss") + ArchValue(".trace32", ".trace64");
        BrowseDialog browse(this, tr("Select stored file"), tr("Store run trace to the following file"),
                            tr("Run trace files (*.%1);;All files (*.*)").arg(ArchValue("trace32", "trace64")), QCoreApplication::applicationDirPath() + QDir::separator() + "db" + QDir::separator() + defaultFileName, true);
        if(browse.exec() == QDialog::Accepted)
        {
            if(browse.path.contains(QChar('"')) || browse.path.contains(QChar('\'')))
                SimpleErrorBox(this, tr("Error"), tr("File name contains invalid character."));
            else
                DbgCmdExec(QString("StartRunTrace \"%1\"").arg(browse.path).toUtf8().constData());
        }
    }
}

void TraceBrowser::closeFileSlot()
{
    if(DbgValFromString("tr.runtraceenabled()") == 1)
        DbgCmdExec("StopRunTrace");
    mTraceFile->Close();
    delete mTraceFile;
    mTraceFile = nullptr;
    reloadData();
}

void TraceBrowser::closeDeleteSlot()
{
    QMessageBox msgbox(QMessageBox::Critical, tr("Close and delete"), tr("Are you really going to delete this file?"), QMessageBox::Yes | QMessageBox::Cancel, this);
    if(msgbox.exec() == QMessageBox::Yes)
    {
        if(DbgValFromString("tr.runtraceenabled()") == 1)
            DbgCmdExecDirect("StopRunTrace");
        mTraceFile->Delete();
        delete mTraceFile;
        mTraceFile = nullptr;
        reloadData();
    }
}

void TraceBrowser::parseFinishedSlot()
{
    if(mTraceFile->isError())
    {
        SimpleErrorBox(this, tr("Error"), "Error when opening run trace file");
        delete mTraceFile;
        mTraceFile = nullptr;
        setRowCount(0);
    }
    else
    {
        if(mTraceFile->HashValue() && DbgIsDebugging())
            if(DbgFunctions()->DbGetHash() != mTraceFile->HashValue())
            {
                SimpleWarningBox(this, tr("Trace file is recorded for another debuggee"),
                                 tr("Checksum is different for current trace file and the debugee. This probably means you have opened a wrong trace file. This trace file is recorded for \"%1\"").arg(mTraceFile->ExePath()));
            }
        setRowCount(mTraceFile->Length());
        mMRUList->addEntry(mFileName);
        mMRUList->save();
    }
    reloadData();
}

void TraceBrowser::gotoSlot()
{
    if(mTraceFile == nullptr || mTraceFile->Progress() < 100)
        return;
    GotoDialog gotoDlg(this, false, true); // TODO: Cannot use when not debugging
    if(gotoDlg.exec() == QDialog::Accepted)
    {
        auto val = DbgValFromString(gotoDlg.expressionText.toUtf8().constData());
        if(val >= 0 && val < mTraceFile->Length())
        {
            setSingleSelection(val);
            makeVisible(val);
            mHistory.addVaToHistory(val);
            updateViewport();
        }
    }
}

void TraceBrowser::gotoNextSlot()
{
    if(mHistory.historyHasNext())
    {
        auto index = mHistory.historyNext();
        setSingleSelection(index);
        makeVisible(index);
        updateViewport();
        selectionChanged();
    }
}

void TraceBrowser::gotoPreviousSlot()
{
    if(mHistory.historyHasPrev())
    {
        auto index = mHistory.historyPrev();
        setSingleSelection(index);
        makeVisible(index);
        updateViewport();
        selectionChanged();
    }
}

void TraceBrowser::copyCipSlot()
{
    QString clipboard;
    for(auto i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        if(i != getSelectionStart())
            clipboard += "\r\n";
        clipboard += ToPtrString(mTraceFile->Registers(i).regcontext.cip);
    }
    Bridge::CopyToClipboard(clipboard);
}

void TraceBrowser::copyIndexSlot()
{
    QString clipboard;
    for(auto i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        if(i != getSelectionStart())
            clipboard += "\r\n";
        clipboard += mTraceFile->getIndexText(i);
    }
    Bridge::CopyToClipboard(clipboard);
}

void TraceBrowser::pushSelectionInto(bool copyBytes, QTextStream & stream, QTextStream* htmlStream)
{
    const int addressLen = getColumnWidth(Address) / getCharWidth() - 1;
    const int bytesLen = getColumnWidth(Opcode) / getCharWidth() - 1;
    const int disassemblyLen = getColumnWidth(Disassembly) / getCharWidth() - 1;
    const int registersLen = getColumnWidth(Registers) / getCharWidth() - 1;
    const int memoryLen = getColumnWidth(Memory) / getCharWidth() - 1;
    if(htmlStream)
        *htmlStream << QString("<table style=\"border-width:0px;border-color:#000000;font-family:%1;font-size:%2px;\">").arg(font().family()).arg(getRowHeight());
    for(unsigned long long i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        if(i != getSelectionStart())
            stream << "\r\n";
        duint cur_addr = mTraceFile->Registers(i).regcontext.cip;
        unsigned char opcode[16];
        int opcodeSize;
        mTraceFile->OpCode(i, opcode, &opcodeSize);
        Instruction_t inst;
        inst = mDisasm->DisassembleAt(opcode, opcodeSize, cur_addr, 0);
        QString address = getAddrText(cur_addr, 0, addressLen > sizeof(duint) * 2 + 1);
        QString bytes;
        QString bytesHTML;
        if(copyBytes)
            RichTextPainter::htmlRichText(getRichBytes(inst), bytesHTML, bytes);
        QString disassembly;
        QString htmlDisassembly;
        if(htmlStream)
        {
            RichTextPainter::List richText;
            if(mHighlightToken.text.length())
                ZydisTokenizer::TokenToRichText(inst.tokens, richText, &mHighlightToken);
            else
                ZydisTokenizer::TokenToRichText(inst.tokens, richText, 0);
            RichTextPainter::htmlRichText(richText, htmlDisassembly, disassembly);
        }
        else
        {
            for(const auto & token : inst.tokens.tokens)
                disassembly += token.text;
        }
        QString fullComment;
        QString comment;
        bool autocomment;
        if(GetCommentFormat(cur_addr, comment, &autocomment))
            fullComment = " " + comment;

        QString registersText;
        QString registersHtml;
        ZydisTokenizer::InstructionToken regTokens = registersTokens(i);
        if(htmlStream)
        {
            RichTextPainter::List richText;
            if(mHighlightToken.text.length())
                ZydisTokenizer::TokenToRichText(regTokens, richText, &mHighlightToken);
            else
                ZydisTokenizer::TokenToRichText(regTokens, richText, 0);
            RichTextPainter::htmlRichText(richText, registersHtml, registersText);
        }
        else
        {
            for(const auto & token : regTokens.tokens)
                registersText += token.text;
        }

        QString memoryText;
        QString memoryHtml;
        ZydisTokenizer::InstructionToken memTokens = memoryTokens(i);
        if(htmlStream)
        {
            RichTextPainter::List richText;
            if(mHighlightToken.text.length())
                ZydisTokenizer::TokenToRichText(memTokens, richText, &mHighlightToken);
            else
                ZydisTokenizer::TokenToRichText(memTokens, richText, 0);
            RichTextPainter::htmlRichText(richText, memoryHtml, memoryText);
        }
        else
        {
            for(const auto & token : memTokens.tokens)
                memoryText += token.text;
        }

        stream << mTraceFile->getIndexText(i) + " | " + address.leftJustified(addressLen, QChar(' '), true);
        if(copyBytes)
            stream << " | " + bytes.leftJustified(bytesLen, QChar(' '), true);
        stream << " | " + disassembly.leftJustified(disassemblyLen, QChar(' '), true);
        stream << " | " + registersText.leftJustified(registersLen, QChar(' '), true);
        stream << " | " + memoryText.leftJustified(memoryLen, QChar(' '), true) + " |" + fullComment;
        if(htmlStream)
        {
            *htmlStream << QString("<tr><td>%1</td><td>%2</td><td>").arg(mTraceFile->getIndexText(i), address.toHtmlEscaped());
            if(copyBytes)
                *htmlStream << QString("%1</td><td>").arg(bytesHTML);
            *htmlStream << QString("%1</td><td>").arg(htmlDisassembly);
            *htmlStream << QString("%1</td><td>").arg(registersText);
            *htmlStream << QString("%1</td><td>").arg(memoryText);
            if(!comment.isEmpty())
            {
                if(autocomment)
                {
                    *htmlStream << QString("<span style=\"color:%1").arg(mAutoCommentColor.name());
                    if(mAutoCommentBackgroundColor != Qt::transparent)
                        *htmlStream << QString(";background-color:%2").arg(mAutoCommentBackgroundColor.name());
                }
                else
                {
                    *htmlStream << QString("<span style=\"color:%1").arg(mCommentColor.name());
                    if(mCommentBackgroundColor != Qt::transparent)
                        *htmlStream << QString(";background-color:%2").arg(mCommentBackgroundColor.name());
                }
                *htmlStream << "\">" << comment.toHtmlEscaped() << "</span></td></tr>";
            }
            else
            {
                char label[MAX_LABEL_SIZE];
                if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label))
                {
                    comment = QString::fromUtf8(label);
                    *htmlStream << QString("<span style=\"color:%1").arg(mLabelColor.name());
                    if(mLabelBackgroundColor != Qt::transparent)
                        *htmlStream << QString(";background-color:%2").arg(mLabelBackgroundColor.name());
                    *htmlStream << "\">" << comment.toHtmlEscaped() << "</span></td></tr>";
                }
                else
                    *htmlStream << "</td></tr>";
            }
        }
    }
    if(htmlStream)
        *htmlStream << "</table>";
}

void TraceBrowser::copySelectionSlot(bool copyBytes)
{
    if(mTraceFile == nullptr || mTraceFile->Progress() < 100)
        return;

    QString selectionString = "";
    QString selectionHtmlString = "";
    QTextStream stream(&selectionString);
    QTextStream htmlStream(&selectionHtmlString);
    pushSelectionInto(copyBytes, stream, &htmlStream);
    Bridge::CopyToClipboard(selectionString, selectionHtmlString);
}

void TraceBrowser::copySelectionToFileSlot(bool copyBytes)
{
    if(mTraceFile == nullptr || mTraceFile->Progress() < 100)
        return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Open File"), "", tr("Text Files (*.txt)"));
    if(fileName != "")
    {
        QFile file(fileName);
        if(!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Error"), tr("Could not open file"));
            return;
        }

        QTextStream stream(&file);
        pushSelectionInto(copyBytes, stream);
        file.close();
    }
}

void TraceBrowser::copySelectionSlot()
{
    copySelectionSlot(true);
}

void TraceBrowser::copySelectionToFileSlot()
{
    copySelectionToFileSlot(true);
}

void TraceBrowser::copySelectionNoBytesSlot()
{
    copySelectionSlot(false);
}

void TraceBrowser::copySelectionToFileNoBytesSlot()
{
    copySelectionToFileSlot(false);
}

void TraceBrowser::copyDisassemblySlot()
{
    if(mTraceFile == nullptr || mTraceFile->Progress() < 100)
        return;

    QString clipboardHtml = QString("<div style=\"font-family: %1; font-size: %2px\">").arg(font().family()).arg(getRowHeight());
    QString clipboard = "";
    for(auto i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        if(i != getSelectionStart())
        {
            clipboard += "\r\n";
            clipboardHtml += "<br/>";
        }
        RichTextPainter::List richText;
        unsigned char opcode[16];
        int opcodeSize;
        mTraceFile->OpCode(i, opcode, &opcodeSize);
        Instruction_t inst = mDisasm->DisassembleAt(opcode, opcodeSize, mTraceFile->Registers(i).regcontext.cip, 0);
        ZydisTokenizer::TokenToRichText(inst.tokens, richText, 0);
        RichTextPainter::htmlRichText(richText, clipboardHtml, clipboard);
    }
    clipboardHtml += QString("</div>");
    Bridge::CopyToClipboard(clipboard, clipboardHtml);
}

void TraceBrowser::copyRvaSlot()
{
    QString text;
    if(mTraceFile == nullptr || mTraceFile->Progress() < 100)
        return;

    for(unsigned long long i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        duint cip = mTraceFile->Registers(i).regcontext.cip;
        duint base = DbgFunctions()->ModBaseFromAddr(cip);
        if(base)
        {
            if(i != getSelectionStart())
                text += "\r\n";
            text += ToHexString(cip - base);
        }
        else
        {
            SimpleWarningBox(this, tr("Error!"), tr("Selection not in a module..."));
            return;
        }
    }
    Bridge::CopyToClipboard(text);
}

void TraceBrowser::copyFileOffsetSlot()
{
    QString text;
    if(mTraceFile == nullptr || mTraceFile->Progress() < 100)
        return;

    for(unsigned long long i = getSelectionStart(); i <= getSelectionEnd(); i++)
    {
        duint cip = mTraceFile->Registers(i).regcontext.cip;
        cip = DbgFunctions()->VaToFileOffset(cip);
        if(cip)
        {
            if(i != getSelectionStart())
                text += "\r\n";
            text += ToHexString(cip);
        }
        else
        {
            SimpleErrorBox(this, tr("Error!"), tr("Selection not in a file..."));
            return;
        }
    }
    Bridge::CopyToClipboard(text);
}

void TraceBrowser::setCommentSlot()
{
    if(!DbgIsDebugging() || mTraceFile == nullptr || mTraceFile->Progress() < 100)
        return;
    duint wVA = mTraceFile->Registers(getInitialSelection()).regcontext.cip;
    LineEditDialog mLineEdit(this);
    mLineEdit.setTextMaxLength(MAX_COMMENT_SIZE - 2);
    QString addr_text = ToPtrString(wVA);
    char comment_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetCommentAt((duint)wVA, comment_text))
    {
        if(comment_text[0] == '\1') //automatic comment
            mLineEdit.setText(QString(comment_text + 1));
        else
            mLineEdit.setText(QString(comment_text));
    }
    mLineEdit.setWindowTitle(tr("Add comment at ") + addr_text);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QString comment = mLineEdit.editText.replace('\r', "").replace('\n', "");
    if(!DbgSetCommentAt(wVA, comment.toUtf8().constData()))
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetCommentAt failed!"));

    static bool easter = isEaster();
    if(easter && comment.toLower() == "oep")
    {
        QFile file(":/icons/images/egg.wav");
        if(file.open(QIODevice::ReadOnly))
        {
            QByteArray egg = file.readAll();
            PlaySoundA(egg.data(), 0, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
        }
    }

    GuiUpdateAllViews();
}

void TraceBrowser::setLabelSlot()
{
    if(!DbgIsDebugging() || mTraceFile == nullptr || mTraceFile->Progress() < 100)
        return;
    duint wVA = mTraceFile->Registers(getInitialSelection()).regcontext.cip;
    LineEditDialog mLineEdit(this);
    mLineEdit.setTextMaxLength(MAX_LABEL_SIZE - 2);
    QString addr_text = ToPtrString(wVA);
    char label_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetLabelAt((duint)wVA, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle(tr("Add label at ") + addr_text);
restart:
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QByteArray utf8data = mLineEdit.editText.toUtf8();
    if(!utf8data.isEmpty() && DbgIsValidExpression(utf8data.constData()) && DbgValFromString(utf8data.constData()) != wVA)
    {
        QMessageBox msg(QMessageBox::Warning, tr("The label may be in use"),
                        tr("The label \"%1\" may be an existing label or a valid expression. Using such label might have undesired effects. Do you still want to continue?").arg(mLineEdit.editText),
                        QMessageBox::Yes | QMessageBox::No, this);
        msg.setWindowIcon(DIcon("compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::No)
            goto restart;
    }
    if(!DbgSetLabelAt(wVA, utf8data.constData()))
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetLabelAt failed!"));

    GuiUpdateAllViews();
}

void TraceBrowser::enableHighlightingModeSlot()
{
    if(mHighlightingMode)
        mHighlightingMode = false;
    else
        mHighlightingMode = true;
    reloadData();
}

void TraceBrowser::followDisassemblySlot()
{
    if(mTraceFile == nullptr || mTraceFile->Progress() < 100)
        return;

    duint cip = mTraceFile->Registers(getInitialSelection()).regcontext.cip;
    if(DbgMemIsValidReadPtr(cip))
        DbgCmdExec(QString("dis ").append(ToPtrString(cip)).toUtf8().constData());
    else
        GuiAddStatusBarMessage(tr("Cannot follow %1. Address is invalid.\n").arg(ToPtrString(cip)).toUtf8().constData());
}

void TraceBrowser::searchConstantSlot()
{
    WordEditDialog constantDlg(this);
    constantDlg.setup(tr("Constant"), 0, sizeof(duint));
    if(constantDlg.exec() == QDialog::Accepted)
    {
        TraceFileSearchConstantRange(mTraceFile, constantDlg.getVal(), constantDlg.getVal());
        emit displayReferencesWidget();
    }
}

void TraceBrowser::searchMemRefSlot()
{
    WordEditDialog memRefDlg(this);
    memRefDlg.setup(tr("References"), 0, sizeof(duint));
    if(memRefDlg.exec() == QDialog::Accepted)
    {
        TraceFileSearchMemReference(mTraceFile, memRefDlg.getVal());
        emit displayReferencesWidget();
    }
}

void TraceBrowser::updateSlot()
{
    if(mTraceFile && mTraceFile->Progress() == 100) // && this->isVisible()
    {
        mTraceFile->purgeLastPage();
        setRowCount(mTraceFile->Length());
        reloadData();
    }
}

void TraceBrowser::toggleAutoDisassemblyFollowSelectionSlot()
{
    mAutoDisassemblyFollowSelection = !mAutoDisassemblyFollowSelection;
}
