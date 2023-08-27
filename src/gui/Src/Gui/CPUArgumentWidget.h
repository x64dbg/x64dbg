#pragma once

#include <QWidget>
#include <vector>
#include "StdTable.h"
#include "StringUtil.h"
#include "Architecture.h"

namespace Ui
{
    class CPUArgumentWidget;
}

class CPUArgumentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CPUArgumentWidget(Architecture* architecture, QWidget* parent = nullptr);
    ~CPUArgumentWidget();

    static QString defaultArgFormat(const QString & format, const QString & expression)
    {
        if(format.length())
            return format;
        return QString("%1 {p:%1} {a:%1}").arg(expression).trimmed();
    }

    static QString defaultArgName(const QString & name, int argN)
    {
        if(name.length())
            return name;
        return QString("%1").arg(argN);
    }

    static QString defaultArgFieldFormat(const QString & argName, const QString & argText)
    {
        if(argText.length())
            return QString("%1: %2").arg(argName).arg(argText);
        return QString();
    }

public slots:
    void disassembleAtSlot(duint addr, duint cip);
    void refreshData();

private slots:
    void contextMenuSlot(QPoint pos);
    void followDisasmSlot();
    void followDumpSlot();
    void followStackSlot();
    void on_comboCallingConvention_currentIndexChanged(int index);
    void on_spinArgCount_valueChanged(int arg1);
    void on_checkBoxLock_stateChanged(int arg1);

private:
    struct Argument
    {
        QString name;
        QString expression32;
        QString expression64;
        QString format32;
        QString format64;

        const QString & getExpression() const
        {
            return ArchValue(expression32, expression64);
        }

        QString getFormat() const
        {
            return CPUArgumentWidget::defaultArgFormat(ArchValue(format32, format64), getExpression());
        }

        explicit Argument(const QString & name, const QString & expression32, const QString & expression64, const QString & format32 = "", const QString & format64 = "")
            : name(name),
              expression32(expression32),
              expression64(expression64),
              format32(format32),
              format64(format64)
        {
        }
    };

    struct CallingConvention
    {
        QString name;
        int stackArgCount;
        QString stackLocation32;
        duint stackOffset32;
        duint callOffset32;
        QString stackLocation64;
        duint callOffset64;
        duint stackOffset64;
        std::vector<Argument> arguments;

        const QString & getStackLocation() const
        {
            return ArchValue(stackLocation32, stackLocation64);
        }

        const duint getStackOffset() const
        {
            return ArchValue(stackOffset32, stackOffset64);
        }

        const duint & getCallOffset() const
        {
            return ArchValue(callOffset32, callOffset64);
        }

        void addArgument(const Argument & argument)
        {
            arguments.push_back(argument);
        }

        explicit CallingConvention(const QString & name,
                                   int stackArgCount = 0,
                                   const QString & stackLocation32 = "esp",
                                   duint stackOffset32 = 0,
                                   duint callOffset32 = sizeof(duint),
                                   const QString & stackLocation64 = "rsp",
                                   duint stackOffset64 = 0x20,
                                   duint callOffset64 = sizeof(duint))
            : name(name),
              stackArgCount(stackArgCount),
              stackLocation32(stackLocation32),
              stackOffset32(stackOffset32),
              callOffset32(callOffset32),
              stackLocation64(stackLocation64),
              stackOffset64(stackOffset64),
              callOffset64(callOffset64)
        {
        }
    };

    Architecture* mArchitecture = nullptr;
    Ui::CPUArgumentWidget* ui = nullptr;
    StdTable* mTable = nullptr;
    int mCurrentCallingConvention = -1;
    duint mStackOffset = 0;
    bool mAllowUpdate = true;
    std::vector<CallingConvention> mCallingConventions;
    std::vector<duint> mArgumentValues;
    QAction* mFollowDisasm;
    QAction* mFollowAddrDisasm;
    QAction* mFollowDump;
    QAction* mFollowAddrDump;
    QAction* mFollowStack;
    QAction* mFollowAddrStack;

    void loadConfig();
    void setupTable();

    void updateStackOffset(bool iscall);
};
