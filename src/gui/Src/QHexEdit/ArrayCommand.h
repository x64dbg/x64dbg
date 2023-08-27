#pragma once

#include <QUndoCommand>
#include <QByteArray>

class XByteArray;

class CharCommand : public QUndoCommand
{
public:
    enum { Id = 1234 };
    enum Cmd {insert, remove, replace};

    CharCommand(XByteArray* xData, Cmd cmd, int charPos, char newChar,
                QUndoCommand* parent = nullptr);

    void undo();
    void redo();
    bool mergeWith(const QUndoCommand* command);
    int id() const
    {
        return Id;
    }

private:
    XByteArray* _xData;
    int _charPos;
    char _newChar;
    char _oldChar;
    Cmd _cmd;
};

class ArrayCommand : public QUndoCommand
{
public:
    enum Cmd {insert, remove, replace};
    ArrayCommand(XByteArray* xData, Cmd cmd, int baPos, QByteArray newBa = QByteArray(), int len = 0, QUndoCommand* parent = nullptr);
    void undo();
    void redo();

private:
    Cmd _cmd;
    XByteArray* _xData;
    int _baPos;
    int _len;
    QByteArray _wasChanged;
    QByteArray _newBa;
    QByteArray _oldBa;
};
