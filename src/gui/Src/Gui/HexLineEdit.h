#pragma once

#include <QLineEdit>

namespace Ui
{
    class HexLineEdit;
}

class HexLineEdit : public QLineEdit
{
    Q_OBJECT

public:

    explicit HexLineEdit(QWidget* parent = 0);
    ~HexLineEdit();

    void keyPressEvent(QKeyEvent* event);

    void setData(const QByteArray & data);
    QByteArray data();

    void setEncoding(QTextCodec* encoding);
    QTextCodec* encoding();

    void setKeepSize(const bool enabled);
    bool keepSize();

    void setOverwriteMode(bool overwriteMode);
    bool overwriteMode();

signals:
    void dataEdited();

private slots:
    void updateData(const QString & arg1);

private:
    Ui::HexLineEdit* ui;

    QByteArray mData;
    QTextCodec* mEncoding;
    bool mKeepSize;
    bool mOverwriteMode;

    QByteArray toEncodedData(const QString & text);
};
