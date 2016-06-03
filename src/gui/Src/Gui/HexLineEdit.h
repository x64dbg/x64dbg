#ifndef HEXLINEEDITT_H
#define HEXLINEEDITT_H

#include <QLineEdit>
#include "Configuration.h"

namespace Ui
{
    class HexLineEdit;
}

class HexLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    enum class Encoding
    {
        Ascii,
        Unicode
    };

    explicit HexLineEdit(QWidget* parent = 0);
    ~HexLineEdit();

    void keyPressEvent(QKeyEvent* event);

    void setData(const QByteArray & data);
    QByteArray data();

    void setEncoding(const Encoding encoding);
    Encoding encoding();

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
    Encoding mEncoding;
    bool mKeepSize;
    bool mOverwriteMode;

    QByteArray toEncodedData(const QString & text);
};

#endif // HEXLINEEDITT_H
